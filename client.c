/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <time.h>

#include "btstack.h"
#include "nxmic_gatt.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#if 1
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#define BTSPECIFIC_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#define BTSPECIFIC_LOG(...)
#endif

#define LED_QUICK_FLASH_DELAY_MS 100
#define LED_SLOW_FLASH_DELAY_MS 1000
#define TIMESTAMP_LENGTH 8

// Gatt Client States
// Defines various states, e.g. scanning, connecting, discovering services, etc.
// TC stands for Temperature Client
typedef enum {
  STATE_OFF,
  STATE_IDLE,
  STATE_W4_SCAN_RESULT,
  STATE_W4_CONNECT,
  STATE_W4_SERVICE_RESULT,
  STATE_W4_CHARACTERISTIC_RESULT,
  STATE_W4_CHARACTERISTIC_READ,
  STATE_W4_CHARACTERISTIC_WRITE,
  STATE_W4_READY
} gc_state_t;

// Global variables
static btstack_packet_callback_registration_t
    hci_event_callback_registration;     // Callback registration for HCI events
static gc_state_t state = STATE_OFF;     // Current state of the Gatt Client
static bd_addr_t server_addr;            // Address of the server device
static bd_addr_type_t server_addr_type;  // Type of the server device address
static hci_con_handle_t connection_handle;    // Handle for the connection
static gatt_client_service_t server_service;  // Service of the server device

// These are the characteristics of the server device
static gatt_client_characteristic_t
    fimrware_version_characteristic;  // Characteristic of the server device
static gatt_client_characteristic_t
    battery_level_characteristic;  // Characteristic of the server device
static gatt_client_characteristic_t
    timestamp_characteristic;  // Characteristic of the server device

static bool listener_registered;  // Flag to check if the listener is registered
static gatt_client_notification_t
    notification_listener;                // Listener for notifications
static btstack_timer_source_t heartbeat;  // Timer source for the heartbeat

// Global Variables for the characteristics values
static uint8_t *firmware_version;  // Firmware version Global Variable
static uint16_t
    firmware_version_length;  // Firmware version length Global Variable

static uint8_t *battery_level;         // Battery level Global Variable
static uint16_t battery_level_length;  // Battery level length Global Variable

static uint8_t *timestamp;         // Timestamp Global Variable
static uint16_t timestamp_length;  // Timestamp length Global Variable

// Add these structures and helper functions after the includes
typedef struct {
  gatt_client_characteristic_t characteristic;
  const uint8_t *uuid128;
  const char *name;
  void (*process_value)(const uint8_t *value, uint16_t length);
} characteristic_handler_t;

// Helper function to read characteristic value
static void read_characteristic_value(characteristic_handler_t *handler) {
  DEBUG_LOG("Reading value of %s characteristic\n", handler->name);
  gatt_client_read_value_of_characteristic(
      handle_gatt_client_event, connection_handle, &handler->characteristic);
}

// Helper function to write characteristic value
static void write_characteristic_value(characteristic_handler_t *handler,
                                       uint16_t length, const uint8_t *data) {
  DEBUG_LOG("Writing value to %s characteristic\n", handler->name);
  gatt_client_write_value_of_characteristic(
      handle_gatt_client_event, connection_handle,
      handler->characteristic.value_handle, length, data);
}

// Helper function to discover characteristic
static void discover_characteristic(characteristic_handler_t *handler) {
  DEBUG_LOG("Discovering %s characteristic\n", handler->name);
  gatt_client_discover_characteristics_for_service_by_uuid128(
      handle_gatt_client_event, connection_handle, &server_service,
      handler->uuid128);
}

// Value processing functions for each characteristic
static void process_firmware_version(const uint8_t *value, uint16_t length) {
  firmware_version = value;
  firmware_version_length = length;
  for (int i = 0; i < length; i++) {
    printf("%02x ", firmware_version[i]);
  }
  printf("\n");
}

