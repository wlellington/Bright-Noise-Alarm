#include <Adafruit_LEDBackpack.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_seesaw.h>
#include <I2S.h>
#include <Wire.h>

namespace {

const uint8_t kDisplayAddress = 0x70;
const uint8_t kSeesawAddress = 0x36;
const uint8_t kEncoderSwitch = 24;
const uint8_t kPanelBrightness = 8;
const uint32_t kInputPollMs = 10;
const uint32_t kAnalogReportMs = 250;
const uint32_t kSpeakerSampleRate = 22050;
const uint32_t kSpeakerDurationMs = 100;
const uint16_t kSpeakerAmplitude = 512;
const uint16_t kSpeakerFrequencyHz = 440;
const uint8_t kAnalogPins[] = {26, 27, 28, 29};

Adafruit_7segment display;
Adafruit_seesaw encoder;
Adafruit_NeoMatrix panel(
    8, 8, PIN_EXTERNAL_NEOPIXELS,
    NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS +
        NEO_MATRIX_PROGRESSIVE,
    NEO_GRBW + NEO_KHZ800);
I2S speaker(OUTPUT);

bool encoderReady = false;
bool firstInputSample = true;
bool lastFrontButtonPressed = false;
bool lastEncoderButtonPressed = false;
int32_t lastEncoderPosition = 0;
uint32_t lastInputPoll = 0;
uint32_t lastAnalogReport = 0;

void showDisplayPattern() {
  display.clear();
  display.writeDigitNum(0, 8, true);
  display.writeDigitNum(1, 8, true);
  display.drawColon(true);
  display.writeDigitNum(3, 8, true);
  display.writeDigitNum(4, 8, true);
  display.writeDisplay();
}

void clearPanel() {
  panel.clear();
  panel.show();
}

void runPanelTest() {
  Serial.println(F("LED test: one pixel at a time, brightness 8/255"));
  digitalWrite(PIN_EXTERNAL_POWER, HIGH);
  delay(50);

  for (uint8_t channel = 0; channel < 4; ++channel) {
    for (uint8_t pixel = 0; pixel < 64; ++pixel) {
      panel.clear();
      panel.setPixelColor(
          pixel,
          channel == 0 ? 255 : 0,
          channel == 1 ? 255 : 0,
          channel == 2 ? 255 : 0,
          channel == 3 ? 255 : 0);
      panel.show();
      delay(15);
    }
  }

  clearPanel();
  digitalWrite(PIN_EXTERNAL_POWER, LOW);
  Serial.println(F("LED test complete; panel off"));
}

bool beginSpeaker() {
  if (!speaker.setBCLK(PIN_I2S_BIT_CLOCK) ||
      !speaker.setDATA(PIN_I2S_DATA) ||
      !speaker.setBitsPerSample(16) ||
      !speaker.begin(kSpeakerSampleRate)) {
    Serial.println(F("ERROR: I2S initialization failed"));
    speaker.end();
    return false;
  }
  return true;
}

void runSpeakerTest() {
  Serial.println(F("Speaker test: 440 Hz, 100 ms, amplitude 512/32767"));
  if (!beginSpeaker()) {
    return;
  }

  const uint32_t sampleCount =
      kSpeakerSampleRate * kSpeakerDurationMs / 1000;
  for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
    const uint32_t phase =
        sampleIndex * kSpeakerFrequencyHz * 2 / kSpeakerSampleRate;
    const int16_t value =
        (phase % 2 == 0) ? kSpeakerAmplitude : -kSpeakerAmplitude;
    speaker.write16(value, value);
  }

  for (uint8_t sampleIndex = 0; sampleIndex < 16; ++sampleIndex) {
    speaker.write16(0, 0);
  }
  speaker.end();
  Serial.println(F("Speaker test complete"));
}

void printHelp() {
  Serial.println(F("Commands: d=display pattern, l=RGBW panel scan, s=short tone, h=help"));
}

