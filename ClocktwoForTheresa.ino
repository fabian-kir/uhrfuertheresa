#include <FastLED.h>
#include <RtcDS1302.h>
#include <EEPROM.h>
#include <math.h>
using namespace std;

void(* resetFunc) (void) = 0;//declare reset function at address 0

#define SHOW_TIME 0
#define SET_COLOR 1
#define SET_BRIGHTNESS 2
#define SET_TIME_HOURS 3
#define SET_TIME_MINUTES 4
#define TEST 5

int globalClockState = TEST;


// SETUP LEDS:
#define NUM_LEDS 12*12
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define LED_PIN 5
#define MAX_BRIGHTNESS 100
int userSetBrightness = MAX_BRIGHTNESS;

#define NUM_COLORS 34

int paletteIndex = 0;
CRGB palette[NUM_COLORS] = {
CRGB::White,
CRGB::Red,
CRGB::Crimson,
CRGB::DarkRed,
CRGB::OrangeRed,
CRGB::DarkOrange,
CRGB::Orange,
CRGB::Gold,
CRGB::Yellow,
CRGB::FairyLight,
CRGB::GreenYellow,
CRGB::Lime,
CRGB::Green,
CRGB::LightGreen,
CRGB::PaleGreen,
CRGB::MediumAquamarine,
CRGB::MediumTurquoise,
CRGB::PaleTurquoise,
CRGB::Cyan,
CRGB::LightCyan,
CRGB::Blue,
CRGB::RoyalBlue,
CRGB::CornflowerBlue,
CRGB::Navy,
CRGB::SlateBlue,
CRGB::BlueViolet,
CRGB::Amethyst,
CRGB::PaleVioletRed,
CRGB::Fuchsia,
CRGB::Magenta,
CRGB::DeepPink,
CRGB::HotPink,
CRGB::LightPink,
CRGB::Beige,
};   //fastled.io/docs/struct_c_r_g_b.html


CRGB leds[NUM_LEDS];

constexpr int WORD_GUTE[2] =   {0, 3};
constexpr int WORD_GUTEN[2] =   {0, 4};

constexpr int WORD_MORGEN[2] =  {6, 11};
constexpr int WORD_ABEND[2] =  {13, 17};
constexpr int WORD_MITTAG[2] = {18, 23};
constexpr int WORD_TAG[2] = {18, 20};

constexpr int WORD_THERESA[2] = { 26, 32 };
constexpr int WORD_HEART[2] = {33, 33};

constexpr int WORD_ES[2] = { 46, 47};
constexpr int WORD_IST[2] = { 42, 44 };

constexpr int WORD_FUENF_PRE[2] = {37, 40};
constexpr int WORD_FUENF_MINUS[2] = {36, 40};
constexpr int WORD_UND[2] = {48, 50};
constexpr int WORD_ZWANZIG[2] = {51, 57};

constexpr int WORD_ZEHN_PRE[2] = {68, 71};
constexpr int WORD_MINUTEN[2] = {60, 66};

constexpr int WORD_VOR[2] = { 72, 74};
constexpr int WORD_NACH[2] = { 75, 78};
constexpr int WORD_HALB[2] = {80, 83};

constexpr int WORD_NULL[2] = {86, 89};
constexpr int WORD_EINS[2] = {116, 119};
constexpr int WORD_EIN[2] = {117, 119};
constexpr int WORD_ZWEI[2] = {112, 115};
constexpr int WORD_DREI[2] = {108, 111};
constexpr int WORD_VIER[2] = {120, 123};
constexpr int WORD_FUENF[2] = {136, 139};
constexpr int WORD_SECHS[2] = {96, 100};
constexpr int WORD_SIEBEN[2] = {100, 105};
constexpr int WORD_ACHT[2] = {140, 144};
constexpr int WORD_NEUN[2] = {89, 92};
constexpr int WORD_ZEHN[2] = {92, 95};          
constexpr int WORD_ELF[2] = {129, 131};
constexpr int WORD_ZWOELF[2] = {124, 128};

constexpr int WORD_UHR[2] = {132, 134};

constexpr int WORD_SETTINGS_ERROR[2] = {36, 47}; 

void setupLed() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(userSetBrightness);

  FastLED.clear();
  FastLED.show();
}

// SETUP RTC:
#define DS1302_CLK_PIN 6
#define DS1302_DAT_PIN 7
#define DS1302_RST_PIN 8

