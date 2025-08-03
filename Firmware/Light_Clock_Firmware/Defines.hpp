// Pin Defines, Pin Mappings + modes, Addresses, and other constants


// ================= Button + Pot Pins =================

// TODO: UPDATE THESE!
#define FRONT_BUTTON_PIN    5
#define SIDE_BUTTON_1_PIN   13
#define SIDE_BUTTON_2_PIN   7
#define POT_1_PIN           8
#define POT_2_PIN           9

// ================= 7 Segment Display =================

// I2C Address for 7 Segment Display
#define DISPLAY_ADDRESS      0x70
#define DISPLAY_BRIGTNESS    3

// ================= Seesaw I2C Encoder =================

// Pin for Switch/Button
#define SEESAW_SWITCH        24

// Neopixel Pin
#define SEESAW_NEOPIX        6

// I2C Address for Encoder Module
#define SEESAW_ADDR          0x36

// Color Scheme + Com settings 
#define SEESAW_MODE          NEO_GRB + NEO_KHZ800

// ================= SD Card (SPI) =================

// Chip Select Pin
#define SD_CS_PIN            10

// Mode for SPI bus
#define SD_SPI_MODE          DEDICATED_SPI

// SPI CLK Frequency (Mhz)
#define SD_FREQ              50

// Spi bus name
#define SD_SPI_BUS           &SPI

// ================= LED Matrix =================

// Brightness (0 to 255)
#define MATRIX_BRIGTNESS    20
// See Matrix Driver