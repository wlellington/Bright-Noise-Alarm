#pragma once

#include <stdint.h>

void format_menu_text(uint16_t* display_buffer, const char* text);
void menu_text_window(const char* full_text, uint8_t scroll_position,
                      char output[5]);
