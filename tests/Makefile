CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -pedantic
.DEFAULT_GOAL := test
BUILD_DIR ?= /tmp/bright-noise-menu-tests
TARGET := $(BUILD_DIR)/menu_system_test
CLOCK_TARGET := $(BUILD_DIR)/clock_command_test
SETTINGS_TARGET := $(BUILD_DIR)/settings_storage_test
LIGHTING_TARGET := $(BUILD_DIR)/matrix_driver_test
ALARM_TARGET := $(BUILD_DIR)/alarm_system_test
ALARM_SOURCES := ../Firmware/Light_Clock_Firmware/Alarm_System.cpp ../Firmware/Light_Clock_Firmware/Audio_Playback.cpp
AUDIO_TARGET := $(BUILD_DIR)/audio_playback_test
AUDIO_SOURCES := ../Firmware/Light_Clock_Firmware/Audio_Playback.cpp
SLEEP_TARGET := $(BUILD_DIR)/sleep_timer_test
SLEEP_SOURCES := ../Firmware/Light_Clock_Firmware/Sleep_Timer.cpp

$(TARGET) $(CLOCK_TARGET): $(SLEEP_SOURCES) ../Firmware/Light_Clock_Firmware/Sleep_Timer.hpp

$(CLOCK_TARGET): $(AUDIO_SOURCES) ../Firmware/Light_Clock_Firmware/Audio_Playback.hpp

$(TARGET) $(CLOCK_TARGET): $(ALARM_SOURCES) ../Firmware/Light_Clock_Firmware/Alarm_System.hpp

.PHONY: test clean

test: $(TARGET) $(CLOCK_TARGET) $(SETTINGS_TARGET) $(LIGHTING_TARGET) $(ALARM_TARGET) $(AUDIO_TARGET) $(SLEEP_TARGET)
	$(TARGET)
	$(CLOCK_TARGET)
	$(SETTINGS_TARGET)
	$(LIGHTING_TARGET)
	$(ALARM_TARGET)
	$(AUDIO_TARGET)
	$(SLEEP_TARGET)

$(SLEEP_TARGET): Sleep_Timer_Test.cpp $(SLEEP_SOURCES) $(AUDIO_SOURCES) ../Firmware/Light_Clock_Firmware/Sleep_Timer.hpp ../Firmware/Light_Clock_Firmware/Alarm_System.hpp ../Firmware/Light_Clock_Firmware/Audio_Playback.hpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.cpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.hpp ../Firmware/Light_Clock_Firmware/File_Management.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) Sleep_Timer_Test.cpp $(SLEEP_SOURCES) $(AUDIO_SOURCES) ../Firmware/Light_Clock_Firmware/Matrix_Driver.cpp ../Firmware/Light_Clock_Firmware/File_Management.cpp -o $(SLEEP_TARGET)

$(AUDIO_TARGET): Audio_Playback_Test.cpp $(AUDIO_SOURCES) ../Firmware/Light_Clock_Firmware/Audio_Playback.hpp ../Firmware/Light_Clock_Firmware/File_Management.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) Audio_Playback_Test.cpp $(AUDIO_SOURCES) ../Firmware/Light_Clock_Firmware/File_Management.cpp -o $(AUDIO_TARGET)

$(ALARM_TARGET): Alarm_System_Test.cpp ../Firmware/Light_Clock_Firmware/Alarm_System.cpp ../Firmware/Light_Clock_Firmware/Alarm_System.hpp ../Firmware/Light_Clock_Firmware/File_Management.cpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) Alarm_System_Test.cpp $(ALARM_SOURCES) ../Firmware/Light_Clock_Firmware/File_Management.cpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.cpp -o $(ALARM_TARGET)

$(TARGET): Menu_System_Test.cpp ../Firmware/Light_Clock_Firmware/Menu_System.cpp ../Firmware/Light_Clock_Firmware/Menu_System.hpp ../Firmware/Light_Clock_Firmware/Clock_Command.cpp ../Firmware/Light_Clock_Firmware/Clock_Command.hpp ../Firmware/Light_Clock_Firmware/Clock_Display.cpp ../Firmware/Light_Clock_Firmware/Clock_Display.hpp ../Firmware/Light_Clock_Firmware/Menu_Text.cpp ../Firmware/Light_Clock_Firmware/Menu_Text.hpp ../Firmware/Light_Clock_Firmware/File_Management.cpp ../Firmware/Light_Clock_Firmware/File_Management.hpp ../Firmware/Light_Clock_Firmware/Settings_Command.cpp ../Firmware/Light_Clock_Firmware/Settings_Command.hpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.cpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) Menu_System_Test.cpp ../Firmware/Light_Clock_Firmware/Menu_System.cpp ../Firmware/Light_Clock_Firmware/Clock_Command.cpp ../Firmware/Light_Clock_Firmware/Clock_Display.cpp ../Firmware/Light_Clock_Firmware/Menu_Text.cpp ../Firmware/Light_Clock_Firmware/File_Management.cpp ../Firmware/Light_Clock_Firmware/Settings_Command.cpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.cpp $(ALARM_SOURCES) $(SLEEP_SOURCES) -o $(TARGET)

$(CLOCK_TARGET): Clock_Command_Test.cpp ../Firmware/Light_Clock_Firmware/Clock_Command.cpp ../Firmware/Light_Clock_Firmware/Clock_Command.hpp ../Firmware/Light_Clock_Firmware/Serial_Command.cpp ../Firmware/Light_Clock_Firmware/Serial_Command.hpp ../Firmware/Light_Clock_Firmware/File_Management.cpp ../Firmware/Light_Clock_Firmware/Settings_Command.cpp ../Firmware/Light_Clock_Firmware/Settings_Command.hpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.cpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) Clock_Command_Test.cpp ../Firmware/Light_Clock_Firmware/Clock_Command.cpp ../Firmware/Light_Clock_Firmware/Serial_Command.cpp ../Firmware/Light_Clock_Firmware/File_Management.cpp ../Firmware/Light_Clock_Firmware/Settings_Command.cpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.cpp $(ALARM_SOURCES) $(SLEEP_SOURCES) -o $(CLOCK_TARGET)

$(SETTINGS_TARGET): Settings_Storage_Test.cpp ../Firmware/Light_Clock_Firmware/File_Management.cpp ../Firmware/Light_Clock_Firmware/File_Management.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) Settings_Storage_Test.cpp ../Firmware/Light_Clock_Firmware/File_Management.cpp -o $(SETTINGS_TARGET)

$(LIGHTING_TARGET): Matrix_Driver_Test.cpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.cpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.hpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) Matrix_Driver_Test.cpp ../Firmware/Light_Clock_Firmware/Matrix_Driver.cpp -o $(LIGHTING_TARGET)

clean:
	rm -f $(TARGET) $(CLOCK_TARGET) $(SETTINGS_TARGET) $(LIGHTING_TARGET) $(ALARM_TARGET) $(AUDIO_TARGET) $(SLEEP_TARGET)
