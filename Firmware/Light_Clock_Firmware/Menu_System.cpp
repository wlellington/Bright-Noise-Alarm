// All structs and logic regarding the menu system
#include "Menu_System.hpp"

void init_MenuState(MenuState* menu){
  menu->active = false;
  menu->current_menu = 0;
  menu->selected_type = 0;
  menu->selected_value = 0;
  menu->type_max = 0;
  menu->type_min = 0;
}