// All structs and logic regarding the display system (screen + led buttons)

#include <sys/_stdint.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>

#include "Defines.hpp"

// All structs and logic regarding the menu system
#include <stdint.h>
#include "Defines.hpp"

#define SCREEN_MODE_OFF      0
#define SCREEN_MODE_TIME     1
#define SCREEN_MODE_STATIC   2
#define SCREEN_MODE_SCOLL    3

#define SCROLL_PAUSE          400
#define SCROLL_PERIOD         100

#define LIGHT_MODE_OFF       0
#define LIGHT_MODE_DIM       1
#define LIGHT_MODE_FULL      2
#define LIGHT_MODE_PULSE     3
#define LIGHT_MODE_BLINK     4

typedef struct {
  // Seven Segment
  Adafruit_7segment sev_segment;
  
  // Encoder neopixel
  seesaw_NeoPixel encoder_pixel;
  
  // Screen output
  char screen_mode;
  char screen_text[100] = "";
  uint8_t screen_brighness;
  char front_button_light;
  char side_button_1_light;
  char side_button_2_light;

  //Timers
  unsigned long pause_timer;
  unsigned long scroll_timer;
  unsigned long front_button_light_timer;
  unsigned long side_button_1_light_timer;
  unsigned long side_button_2_light_timer;
} DisplayState;

void init_DisplayState(DisplayState* display);