void handleSerialCommand() {
  while (Serial.available() > 0) {
    const char command = Serial.read();
    switch (command) {
      case 'd':
      case 'D':
        showDisplayPattern();
        Serial.println(F("Display pattern shown"));
        break;
      case 'l':
      case 'L':
        runPanelTest();
        break;
      case 's':
      case 'S':
        runSpeakerTest();
        break;
      case 'h':
      case 'H':
        printHelp();
        break;
      case '\r':
      case '\n':
        break;
      default:
        Serial.println(F("ERROR: unknown command"));
        break;
    }
  }
}

void printInputState(bool frontPressed, bool encoderPressed,
                     int32_t encoderPosition) {
  Serial.print(F("INPUT front="));
  Serial.print(frontPressed ? F("pressed") : F("released"));
  Serial.print(F(" encoder_button="));
  Serial.print(encoderPressed ? F("pressed") : F("released"));
  Serial.print(F(" encoder_position="));
  Serial.println(encoderPosition);
}

void pollInputs(uint32_t now) {
  if (static_cast<uint32_t>(now - lastInputPoll) < kInputPollMs) {
    return;
  }
  lastInputPoll = now;

  const bool frontPressed = digitalRead(PIN_EXTERNAL_BUTTON) == LOW;
  bool encoderPressed = false;
  int32_t encoderPosition = lastEncoderPosition;
  if (encoderReady) {
    encoderPressed = encoder.digitalRead(kEncoderSwitch) == LOW;
    encoderPosition = encoder.getEncoderPosition();
  }

  if (Serial &&
      (firstInputSample || frontPressed != lastFrontButtonPressed ||
       encoderPressed != lastEncoderButtonPressed ||
       encoderPosition != lastEncoderPosition)) {
    printInputState(frontPressed, encoderPressed, encoderPosition);
  }

  firstInputSample = false;
  lastFrontButtonPressed = frontPressed;
  lastEncoderButtonPressed = encoderPressed;
  lastEncoderPosition = encoderPosition;

  if (static_cast<uint32_t>(now - lastAnalogReport) >= kAnalogReportMs) {
    lastAnalogReport = now;
    if (Serial) {
      Serial.print(F("ADC"));
      for (uint8_t index = 0; index < sizeof(kAnalogPins); ++index) {
        Serial.print(' ');
        Serial.print(kAnalogPins[index]);
        Serial.print('=');
        Serial.print(analogRead(kAnalogPins[index]));
      }
      Serial.println();
    }
  }
}

}

void setup() {
  Serial.begin(115200);
  const uint32_t serialWaitStart = millis();
  while (!Serial &&
         static_cast<uint32_t>(millis() - serialWaitStart) < 2000) {
    delay(10);
  }

  Wire.begin();
  pinMode(PIN_EXTERNAL_BUTTON, INPUT_PULLUP);
  pinMode(PIN_EXTERNAL_POWER, OUTPUT);
  digitalWrite(PIN_EXTERNAL_POWER, LOW);

  for (uint8_t index = 0; index < sizeof(kAnalogPins); ++index) {
    pinMode(kAnalogPins[index], INPUT);
  }
  analogReadResolution(12);

  display.begin(kDisplayAddress);
  display.setBrightness(3);
  showDisplayPattern();

  panel.begin();
  panel.setBrightness(kPanelBrightness);
  clearPanel();

  encoderReady = encoder.begin(kSeesawAddress);
  if (encoderReady) {
    encoder.pinMode(kEncoderSwitch, INPUT_PULLUP);
    lastEncoderPosition = encoder.getEncoderPosition();
  } else {
    Serial.println(F("ERROR: seesaw encoder not detected at 0x36"));
  }

  Serial.println(F("Peripheral bring-up ready; outputs are off."));
  printHelp();
}

void loop() {
  handleSerialCommand();
  pollInputs(millis());
}
