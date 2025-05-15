#include <stdint.h>

#define MAX_CHARACTERISTICS 16  // adjust as needed

// NXMIC GATT Characteristics Properties
#define GATT_CHAR_READ 0x01
#define GATT_CHAR_WRITE 0x02
#define GATT_CHAR_NOTIFY 0x04
#define GATT_CHAR_INDICATE 0x08
#define GATT_CHAR_AUTH_READ 0x10
#define GATT_CHAR_AUTH_WRITE 0x20

typedef struct {
  uint8_t char_id;
  uint8_t uuid128[16];    // 128-bit UUID
  uint16_t handle;        // Characteristic handle
  uint16_t value_handle;  // Handle to the actual value
  uint8_t properties;     // e.g., read/write/notify
} gatt_characteristic_t;

typedef struct {
  uint8_t uuid128[16];  // 128-bit Service UUID
  gatt_characteristic_t characteristics[MAX_CHARACTERISTICS];
  uint8_t num_characteristics;
} gatt_service_t;

typedef enum {
  CHAR_DEVICE_SERIAL,
  CHAR_TIMESTAMP,
  CHAR_FIRMWARE_VERSION,
  CHAR_IMU_STREAMING,
  CHAR_TEMPERATURE_STREAMING,
  CHAR_STETHOSCOPE_STREAMING,
  CHAR_STETHOSCOPE_PREVIEW_STREAMING,
  CHAR_ECG_STREAMING,
  CHAR_LED_INDICATE,
  CHAR_BATTERY_LEVEL,
  CHAR_DEVICE_CONTROL,
  CHAR_ACTIVE_RECORDING,
  CHAR_DATA_EXPORT,
  CHAR_LABEL_DATA,
  CHAR_RECORDING_INTERVAL_SETTINGS,
  CHAR_FILESYSTEM_MANAGEMENT,
  CHAR_COUNT  // total number of characteristics
} gatt_characteristic_id_t;

// NXMIC GATT Service
gatt_service_t nxmic_gatt_service = {
    .uuid128 = {0x41, 0x2b, 0x27, 0x81, 0x29, 0x87, 0x44, 0x46, 0x9c, 0x62,
                0xfc, 0x2e, 0x52, 0x62, 0x86, 0xa4},
    .characteristics =
        {// Device Serial Characteristic
         {.char_id = CHAR_DEVICE_SERIAL,
          .uuid128 = {0x66, 0xaa, 0xca, 0x5e, 0xbd, 0x89, 0x45, 0xcc, 0x82,
                      0x4f, 0x47, 0x93, 0x8b, 0x44, 0xdb, 0x77},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ},

         // Timestamp Characteristic
         {.char_id = CHAR_TIMESTAMP,
          .uuid128 = {0x44, 0x12, 0x51, 0xcc, 0x7b, 0xfb, 0x44, 0x17, 0xa3,
                      0x44, 0x88, 0xcd, 0x67, 0x8a, 0x9e, 0xd3},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_WRITE},

         // Firmware Version Characteristic
         {.char_id = CHAR_FIRMWARE_VERSION,
          .uuid128 = {0x87, 0x2e, 0x36, 0x97, 0xc0, 0x6e, 0x4e, 0xe2, 0xb2,
                      0xb4, 0xb9, 0x7f, 0x56, 0x68, 0xbd, 0x31},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ},

         // IMU Streaming Characteristic
         {.char_id = CHAR_IMU_STREAMING,
          .uuid128 = {0x33, 0x33, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10,
                      0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_NOTIFY},

         // Temperature Streaming Characteristic
         {.char_id = CHAR_TEMPERATURE_STREAMING,
          .uuid128 = {0x55, 0x55, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10,
                      0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_NOTIFY},

         // Stethoscope Streaming Characteristic
         {.char_id = CHAR_STETHOSCOPE_STREAMING,
          .uuid128 = {0x11, 0x11, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10,
                      0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_NOTIFY},

         // Stethoscope Preview Streaming Characteristic
         {.char_id = CHAR_STETHOSCOPE_PREVIEW_STREAMING,
          .uuid128 = {0x77, 0x77, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10,
                      0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_NOTIFY},

         // ECG Streaming Characteristic
         {.char_id = CHAR_ECG_STREAMING,
          .uuid128 = {0xaa, 0xaa, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10,
                      0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_NOTIFY},

         // LED Indicate Characteristic
         {.char_id = CHAR_LED_INDICATE,
          .uuid128 = {0x88, 0x88, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10,
                      0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_WRITE},

         //    Level Characteristic
         {.char_id = CHAR_BATTERY_LEVEL,
          .uuid128 = {0x99, 0x99, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10,
                      0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ},

         // Device Control Characteristic
         {.char_id = CHAR_DEVICE_CONTROL,
          .uuid128 = {0x00, 0x00, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10,
                      0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_WRITE},

         // Active Recording Characteristic
         {.char_id = CHAR_ACTIVE_RECORDING,
          .uuid128 = {0x66, 0x66, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10,
                      0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_WRITE | GATT_CHAR_NOTIFY},

         // Data Export Characteristic
         {.char_id = CHAR_DATA_EXPORT,
          .uuid128 = {0x19, 0x09, 0x93, 0xf1, 0xc8, 0x1d, 0x49, 0x2b, 0xa8,
                      0x29, 0x80, 0x4f, 0x28, 0xb9, 0x74, 0x5d},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_WRITE | GATT_CHAR_NOTIFY},

         // Label Data Characteristic
         {.char_id = CHAR_LABEL_DATA,
          .uuid128 = {0xb5, 0xf5, 0x33, 0x48, 0xc6, 0x01, 0x47, 0x1d, 0x8e,
                      0xde, 0xf9, 0x0b, 0x24, 0x76, 0x08, 0x75},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_WRITE | GATT_CHAR_NOTIFY},

         // Recording Interval Settings Characteristic
         {.char_id = CHAR_RECORDING_INTERVAL_SETTINGS,
          .uuid128 = {0x9d, 0xd7, 0xd8, 0xa4, 0x0e, 0xd9, 0x4d, 0x35, 0xbe,
                      0xb1, 0x84, 0x6a, 0x98, 0xee, 0xab, 0xa9},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_WRITE},

         // Filesystem Management Characteristic
         {.char_id = CHAR_FILESYSTEM_MANAGEMENT,
          .uuid128 = {0x44, 0x44, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe, 0x10,
                      0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe},
          .handle = 0x0000,
          .value_handle = 0x0000,
          .properties = GATT_CHAR_READ | GATT_CHAR_WRITE}},
    .num_characteristics = CHAR_COUNT  // Using the enum count
};
