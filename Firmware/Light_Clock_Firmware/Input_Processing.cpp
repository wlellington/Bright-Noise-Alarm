// All data structures and processing logic for processing input from the buttons, encoder, pots etc.
#include "Input_Processing.hpp"

void init_InputState(InputState* inputs){
  // Accelerometer
  inputs->accel = Adafruit_LIS3DH();
  Serial.println("\tINIT: Accelerometer Configured");

  // Seesaw encoder
  if (! inputs->encoder.begin(SEESAW_ADDR)) {
    Serial.println("Couldn't find seesaw encoder on default address");
    while(1) delay(10);
  }
  inputs->encoder.pinMode(SEESAW_SWITCH, INPUT_PULLUP);
  inputs->encoder.setGPIOInterrupts((uint32_t)1 << SEESAW_SWITCH, 1);
  inputs->encoder.enableEncoderInterrupt();
  Serial.println("\tINIT: Encoder Configured");

  // Input State
  inputs->encoder_up = false;
  inputs->encoder_down = false;
  inputs->encoder_press = false;
  inputs->front_button_val = false;
  inputs->side_button_1_val = false;
  inputs->side_button_2_val = false;
  inputs->pot_1_pos = 0;
  inputs->pot_2_pos = 0;

  inputs->encoder_timer = 0;
  inputs->front_button_timer = 0;
  inputs->side_button_1_timer = 0;
  inputs->side_button_2_timer = 0;
}

void init_InputGestures(InputGestures* gestures){
  gestures->encoder_direction = ENCODER_NONE;
  gestures->encoder_button_press = BUTTON_NONE;
  gestures->front_button_press = BUTTON_NONE;
  gestures->side_button_1_press = BUTTON_NONE;
  gestures->side_button_2_press = BUTTON_NONE;
  gestures->pot_1_status = POT_FREE;
  gestures->pot_1_val = 0;
  gestures->pot_2_status = POT_FREE;
  gestures->pot_2_val = 0;
}


// ========================== Input State Update Logic ======================
// Poll pins and whatnot
void update_InputState(InputState* inputs){
  // Read encoder

  // Read buttons
  
  // Read Pots

}


// ========================== Gestrue Interpretation Logic ======================
// Decode the meanings of timeings and whatnot
void update_GestureState(InputState* inputs, InputGestures* gestures){

}
//duration = millis() - inputState->button_timer;