#define USER_SETUP_INFO "ESP32-S3 + ST7796"

// ==== Display Driver ====
#define ST7796_DRIVER
#define TFT_RGB_ORDER TFT_BGR

// ==== DMA ====
#define USE_DMA_TRANSFERS
#define TFT_SKIP_DMA_CHECK

// ==== SPI Bus ====
#define USE_HSPI_PORT
#define SPI_FREQUENCY       70000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000
#define SUPPORT_TRANSACTIONS

// ==== Pin Mapping ====
#define TFT_MISO 38
#define TFT_CS   9
#define TFT_RST  40
#define TFT_DC   41
#define TFT_MOSI 42
#define TFT_SCLK 2
#define TFT_BL   1
#define TFT_BACKLIGHT_ON HIGH

// ==== Fonts ====
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT