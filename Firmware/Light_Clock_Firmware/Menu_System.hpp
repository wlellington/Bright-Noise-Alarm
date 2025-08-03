// All structs and logic regarding the menu system
#include <stdint.h>
#include "Defines.hpp"

typedef struct {
  bool active;
  char current_menu;
  char selected_type;
  uint16_t selected_value;
  uint16_t type_max;
  uint16_t type_min;
} MenuState;

void init_MenuState(MenuState* menu);