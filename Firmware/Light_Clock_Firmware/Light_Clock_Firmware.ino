// ===================== Libraries + Includes =====================
// Package/lib Includes

#include <gamma.h>
#include <Wire.h>

// User Includes
#include "Defines.hpp"
#include "Audio_Playback.hpp"
#include "Display_Driver.hpp"
#include "File_Management.hpp"
#include "Input_Processing.hpp"
#include "Matrix_Driver.hpp"
#include "Menu_System.hpp"
#include "Time_Keeping.hpp"

// ===================== Global State Objects =====================

// Time Keeping System
TimeKeeper time_keeper;

// Input System
InputState inputs;
InputGestures gestures;

// Menu System
MenuState menu;

// Display System
DisplayState display;

// SD Card - Settings
//FSManager file_system;

// SD Card - Audio Streaming
//

// Audio Playback
//PlaybackState playback;

// LED Matrix

// ================== Synchronization logic ===============
// These are a cheap semaphore system - probably not ideal for anything beyond the boot up sequence
bool core_0_configured = false;
bool core_1_configured = false;

// ===================== Core 0 Logic =====================
void setup() {
  // put your setup code here, to run once:

  // ----- Init Serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("C0-INIT: Serial Started!");

  // ----- Configure Power Pins
  pinMode(PIN_EXTERNAL_POWER, OUTPUT);
  digitalWrite(PIN_EXTERNAL_POWER, HIGH);
  pinMode(PIN_EXTERNAL_BUTTON, INPUT_PULLUP);
  Serial.println("C0-INIT: NEOPIXEL Pins and Power Configured!");

  // ----- Init Time Keeping System
  init_TimeKeeper(&time_keeper);
  Serial.println("C0-INIT: Time System + RTC Initialized!");

  // ----- Init Display
  init_DisplayState(&display);
  Serial.println("C0-INIT: Display Initialized!");

  // ----- Init Encoder + Pots
  init_InputState(&inputs);
  init_InputGestures(&gestures);
  Serial.println("C0-INIT: Input System Initialized!");

  // ----- Init SD Card for settings
  //init_FSManager(&file_system);
  //Serial.println("C0-INIT: SD File System Initialized!");

  // ----- Init Menu System
  init_MenuState(&menu);
  Serial.println("C0-INIT: Menu/UI System Initialized!");

    // ----- Init LED Matrix
  //init_Matrix(&matrix);
  //Serial.println("C0-INIT: LED Matrix Initialized");

  // Unblock core 1 setup
  core_0_configured = true;
  Serial.println("C0-INIT: Core 0 configured, waiting for Core 1!");

  // Mark Init as complete once both cores are in lock step...
  while(core_1_configured == false){
    // Do nothing while we wait for core 1 to initialize the audio system
  }

  delay(1000);
  Serial.println("C0-INIT: All Cores Ready to Go! Starting Loop...");
}

void loop() {
  
  // ----- Read User Input
  //update_input(inputs, gestures);

  // ----- Navigate Menu State (Update settings etc.) 
  //update_menu(gestures, menu);

  // ----- Update Display
  //update_display(display, menu);

  // ----- Update Playback Status
  //update_playback();

  // ----- Update LED Matrix
  //update_matrix(matrix);

}


// ===================== Core 1 Logic =====================
void setup1() {

  delay(2000);
  Serial.println("C1-INIT: Core 1 waiting for Core 0...");

  // ----- Wait for Ok signal from core 0
  while(core_0_configured == false){
    // Do nothing while we wait for core o to initialize everything
  }

  Serial.println("C1-INIT: Core 1 Released, Starting Configuration!");

  // ----- Init SD card for audio streaming
  //init_AudioFileManager(AudioFileManager audio_file_manager)
  //Serial.println("C1-INIT: Audio File Manager Initialized!");

  // ----- Init Audio playback objects
  //init_PlaybackState(PlaybackState playback);
  //Serial.println("C1-INIT: Audio Playback System Initialized!");
  
  // Release core 0 hold, start loop
  core_1_configured = true;
  Serial.println("C1-INIT: Core 0 Released!");

}

void loop1(){

  // ----- Update playback state

}
