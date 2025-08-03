// All data structures and processing logic for processing input from the buttons, encoder, pots etc.
#include <stdint.h>
#include "Adafruit_seesaw.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_LIS3DH.h>

#include "Defines.hpp"

// Encoder State Defines
#define ENCODER_NONE      0  
#define ENCODER_UP        1
#define ENCODER_DOWN      2

// Button State Defines
#define BUTTON_NONE       0
#define BUTTON_TAP        1
#define BUTTON_DOUBLE_TAP 2
#define BUTTON_LONG_PRESS 3
#define BUTTON_HOLD       4

#define BUTTON_LONG_TIME  2000 // Greater than -> long press
#define BUTTON_TAP_TIME   400  // Less than -> tap

// Pot State Defines
#define POT_FREE          0 // Value directly translated
#define POT_LOCKED        1 // Value locked at old state
#define POT_READY_TO_FREE 2 // Value will unlock if threshold is exceeded

#define POT_THRESHOLD     20 // Movement from current val necessary to unlock

// ---- Basic input state struct
typedef struct {
  // Input hardware
  Adafruit_LIS3DH accel;
  Adafruit_seesaw encoder;

  // Input tracking
  bool encoder_up;
  bool encoder_down;
  bool encoder_press;
  bool front_button_val;
  bool side_button_1_val;
  bool side_button_2_val;
  uint16_t pot_1_pos;
  uint16_t pot_2_pos;
  unsigned long encoder_timer;
  unsigned long front_button_timer;
  unsigned long side_button_1_timer;
  unsigned long side_button_2_timer;
} InputState;

// ---- Gesture processed input interpretation struct
typedef struct {
  char encoder_direction;
  char encoder_button_press;
  char front_button_press;
  char side_button_1_press;
  char side_button_2_press;
  char pot_1_status;
  uint16_t pot_1_val;
  char pot_2_status;
  uint16_t pot_2_val;
} InputGestures;

void init_InputState(InputState* inputs);
void init_InputGestures(InputGestures* gestures);
void update_InputState(InputState* inputs);
void update_GestureState(InputState* inputs, InputGestures* gestures);