ThreeWire myWire(DS1302_DAT_PIN, DS1302_CLK_PIN, DS1302_RST_PIN);
RtcDS1302<ThreeWire> rtc(myWire);

void setupRtc() {
  rtc.Begin();

  if (rtc.GetIsWriteProtected()) {
    rtc.SetIsWriteProtected(false);
  }

  if (!rtc.GetIsRunning()) {
    rtc.SetIsRunning(true);
  }


  RtcDateTime timeToSet = RtcDateTime(
    2025,
    1,
    10,
    19,
    23,
    10
  );

  rtc.SetDateTime(timeToSet);


}

// SETUP BUTTONS:

#define BUTTON_CHANGE_STATE_PIN 10
#define BUTTON_ACTION_PIN 11

void setupButtons() {
  pinMode(BUTTON_CHANGE_STATE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_ACTION_PIN, INPUT_PULLUP);
}

// returns true, true, if both buttons were released,
// return true, false if only changeState button was 

int buttonChangeState = HIGH;
int buttonAction = HIGH;

bool wasChangeStateButtonReleased() {
  bool buttonChangeStateReleased = false;
  if (buttonChangeState == LOW && digitalRead(BUTTON_CHANGE_STATE_PIN) == HIGH) {
    buttonChangeStateReleased = true;
  }

  buttonChangeState = digitalRead(BUTTON_CHANGE_STATE_PIN);
  return buttonChangeStateReleased;
}

bool wasActionButtonReleased() {
  bool buttonActionReleased = false;
  if (buttonAction == LOW && digitalRead(BUTTON_ACTION_PIN) == HIGH) {
    buttonActionReleased = true;
  }

  buttonAction = digitalRead(BUTTON_ACTION_PIN);
  return buttonActionReleased;
}


// SETUP EEPROM:

#define MEMORY_START 0

void writeToEEPROM() {
    // Write paletteIndex to EEPROM
    EEPROM.write(MEMORY_START, paletteIndex & 0xFF);
    EEPROM.write(MEMORY_START + 1, (paletteIndex >> 8) & 0xFF);
    
    // Write brightness to EEPROM
    EEPROM.write(MEMORY_START + 2, userSetBrightness & 0xFF);
    EEPROM.write(MEMORY_START + 3, (userSetBrightness >> 8) & 0xFF);
}

void readFromEEPROM() {
    paletteIndex = EEPROM.read(MEMORY_START) | (EEPROM.read(MEMORY_START + 1) << 8);
    userSetBrightness = EEPROM.read(MEMORY_START + 2) | (EEPROM.read(MEMORY_START + 3) << 8);
}


void setup() {
  Serial.begin(9600);
  delay(3000); // power-up safety delay

  readFromEEPROM();

  setupLed();
  setupRtc();
  setupButtons();
}


void lightTilesForIdentifier(const int tileDescriptor[3]) {
  for (int i = tileDescriptor[0]; i <= tileDescriptor[1]; i++) {
    leds[i] = palette[paletteIndex];

    if (tileDescriptor == WORD_SETTINGS_ERROR) {
      leds[i] = CRGB::Red;
    }
  }
}


