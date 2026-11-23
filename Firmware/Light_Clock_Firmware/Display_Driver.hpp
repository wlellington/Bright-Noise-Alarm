// All structs and logic regarding the display system (screen + led buttons)

#include <stdint.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>

#include "Defines.hpp"
#include "Menu_System.hpp"
#include "Menu_Text.hpp"
#include "Time_Keeping.hpp"
#include "Clock_Display.hpp"

// All structs and logic regarding the menu system
#include <stdint.h>
#include "Defines.hpp"

#define SCREEN_MODE_OFF      0
#define SCREEN_MODE_TIME     1
#define SCREEN_MODE_STATIC   2
#define SCREEN_MODE_SCOLL    3

#define SCROLL_SPEED_SLOW      0
#define SCROLL_SPEED_MEDIUM    1
#define SCROLL_SPEED_FAST      2
#define SCROLL_TEXT_CAPACITY   32

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
  bool encoder_pixel_ready;
  
  // Screen output
  char screen_mode;
  char screen_text[100];
  ClockWakeState clock_wake;
  SleepClockState sleep_clock;
  bool clock_blanked;
  bool alarm_active;
  bool alarm_snoozed;
  bool alarm_hold;
  bool alarm_audio_fault;
  uint8_t screen_brighness;
  char front_button_light;
  char side_button_1_light;
  char side_button_2_light;

  //Timers
  unsigned long pause_timer;
  unsigned long last_scroll;
  uint8_t scroll_speed;
  uint8_t scroll_position;
  char scroll_text[SCROLL_TEXT_CAPACITY];
  bool last_scroll_colon;
  MenuScreen last_menu_screen;
  unsigned long front_button_light_timer;
  unsigned long side_button_1_light_timer;
  unsigned long side_button_2_light_timer;
  unsigned long refresh_timer;
} DisplayState;

void init_DisplayState(DisplayState* display);
void update_Display(DisplayState* display, const MenuState* menu,
                    TimeKeeper* time_keeper);
void format_menu_text(uint16_t* display_buffer, const char* text);
void display_formatted_text(DisplayState* display, uint16_t* display_buffer,
                            bool show_colon);
void display_scrolling_text(DisplayState* display, const char* full_text,
                            bool show_colon);
