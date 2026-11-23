#include "Input_Processing.hpp"
#include "Serial_Diagnostics.hpp"

static const unsigned long INPUT_DEBOUNCE_MS = 30;

static bool read_accel_register(uint8_t address, uint8_t& value) {
  Wire.beginTransmission(tap_input::ADDRESS);
  Wire.write(address);
  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(tap_input::ADDRESS, static_cast<uint8_t>(1)) != 1)
    return false;
  value = static_cast<uint8_t>(Wire.read());
  return true;
}

static bool configure_accel() {
  static const uint8_t registers[][2] = {
      {0x20, 0x77}, {0x21, 0x00}, {0x22, 0x00}, {0x23, 0x98},
      {0x24, 0x00}, {0x38, 0x00}};
  for (const auto& setting : registers) {
    Wire.beginTransmission(tap_input::ADDRESS);
    Wire.write(setting[0]);
    Wire.write(setting[1]);
    if (Wire.endTransmission() != 0) return false;
    uint8_t actual = 0;
    if (!read_accel_register(setting[0], actual) || actual != setting[1])
      return false;
  }
  return true;
}

static bool read_accel_axes(int16_t axes[3]) {
  Wire.beginTransmission(tap_input::ADDRESS);
  Wire.write(static_cast<uint8_t>(0x28 | 0x80));
  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(tap_input::ADDRESS, static_cast<uint8_t>(6)) != 6)
    return false;
  for (unsigned i = 0; i < 3; ++i) {
    const uint16_t low = static_cast<uint8_t>(Wire.read());
    const uint16_t high = static_cast<uint8_t>(Wire.read());
    axes[i] = static_cast<int16_t>(low | (high << 8));
  }
  return true;
}

static void update_debounced_button(bool sample_pressed,
                                    bool& raw_pressed,
                                    bool& stable_pressed,
                                    unsigned long& changed_at,
                                    unsigned long now) {
  if (sample_pressed != raw_pressed) {
    raw_pressed = sample_pressed;
    changed_at = now;
  }
  if (raw_pressed != stable_pressed &&
      now - changed_at >= INPUT_DEBOUNCE_MS) {
    stable_pressed = raw_pressed;
  }
}

void init_InputState(InputState* inputs) {
  inputs->accel = Adafruit_LIS3DH();
  inputs->tap_status = {};
  inputs->tap_detected = false;
  inputs->tap_poll_ms = static_cast<uint32_t>(millis());
  if (!inputs->accel.begin(tap_input::ADDRESS)) {
    serial_diagnostics_log("INPUT-ERROR: LIS3DH unavailable at 0x18");
  } else if (!configure_accel()) {
    inputs->tap_status.error = tap_input::Error::configuration;
    serial_diagnostics_log("INPUT-ERROR: LIS3DH tap configuration failed");
  } else {
    inputs->tap_status.available = true;
    inputs->tap_status.error = tap_input::Error::none;
    serial_diagnostics_log("INIT: LIS3DH combined XYZ taps, 50mg, 4g");
  }

  inputs->encoder_available = inputs->encoder.begin(SEESAW_ADDR);
  if (inputs->encoder_available) {
    inputs->encoder.pinMode(SEESAW_SWITCH, INPUT_PULLUP);
    inputs->encoder_position = inputs->encoder.getEncoderPosition();
    inputs->encoder_raw_press = !inputs->encoder.digitalRead(SEESAW_SWITCH);
    serial_diagnostics_log("INIT: Encoder Configured");
  } else {
    inputs->encoder_position = 0;
    inputs->encoder_raw_press = false;
    serial_diagnostics_log("INPUT-ERROR: seesaw encoder unavailable");
  }

  pinMode(PIN_EXTERNAL_BUTTON, INPUT_PULLUP);
  const unsigned long now = millis();
  inputs->encoder_delta = 0;
  inputs->encoder_up = false;
  inputs->encoder_down = false;
  inputs->encoder_press = inputs->encoder_raw_press;
  inputs->previous_encoder_press = inputs->encoder_press;
  inputs->front_button_raw_val = !digitalRead(PIN_EXTERNAL_BUTTON);
  inputs->front_button_val = inputs->front_button_raw_val;
  inputs->previous_front_button_val = inputs->front_button_val;
  inputs->side_button_1_val = false;
  inputs->side_button_2_val = false;
  inputs->pot_1_pos = 0;
  inputs->pot_2_pos = 0;

  inputs->encoder_timer = now;
  inputs->front_button_timer = now;
  inputs->side_button_1_timer = now;
  inputs->side_button_2_timer = now;
}