void lightTilesForTime(uint8_t minutes, uint8_t hours) {
  // assert(minutes>=0);
  // assert(minutes<=59);
  // assert(hours>=0);
  // assert(hours<=24);
  // TODO assertions
 
  lightTilesForIdentifier(WORD_ES);
  lightTilesForIdentifier(WORD_IST);

  if (3 <= hours && hours <= 10) {
    lightTilesForIdentifier(WORD_GUTEN);
    lightTilesForIdentifier(WORD_MORGEN);
  } else if (11 <= hours && hours <= 14) {
    lightTilesForIdentifier(WORD_GUTEN);
    lightTilesForIdentifier(WORD_MITTAG);
  } else if (17<= hours && hours <= 24) {
    lightTilesForIdentifier(WORD_GUTEN);
    lightTilesForIdentifier(WORD_ABEND);
  } else {
    lightTilesForIdentifier(WORD_GUTEN);
    lightTilesForIdentifier(WORD_TAG);
  }

  lightTilesForIdentifier(WORD_THERESA);
  lightTilesForIdentifier(WORD_HEART);

  //Color change at 11:11

  if (minutes == 11 && hours == 11) {
    for (int i = 26; i<=33; i++) {
      leds[i] = CRGB::Red;
    }
  }

  if (minutes <= 2) {
    lightTilesForIdentifier(WORD_UHR);
  } else if (3 <= minutes && minutes <= 6) {
    lightTilesForIdentifier(WORD_FUENF_PRE);
    lightTilesForIdentifier(WORD_NACH);
  } else if (7 <= minutes && minutes <= 12) {
    lightTilesForIdentifier(WORD_ZEHN_PRE);
    lightTilesForIdentifier(WORD_NACH);
  } else if (13 <= minutes && minutes <= 16) {
    lightTilesForIdentifier(WORD_FUENF_MINUS);
    lightTilesForIdentifier(WORD_ZEHN_PRE);
    lightTilesForIdentifier(WORD_MINUTEN);
    lightTilesForIdentifier(WORD_NACH);
  } else if (17 <= minutes && minutes <= 22) {
    lightTilesForIdentifier(WORD_ZWANZIG);
    lightTilesForIdentifier(WORD_MINUTEN);
    lightTilesForIdentifier(WORD_NACH);
  } else if (22 <= minutes && minutes <= 26) {
    lightTilesForIdentifier(WORD_FUENF_PRE);
    lightTilesForIdentifier(WORD_VOR);
    lightTilesForIdentifier(WORD_HALB);

    hours += 1;
  } else if (27 <= minutes && minutes <= 32) {
    lightTilesForIdentifier(WORD_HALB);

    hours += 1;
  } else if (33 <= minutes && minutes <= 36) {
    lightTilesForIdentifier(WORD_FUENF_PRE);
    lightTilesForIdentifier(WORD_NACH);
    lightTilesForIdentifier(WORD_HALB);

    hours += 1;
  } else if (37 <= minutes && minutes <= 42) {
    lightTilesForIdentifier(WORD_ZWANZIG);
    lightTilesForIdentifier(WORD_MINUTEN);
    lightTilesForIdentifier(WORD_VOR);

    hours += 1;
  } else if (43 <= minutes && minutes <= 46) {
    lightTilesForIdentifier(WORD_FUENF_MINUS);
    lightTilesForIdentifier(WORD_ZEHN_PRE);
    lightTilesForIdentifier(WORD_MINUTEN);
    lightTilesForIdentifier(WORD_VOR);

    hours += 1;
  } else if (47 <= minutes && minutes <= 52) {
    lightTilesForIdentifier(WORD_ZEHN_PRE);
    lightTilesForIdentifier(WORD_VOR);

    hours += 1;
  } else if (53 <= minutes && minutes <= 57) {
    lightTilesForIdentifier(WORD_FUENF_PRE);
    lightTilesForIdentifier(WORD_VOR);

    hours += 1;
  } else if (minutes >= 58) {
    lightTilesForIdentifier(WORD_UHR);

    hours += 1;
  } else {
    lightTilesForIdentifier(WORD_SETTINGS_ERROR);
  }

  if(hours > 12) {
    hours -= 12;
  }
  
  switch (hours) {
    case 0:
      lightTilesForIdentifier(WORD_NULL);
      break;
      
    case 1:
      if (minutes <= 2 || minutes >= 58) {
        lightTilesForIdentifier(WORD_EIN);
      } else {
        lightTilesForIdentifier(WORD_EINS);
      }
      break;
    
    case 2:
      lightTilesForIdentifier(WORD_ZWEI);
      break;

    case 3:
      lightTilesForIdentifier(WORD_DREI);
      break;

    case 4:
      lightTilesForIdentifier(WORD_VIER);
      break;

    case 5:
      lightTilesForIdentifier(WORD_FUENF);
      break;

    case 6:
      lightTilesForIdentifier(WORD_SECHS);
      break;

    case 7:
      lightTilesForIdentifier(WORD_SIEBEN);
      break;
      
    case 8:
      lightTilesForIdentifier(WORD_ACHT);
      break;

    case 9:
      lightTilesForIdentifier(WORD_NEUN);
      break;

    case 10:
      lightTilesForIdentifier(WORD_ZEHN);
      break;

    case 11:
      lightTilesForIdentifier(WORD_ELF);
      break;

    case 12:
      lightTilesForIdentifier(WORD_ZWOELF);
      break;
    
    default:
      lightTilesForIdentifier(WORD_SETTINGS_ERROR);
      break;
  }
}