static void process_battery_level(const uint8_t *value, uint16_t length) {
  battery_level = value;
  battery_level_length = length;
  uint32_t raw_value = (battery_level[3] << 24) | (battery_level[2] << 16) |
                       (battery_level[1] << 8) | battery_level[0];
  float battery = *(float *)&raw_value;
  printf("Battery level: %f\n", battery);
}

static void process_timestamp(const uint8_t *value, uint16_t length) {
  timestamp = value;
  timestamp_length = length;
  for (int i = 0; i < length; i++) {
    printf("%02x ", timestamp[length - i - 1]);
  }
  printf("\n");

  uint64_t timestamp_value = little_endian_read_32(timestamp, 0);
  timestamp_value |= ((uint64_t)little_endian_read_32(timestamp, 4) << 32);
  printf("Timestamp: %llu\n", timestamp_value);
}

// Define characteristic handlers
static characteristic_handler_t characteristic_handlers[] = {
    {.uuid128 =
         nxmic_gatt_service.characteristics[CHAR_FIRMWARE_VERSION].uuid128,
     .name = "Firmware Version",
     .process_value = process_firmware_version},
    {.uuid128 = nxmic_gatt_service.characteristics[CHAR_BATTERY_LEVEL].uuid128,
     .name = "Battery Level",
     .process_value = process_battery_level},
    {.uuid128 = nxmic_gatt_service.characteristics[CHAR_TIMESTAMP].uuid128,
     .name = "Timestamp",
     .process_value = process_timestamp}};

// Add this before the handle_gatt_client_event function
static characteristic_handler_t *current_handler = NULL;

static void client_start(void) {
  DEBUG_LOG("Start scanning!\n");
  state = STATE_W4_SCAN_RESULT;
  gap_set_scan_parameters(0, 0x0030, 0x0030);
  gap_start_scan();
}