void init_InputGestures(InputGestures* gestures) {
  gestures->tap_detected = false;
  gestures->encoder_direction = ENCODER_NONE;
  gestures->encoder_delta = 0;
  gestures->encoder_button_press = BUTTON_NONE;
  gestures->front_button_press = BUTTON_NONE;
  gestures->side_button_1_press = BUTTON_NONE;
  gestures->side_button_2_press = BUTTON_NONE;
  gestures->pot_1_status = POT_FREE;
  gestures->pot_1_val = 0;
  gestures->pot_2_status = POT_FREE;
  gestures->pot_2_val = 0;
}

void update_InputState(InputState* inputs) {
  const unsigned long now = millis();
  inputs->tap_detected = false;
  if (inputs->tap_status.available &&
      static_cast<uint32_t>(now) - inputs->tap_poll_ms >= 5u) {
    inputs->tap_poll_ms = static_cast<uint32_t>(now);
    int16_t axes[3] = {};
    if (!read_accel_axes(axes)) {
      inputs->tap_status.available = false;
      inputs->tap_status.error = tap_input::Error::read;
      serial_diagnostics_log("INPUT-ERROR: LIS3DH read failed; taps disabled");
    } else {
      inputs->tap_detected = tap_input::update(
          inputs->tap_status.state, axes, static_cast<uint32_t>(now),
          inputs->tap_status.threshold_mg);
    }
  }
  inputs->encoder_delta = 0;
  inputs->encoder_up = false;
  inputs->encoder_down = false;

  if (inputs->encoder_available) {
    const int32_t position = inputs->encoder.getEncoderPosition();
    inputs->encoder_delta = position - inputs->encoder_position;
    inputs->encoder_position = position;
    inputs->encoder_up = inputs->encoder_delta > 0;
    inputs->encoder_down = inputs->encoder_delta < 0;

    update_debounced_button(!inputs->encoder.digitalRead(SEESAW_SWITCH),
                            inputs->encoder_raw_press,
                            inputs->encoder_press,
                            inputs->encoder_timer,
                            now);
  }

  update_debounced_button(!digitalRead(PIN_EXTERNAL_BUTTON),
                          inputs->front_button_raw_val,
                          inputs->front_button_val,
                          inputs->front_button_timer,
                          now);
}

void update_GestureState(InputState* inputs, InputGestures* gestures) {
  gestures->tap_detected = inputs->tap_detected;
  gestures->encoder_delta = inputs->encoder_delta;
  gestures->encoder_direction = ENCODER_NONE;
  if (inputs->encoder_up) {
    gestures->encoder_direction = ENCODER_UP;
  } else if (inputs->encoder_down) {
    gestures->encoder_direction = ENCODER_DOWN;
  }

  gestures->encoder_button_press = BUTTON_NONE;
  if (inputs->encoder_press && !inputs->previous_encoder_press) {
    gestures->encoder_button_press = BUTTON_TAP;
  }
  inputs->previous_encoder_press = inputs->encoder_press;

  gestures->front_button_press = BUTTON_NONE;
  if (inputs->front_button_val && !inputs->previous_front_button_val) {
    gestures->front_button_press = BUTTON_TAP;
  }
  inputs->previous_front_button_val = inputs->front_button_val;

  gestures->side_button_1_press = BUTTON_NONE;
  gestures->side_button_2_press = BUTTON_NONE;
}
