// This file is based on snippets copied from the example sketches for all of the included hardware.
// Please look at the original documentation and example sketches for each library mentioned for more detail.



#include <gamma.h>
#include <Adafruit_Sensor.h>

// =========================== RTC ===============================
#include <RTClib.h>
RTC_PCF8523 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// ======================== Seven Segment Display=======================
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

// I2C address of the display.  Stick with the default address of 0x70
// unless you've changed the address jumpers on the back of the display.
#define DISPLAY_ADDRESS   0x70

// Create display and DS1307 objects.  These are global variables that
// can be accessed from both the setup and loop function below.
Adafruit_7segment clockDisplay = Adafruit_7segment();

// ======================== LED Panel =======================
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>

#define LED_COUNT 64

//Adafruit_NeoPixel strip(LED_COUNT, PIN_EXTERNAL_NEOPIXELS, NEO_RGBW + NEO_KHZ800);
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN_EXTERNAL_NEOPIXELS,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_RGBW           + NEO_KHZ800);

const uint16_t colors[] = { matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255) };
int cursor    = matrix.width();
int pass = 0;
uint32_t color = 0;


// ======================== IMU =======================
#include <Adafruit_LIS3DH.h>
Adafruit_LIS3DH lis = Adafruit_LIS3DH();


// ======================== Encoder =======================
#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>

#define SS_SWITCH        24
#define SS_NEOPIX        6
#define SEESAW_ADDR          0x36

Adafruit_seesaw ss;
seesaw_NeoPixel sspixel = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);

int32_t encoder_position;

// ======================== Speaker =======================
#include <I2S.h>

// ======================== SD Card =======================
#include <SPI.h>
#include "SdFat.h"

#define SD_CS_PIN 10

SdFat SD;
FsFile openFile;
//SdSpiConfig config(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(16), &SPI);
SdSpiConfig config(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(50), &SPI);

// ======================== Main Functions =======================
uint8_t x = 0;

void setup() {
  // core1 setup
  Serial.begin(115200);
  delay(1000);

  // IMU
  if (! lis.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start LIS3DH");
    while (1) yield();
  }

  lis.setRange(LIS3DH_RANGE_2_G);
  Serial.println("IMU Configured");

  // LED Panel Pin Configuration
  pinMode(PIN_EXTERNAL_POWER, OUTPUT);
  digitalWrite(PIN_EXTERNAL_POWER, HIGH);
  pinMode(PIN_EXTERNAL_BUTTON, INPUT_PULLUP);
  Serial.println("NEOPIXEL Pins and Power Configured");

  // Setup the RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  Serial.println("RTC Connection Established");

  // Setup the display.
  clockDisplay.begin(DISPLAY_ADDRESS);
  clockDisplay.setBrightness(3);
  Serial.println("7 Segment Display Configured");

  // Set up the encoder
  if (! ss.begin(SEESAW_ADDR) || ! sspixel.begin(SEESAW_ADDR)) {
    Serial.println("Couldn't find seesaw on default address");
    while(1) delay(10);
  }
  Serial.println("Seesaw started");

  uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
  if (version  != 4991){
    Serial.print("Wrong firmware loaded? ");
    Serial.println(version);
    while(1) delay(10);
  }
  Serial.println("Found Seesaw Encoder Firmware");

  // set not so bright!
  sspixel.setBrightness(20);
  sspixel.show();
  Serial.println("Seesaw NEOPixel Configured");
  
  // use a pin for the built in encoder switch
  ss.pinMode(SS_SWITCH, INPUT_PULLUP);

  // get starting position
  encoder_position = ss.getEncoderPosition();

  Serial.println("Turning on interrupts");
  delay(10);
  ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
  ss.enableEncoderInterrupt();

  //LED Panel
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(20);
  matrix.setTextColor(colors[0]);
  Serial.println("NEOPIXEL Panel Configured");
  
  // SD Card 
  Serial.print("Initializing SD card...");



  // Retry mechanism for SD card initialization
  while (!SD.begin(config)) {
    Serial.println("initialization failed! Retrying...");
    delay(1000); // Wait for a second before retrying
  }
  Serial.println("SD initialization done.");
  SD.ls("/", LS_R);

  // Read SD Card file
  openFile = SD.open("read_test_file.txt", FILE_READ);
  if (openFile){
    // read from the file until there's nothing else in it:
    while (openFile.available()) {
      Serial.write(openFile.read());
    }
    openFile.close();
  }
  else{
    Serial.println("Could not open read_test_file.txt");
  }
  
  int times_accessed = 0;

  // Read and Write to SD Card file - come back to this
  /*
  openFile = SD.open("write_test_file.txt", FILE_READ);
  if (openFile){
    openFile.read();
    times_accessed = openFile.readln().toInt();
    times_accessed++;
    Serial.println("Setting times SD card read to: ", times_accessed);
    openFile.close();
    openFile = SD.open("write_test_file.txt", FILE_WRITE);
    openFile.println("Times Accessed:");
    openFile.println(times_accessed);
    openFile.close();
  }
  else{
    Serial.println("Could not open write_test_file.txt");
  }
  */
  Serial.println("SD Access Tests Done");
  delay(5000);
}

