#include "Display_Driver.hpp"

void init_DisplayState(DisplayState* display){
  // Seven Segement
  display->sev_segment = Adafruit_7segment();
  display->sev_segment.begin(DISPLAY_ADDRESS);
  display->screen_brighness = 3;
  display->sev_segment.setBrightness(display->screen_brighness);
  Serial.println("\tINIT: 7 Segment Display Configured");

  // Encoder pixel
  display->encoder_pixel = seesaw_NeoPixel(1, SEESAW_NEOPIX, NEO_GRB + NEO_KHZ800);
  if (! display->encoder_pixel.begin(SEESAW_ADDR)) {
    Serial.println("Couldn't find seesaw neopixel on default address");
    while(1) delay(10);
  }
  Serial.println("\tINIT: Encoder Pixel Configured");
  
  // Display State
  display->screen_mode = SCREEN_MODE_TIME;
  memset(display->screen_text, 0, sizeof(display->screen_text));
  strcpy(display->screen_text, "Hello, World!");
  display->front_button_light = LIGHT_MODE_OFF;
  display->side_button_1_light = LIGHT_MODE_OFF;
  display->side_button_2_light = LIGHT_MODE_OFF;
  display->pause_timer = 0;
  display->scroll_timer = 0;
  display->front_button_light_timer = 0;
  display->side_button_1_light_timer = 0;
  display->side_button_2_light_timer = 0;
}
