#include "Menu_Text.hpp"

#include <string.h>

static const uint16_t MENU_CHARACTER_MAP[] = {
    0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x3D, 0x76, 0x30,
    0x1E, 0x75, 0x38, 0x55, 0x54, 0x5C, 0x73, 0x67, 0x50,
    0x6D, 0x78, 0x3E, 0x1C, 0x1D, 0x64, 0x6E, 0x5B};

static const uint16_t MENU_DIGIT_MAP[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

void format_menu_text(uint16_t* display_buffer, const char* text) {
  for (uint8_t index = 0; index < 5; ++index) {
    display_buffer[index] = 0;
  }

  const uint8_t display_positions[] = {0, 1, 3, 4};
  for (uint8_t index = 0; index < 4 && text[index] != '\0'; ++index) {
    const unsigned char character = text[index];
    uint16_t pattern = 0;

    if (character >= 'A' && character <= 'Z') {
      pattern = MENU_CHARACTER_MAP[character - 'A'];
    } else if (character >= 'a' && character <= 'z') {
      pattern = MENU_CHARACTER_MAP[character - 'a'];
    } else if (character >= '0' && character <= '9') {
      pattern = MENU_DIGIT_MAP[character - '0'];
    } else if (character == '-') {
      pattern = 0x40;
    }

    display_buffer[display_positions[index]] = pattern;
  }
}

void menu_text_window(const char* full_text, uint8_t scroll_position,
                      char output[5]) {
  const size_t full_length = strlen(full_text);
  for (uint8_t index = 0; index < 4; ++index) {
    const size_t source_index = static_cast<size_t>(scroll_position) + index;
    output[index] =
        source_index < full_length ? full_text[source_index] : ' ';
  }
  output[4] = '\0';
}