void loop() {

  delay(10);
  x++;
  //for(int32_t i=0; i< strip.numPixels(); i++) {
  //    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + x) & 255));
  //}
  //strip.show();
  // Print X Y & Z accelerometer data
  if (x % 50 == 0) {
    // every 100ms
    sensors_event_t event;
    lis.getEvent(&event);
    /* Display the results (acceleration is measured in m/s^2) */
    Serial.print("Accel X: "); Serial.print(event.acceleration.x);
    Serial.print(" \tY: "); Serial.print(event.acceleration.y);
    Serial.print(" \tZ: "); Serial.print(event.acceleration.z);
    Serial.println(" m/s^2 ");
    Serial.println(x);
  }  
  // external button press disable external power
  if (! digitalRead(PIN_EXTERNAL_BUTTON)) {
    Serial.println("External button pressed");
    digitalWrite(PIN_EXTERNAL_POWER, LOW);
  }
  else {
    digitalWrite(PIN_EXTERNAL_POWER, HIGH);
  }

  // Show RTC values
  if (x % 50 == 0){
    display_rtc_time();
  }

  // read encoder
  int32_t new_position = ss.getEncoderPosition();
  
  // did we move arounde?
  if (encoder_position != new_position) {
    Serial.println("Encoder moved to:");         // display new position
    Serial.println(new_position);

    // change the neopixel color in response
    color = EncoderWheel(new_position & 0xFF);
    sspixel.setPixelColor(0, color);
    sspixel.show();
  }

  // Special condition to stop button press detection on each rotation
  if ((!ss.digitalRead(SS_SWITCH)) && (encoder_position == new_position)) {
    Serial.println("Button pressed!");
    for(int i = 0; i < 64; i++){
      matrix.setPixelColor(i, 0, 0, 0, 20);
    }
  }
  else{
    // LED Panel
    if (x % 20 == 0){
      matrix.fillScreen(matrix.Color(0, 0, 0));
      matrix.setCursor(cursor, 0);
      matrix.print(F("Howdy"));
      if(--cursor < -36) {
        cursor = matrix.width();
      }
      matrix.setTextColor(color);
    }
  }
  encoder_position = new_position;      // and save for next round
  matrix.show();

  // print some stuff to 7 segment display
  clockDisplay.print(encoder_position, DEC);
  clockDisplay.writeDisplay();
  return;
  
}

// ======================== Utility Functions =======================

void display_rtc_time(){
  DateTime now = rtc.now();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");

    // calculate a date which is 7 days, 12 hours and 30 seconds into the future
    DateTime future (now + TimeSpan(7,12,30,6));

    Serial.print(" now + 7d + 12h + 30m + 6s: ");
    Serial.print(future.year(), DEC);
    Serial.print('/');
    Serial.print(future.month(), DEC);
    Serial.print('/');
    Serial.print(future.day(), DEC);
    Serial.print(' ');
    Serial.print(future.hour(), DEC);
    Serial.print(':');
    Serial.print(future.minute(), DEC);
    Serial.print(':');
    Serial.print(future.second(), DEC);
    Serial.println();

    Serial.println();
}


uint32_t EncoderWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return sspixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return sspixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return sspixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


// =================== Core Two (audio) Related ==================

#include <BackgroundAudioWAV.h>
#include <BackgroundAudioMixer.h>

#define N_SOUNDS (sizeof(sound) / sizeof(sound[0]))
#define MAIN_AUDIO_FILE "/Ooblets_41.wav"
#define MENU_CLICK_FILE "/Click.wav"

I2S i2s(OUTPUT);
FsFile main_file;
FsFile menu_file;
uint8_t filebuff[512];
BackgroundAudioWAV main_audio;
BackgroundAudioWAV menu_audio;
BackgroundAudioMixer<640> mixer(i2s, 44100);

uint8_t sndIdx = 0;

void setup1(void) {
  i2s.setBCLK(PIN_I2S_BIT_CLOCK);
  i2s.setDATA(PIN_I2S_DATA);
  i2s.setBitsPerSample(16);

  // wait for SD init on other core!
  delay(2000);

  main_file = SD.open(MAIN_AUDIO_FILE);
  if (!main_file) {
    Serial.printf("Unable to open %s", MAIN_AUDIO_FILE);
  }

  // Enroll Audio Channels into mixer
  main_audio.setDevice(mixer.add());
  menu_audio.setDevice(mixer.add());

  mixer.begin();
  main_audio.begin();
  menu_audio.begin();

  Serial.println("Core #2 Playing audio @ 44.1Khz...");
}

float gain_setting = 1.0;

void loop1() {
  
  gain_setting = ((encoder_position + 10) % 100) / 100.0;
  
  main_audio.setGain(gain_setting);

  // Main Audio Loop
  while (main_file && main_audio.availableForWrite() > 512) {
    int len = main_file.read(filebuff, 512);
    main_audio.write(filebuff, len);
    if (len != 512) {
      main_file.close();
    }
  }

  // Menu Clicky noises :) TBD
}