void dimLed() {
  FastLED.setBrightness(6);
  FastLED.show();
  FastLED.clear(true);
  for(int i = 0; i<6; i++) {
    FastLED.clear();
  }
}

void loop() {
  RtcDateTime now = rtc.GetDateTime();

  if (wasChangeStateButtonReleased()) {
    globalClockState = (globalClockState + 1) % 6;
  }

  bool actionOccured = wasActionButtonReleased();

  FastLED.clear();

  switch (globalClockState) {
    case SHOW_TIME:
      lightTilesForTime(now.Minute(), now.Hour());      

      if (now.Hour() >= 22 || now.Hour() < 7) {
        dimLed();
      } else {
        FastLED.setBrightness(userSetBrightness);
        delay(2000);
      }
      break;
      
    case SET_COLOR:
      if (actionOccured) {
        leds[1] = CRGB::Black; // For some really fckn weird reason the clock does not start blinking at 19:22:15 when this line of code is added wtf... wtf... this is the weirdst bug I've ever faced in all my life.

        paletteIndex = (paletteIndex + 1) % NUM_COLORS;

        writeToEEPROM();
      }

      // Light Tiles to get the word "FARBE":
      leds[12] = palette[paletteIndex]; // F
      leds[32] = palette[paletteIndex]; // A
      leds[74] = palette[paletteIndex]; // R 
      leds[103] = palette[paletteIndex];// B
      leds[109] = palette[paletteIndex];// E
      break;

    case SET_BRIGHTNESS:
      if (actionOccured) {
        userSetBrightness = (userSetBrightness + 10) % MAX_BRIGHTNESS;
        if (userSetBrightness <= 0) {
          userSetBrightness += 10;
        }

        writeToEEPROM();
        FastLED.setBrightness(userSetBrightness);
      }

      // Light Tiles to get word "HELL":
      leds[93] = palette[paletteIndex];
      leds[91] = palette[paletteIndex];
      leds[86] = palette[paletteIndex];
      leds[87] = palette[paletteIndex];

      lightTilesForIdentifier(WORD_THERESA);
      lightTilesForIdentifier(WORD_HEART);

      break;

    case SET_TIME_HOURS:
      if (actionOccured) {
        now += 3600;
        rtc.SetDateTime(now);
      }

      lightTilesForTime(now.Minute(), now.Hour());

      // Ensure only the hour-section is lid
      for (int i = 0; i <= 83; i++) {
        leds[i] = CRGB::Black;
      }

      // Light tiles for word "STUNDE":
      leds[42] = CRGB::Blue; // S
      leds[43] = CRGB::Blue; // T
      leds[48] = CRGB::Blue; // U
      leds[49] = CRGB::Blue; // N
      leds[50] = CRGB::Blue; // D
      leds[70] = CRGB::Blue; // E

      break;

    case SET_TIME_MINUTES:
      if (actionOccured) {
        now += 150;
        rtc.SetDateTime(now);
      }

      lightTilesForTime(now.Minute(), now.Hour());

      // Ensure only the minute-section is lid:
      for (int i = 0; i <= 35; i++) {
        leds[i] = CRGB::Black;
      }

      // Light tiles for word "MINUTEN"
      for (int i = WORD_MINUTEN[0]; i <= WORD_MINUTEN[1]; i++) {
        leds[i] = CRGB::Blue;
      }

      break;

    case TEST:
      for(int ledIndex = 0; ledIndex<144; ledIndex++) {
        leds[ledIndex] = palette[ledIndex % NUM_COLORS];
        FastLED.show();
      }

      for(int ledIndex = 0; ledIndex < 144; ledIndex++) {
        leds[ledIndex] = CRGB::Black;
        FastLED.show();
      }

      delay(500);

      globalClockState = SHOW_TIME;

      break;

    default:


      // The 7 or 8 hour-bug occurs. No idea why the fuck this happens. might need to rely on the arduinos internal clock for this, maybe some strange workaround for that dont know yet. ignored for know.
      
      // globalClockState = SHOW_TIME;

      leds[0] = CRGB::Red;
      FastLED.show();
      // delay(1000);

      // Solve this by somehow making the Arduino rely on its own time whenever we end up here.   
      break;
  }

  FastLED.show();
}