static bool advertisement_report_contains_service(
    uint8_t *service,  // now points to 16-byte UUID
    uint8_t *advertisement_report) {
  // get advertisement from report event
  const uint8_t *adv_data =
      gap_event_advertising_report_get_data(advertisement_report);
  uint8_t adv_len =
      gap_event_advertising_report_get_data_length(advertisement_report);

  // iterate over advertisement data
  ad_context_t context;
  for (ad_iterator_init(&context, adv_len, adv_data);
       ad_iterator_has_more(&context); ad_iterator_next(&context)) {
    uint8_t data_type = ad_iterator_get_data_type(&context);
    uint8_t data_size = ad_iterator_get_data_len(&context);
    const uint8_t *data = ad_iterator_get_data(&context);
    switch (data_type) {
      case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
        BTSPECIFIC_LOG("Advertisement report length: %d\n", adv_len);
        BTSPECIFIC_LOG(
            "Advertisement report data: %02x %02x %02x %02x %02x %02x %02x "
            "%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
            data[0], data[1], data[2], data[3], data[4], data[5], data[6],
            data[7], data[8], data[9], data[10], data[11], data[12], data[13],
            data[14], data[15]);

        for (int i = 0; i < data_size; i += 16) {
          uint8_t uuid[16];
          // Copy and reverse the bytes
          for (int j = 0; j < 16; j++) {
            uuid[j] = data[i + (15 - j)];  // Reverse the byte order
          }
          if (memcmp(uuid, service, 16) == 0) return true;
        }
      default:
        break;
    }
  }
  return false;
}

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel,
                                     uint8_t *packet, uint16_t size) {
  UNUSED(packet_type);
  UNUSED(channel);
  UNUSED(size);

  uint8_t att_status;
  switch (state) {
    case STATE_W4_SERVICE_RESULT:
      switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_SERVICE_QUERY_RESULT:
          gatt_event_service_query_result_get_service(packet, &server_service);
          break;
        case GATT_EVENT_QUERY_COMPLETE:
          att_status = gatt_event_query_complete_get_att_status(packet);
          if (att_status != ATT_ERROR_SUCCESS) {
            printf("Service query failed, ATT Error 0x%02x\n", att_status);
            gap_disconnect(connection_handle);
            break;
          }
          // Start with first characteristic
          current_handler = &characteristic_handlers[0];
          state = STATE_W4_CHARACTERISTIC_RESULT;
          discover_characteristic(current_handler);
          break;
      }
      break;

    case STATE_W4_CHARACTERISTIC_RESULT:
      switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
          gatt_event_characteristic_query_result_get_characteristic(
              packet, &current_handler->characteristic);
          break;
        case GATT_EVENT_QUERY_COMPLETE:
          att_status = gatt_event_query_complete_get_att_status(packet);
          if (att_status != ATT_ERROR_SUCCESS) {
            printf("Characteristic query failed, ATT Error 0x%02x\n",
                   att_status);
            gap_disconnect(connection_handle);
            break;
          }

          state = STATE_W4_CHARACTERISTIC_READ;
          if (current_handler ==
              &characteristic_handlers[2]) {  // Timestamp handler
            // Special handling for timestamp - write current time
            time_t now = time(NULL);
            struct tm *time_info = localtime(&now);
            DEBUG_LOG("%s\n", asctime(time_info));

            uint8_t ts_bytes[8];
            uint64_t current_timestamp = (uint64_t)now;
            for (int i = 0; i < 8; i++) {
              ts_bytes[i] = (current_timestamp >> (8 * i)) & 0xFF;
            }
            state = STATE_W4_CHARACTERISTIC_WRITE;
            write_characteristic_value(current_handler, 8, ts_bytes);
          } else {
            read_characteristic_value(current_handler);
          }
          break;
      }
      break;

    case STATE_W4_CHARACTERISTIC_READ:
      switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:
          current_handler->process_value(
              gatt_event_characteristic_value_query_result_get_value(packet),
              gatt_event_characteristic_value_query_result_get_value_length(
                  packet));
          break;
        case GATT_EVENT_QUERY_COMPLETE:
          att_status = gatt_event_query_complete_get_att_status(packet);
          if (att_status != ATT_ERROR_SUCCESS) {
            printf("Read failed, ATT Error 0x%02x\n", att_status);
            gap_disconnect(connection_handle);
            break;
          }

          // Move to next characteristic or finish
          current_handler++;
          if (current_handler < &characteristic_handlers[3]) {
            state = STATE_W4_CHARACTERISTIC_RESULT;
            discover_characteristic(current_handler);
          } else {
            state = STATE_OFF;
          }
          break;
      }
      break;

    case STATE_W4_CHARACTERISTIC_WRITE:
      switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_QUERY_COMPLETE:
          att_status = gatt_event_query_complete_get_att_status(packet);
          if (att_status != ATT_ERROR_SUCCESS) {
            printf("Write failed, ATT Error 0x%02x\n", att_status);
            gap_disconnect(connection_handle);
            break;
          }
          state = STATE_W4_CHARACTERISTIC_READ;
          read_characteristic_value(current_handler);
          break;
      }
      break;
  }
}

static void hci_event_handler(uint8_t packet_type, uint16_t channel,
                              uint8_t *packet, uint16_t size) {
  UNUSED(size);
  UNUSED(channel);
  bd_addr_t local_addr;
  if (packet_type != HCI_EVENT_PACKET) return;

  uint8_t event_type = hci_event_packet_get_type(packet);
  switch (event_type) {
    case BTSTACK_EVENT_STATE:
      if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
        gap_local_bd_addr(local_addr);
        printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
        client_start();
      } else {
        state = STATE_OFF;
      }
      break;
    case GAP_EVENT_ADVERTISING_REPORT:
      if (state != STATE_W4_SCAN_RESULT) return;
      // check name in advertisement
      if (!advertisement_report_contains_service(nxmic_gatt_service.uuid128,
                                                 packet))
        return;
      // store address and type
      gap_event_advertising_report_get_address(packet, server_addr);
      server_addr_type = gap_event_advertising_report_get_address_type(packet);
      // stop scanning, and connect to the device
      state = STATE_W4_CONNECT;
      gap_stop_scan();
      printf("Connecting to device with addr %s.\n",
             bd_addr_to_str(server_addr));
      gap_connect(server_addr, server_addr_type);
      break;
    case HCI_EVENT_LE_META:
      // wait for connection complete
      switch (hci_event_le_meta_get_subevent_code(packet)) {
        case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
          if (state != STATE_W4_CONNECT) return;
          connection_handle =
              hci_subevent_le_connection_complete_get_connection_handle(packet);
          // initialize gatt client context with handle, and add it to the list
          // of active clients query primary services
          DEBUG_LOG("Search for NXVET service.\n");
          state = STATE_W4_SERVICE_RESULT;
          gatt_client_discover_primary_services_by_uuid128(
              handle_gatt_client_event, connection_handle,
              nxmic_gatt_service.uuid128);
          break;
        default:
          break;
      }
      break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
      // unregister listener
      connection_handle = HCI_CON_HANDLE_INVALID;
      if (listener_registered) {
        listener_registered = false;
        gatt_client_stop_listening_for_characteristic_value_updates(
            &notification_listener);
      }
      printf("Disconnected %s\n", bd_addr_to_str(server_addr));
      if (state == STATE_OFF) break;
      client_start();
      break;
    default:
      break;
  }
}

static void heartbeat_handler(struct btstack_timer_source *ts) {
  // Invert the led
  static bool quick_flash;
  static bool led_on = true;

  led_on = !led_on;
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
  if (listener_registered && led_on) {
    quick_flash = !quick_flash;
  } else if (!listener_registered) {
    quick_flash = false;
  }

  // Restart timer
  btstack_run_loop_set_timer(ts, (led_on || quick_flash)
                                     ? LED_QUICK_FLASH_DELAY_MS
                                     : LED_SLOW_FLASH_DELAY_MS);
  btstack_run_loop_add_timer(ts);
}

int main() {
  stdio_init_all();

  // initialize CYW43 driver architecture (will enable BT if/because
  // CYW43_ENABLE_BLUETOOTH == 1)
  if (cyw43_arch_init()) {
    printf("failed to initialise cyw43_arch\n");
    return -1;
  }

  // Initialize L2CAP
  l2cap_init();

  // Initialize Security Manager
  sm_init();
  sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

  // setup empty ATT server - only needed if LE Peripheral does ATT queries on
  // its own, e.g. Android and iOS
  att_server_init(NULL, NULL, NULL);

  gatt_client_init();

  hci_event_callback_registration.callback = &hci_event_handler;
  hci_add_event_handler(&hci_event_callback_registration);

  // set one-shot btstack timer
  heartbeat.process = &heartbeat_handler;
  btstack_run_loop_set_timer(&heartbeat, LED_SLOW_FLASH_DELAY_MS);
  btstack_run_loop_add_timer(&heartbeat);

  // turn on!
  hci_power_control(HCI_POWER_ON);

  // btstack_run_loop_execute is only required when using the 'polling' method
  // (e.g. using pico_cyw43_arch_poll library). This example uses the
  // 'threadsafe background` method, where BT work is handled in a low priority
  // IRQ, so it is fine to call bt_stack_run_loop_execute() but equally you can
  // continue executing user code.

#if 0  // this is only necessary when using polling (which we aren't, but we're
       // showing it is still safe to call in this case)
  btstack_run_loop_execute();
#else
  // this core is free to do it's own stuff except when using 'polling' method
  // (in which case you should use btstacK_run_loop_ methods to add work to the
  // run loop.

  // this is a forever loop in place of where user code would go.
  while (true) {
    sleep_ms(1000);
  }
#endif
  return 0;
}
