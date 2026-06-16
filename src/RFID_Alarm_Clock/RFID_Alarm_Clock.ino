#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

// ===================== LCD =====================
LiquidCrystal_I2C lcd(0x27, 16, 2);
// If LCD does not work, try changing 0x27 to 0x3F.

// ===================== DS1302 RTC =====================
// CLK/SCLK -> D4
// DAT/IO   -> D5
// RST/CE   -> D6
#define DS1302_CLK 4
#define DS1302_DAT 5
#define DS1302_RST 6

ThreeWire myWire(DS1302_DAT, DS1302_CLK, DS1302_RST);
RtcDS1302<ThreeWire> Rtc(myWire);

// Set true once to update RTC time from compile time.
// After uploading once, set it back to false and upload again.
const bool FORCE_SET_RTC_TIME_ON_UPLOAD = false;

// ===================== RFID RC522 =====================
#define RFID_SS_PIN 10
#define RFID_RST_PIN 9
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// ===================== Joystick =====================
#define JOY_X A0
#define JOY_Y A1
#define JOY_SW 2

#define JOY_LOW 250
#define JOY_HIGH 750
#define JOY_DELAY 220

// ===================== Buzzer =====================
#define BUZZER_PIN 3

// ===================== Notes =====================
#define REST     0

#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988

#define NOTE_C6  1047
#define NOTE_D6  1175
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_G6  1568
#define NOTE_A6  1760
#define NOTE_B6  1976

#define NOTE_C7  2093
#define NOTE_D7  2349
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_G7  3136

// ===================== Alarm melody =====================
// Custom alarm melody. Higher notes are louder on many small buzzers.
int alarmMelody[] = {
  NOTE_C7, NOTE_E7, NOTE_G7, REST,
  NOTE_G7, NOTE_E7, NOTE_C7, REST,

  NOTE_D7, NOTE_F7, NOTE_G7, REST,
  NOTE_G7, NOTE_F7, NOTE_D7, REST,

  NOTE_E7, NOTE_G7, NOTE_E7, NOTE_C7,
  NOTE_D7, NOTE_F7, NOTE_D7, NOTE_B6,

  NOTE_C7, REST, NOTE_C7, REST
};

// 8 = short, 4 = medium, 2 = long
int alarmDurations[] = {
  8, 8, 4, 12,
  8, 8, 4, 12,

  8, 8, 4, 12,
  8, 8, 4, 12,

  8, 8, 8, 8,
  8, 8, 8, 8,

  4, 12, 4, 12
};

const int MELODY_LENGTH = sizeof(alarmMelody) / sizeof(alarmMelody[0]);

int melodyIndex = 0;
unsigned long melodyNoteStart = 0;
unsigned long melodyNoteDuration = 0;
bool melodyRunning = false;

// ===================== Alarm settings =====================
int alarmHour = 7;
int alarmMinute = 30;
int alarmSecond = 0;
bool alarmEnabled = false;

// Main mode: 1 minute melody, 30 seconds pause, repeated 3 times.
const unsigned long ALARM_BEEP_MS = 60000;
const unsigned long ALARM_PAUSE_MS = 30000;

// For quick testing, temporarily use:
// const unsigned long ALARM_BEEP_MS = 10000;
// const unsigned long ALARM_PAUSE_MS = 5000;

// ===================== UI states =====================
enum ScreenMode {
  SCREEN_MAIN,
  SCREEN_MENU,
  SCREEN_SET_ALARM
};

ScreenMode screenMode = SCREEN_MAIN;

int menuIndex = 0;
int editField = 0;

int editHour = 7;
int editMinute = 30;
int editSecond = 0;

// ===================== Alarm states =====================
enum AlarmRunState {
  ALARM_IDLE,
  ALARM_BEEPING,
  ALARM_PAUSE
};

AlarmRunState alarmState = ALARM_IDLE;

unsigned long alarmPhaseStart = 0;
int alarmRound = 0;

int lastTriggeredDate = 0;

unsigned long messageUntil = 0;
char messageLine1[17] = "";
char messageLine2[17] = "";

// ===================== LCD timing =====================
unsigned long lastLcdUpdate = 0;
const unsigned long LCD_UPDATE_MS = 250;

// ===================== Joystick event =====================
enum JoyEvent {
  JOY_NONE,
  JOY_UP,
  JOY_DOWN,
  JOY_LEFT,
  JOY_RIGHT,
  JOY_PRESS
};

JoyEvent readJoystick() {
  static unsigned long lastJoyTime = 0;

  if (millis() - lastJoyTime < JOY_DELAY) {
    return JOY_NONE;
  }

  if (digitalRead(JOY_SW) == LOW) {
    lastJoyTime = millis();
    return JOY_PRESS;
  }

  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);

  if (x < JOY_LOW) {
    lastJoyTime = millis();
    return JOY_LEFT;
  }

  if (x > JOY_HIGH) {
    lastJoyTime = millis();
    return JOY_RIGHT;
  }

  if (y < JOY_LOW) {
    lastJoyTime = millis();
    return JOY_UP;
  }

  if (y > JOY_HIGH) {
    lastJoyTime = millis();
    return JOY_DOWN;
  }

  return JOY_NONE;
}

// ===================== Helpers =====================
int makeDateCode(const RtcDateTime& now) {
  return now.Year() * 10000 + now.Month() * 100 + now.Day();
}

void stopAlarmSound() {
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);

  melodyIndex = 0;
  melodyNoteStart = 0;
  melodyNoteDuration = 0;
  melodyRunning = false;
}

void showMessage(const char* line1, const char* line2, unsigned long durationMs) {
  strncpy(messageLine1, line1, 16);
  strncpy(messageLine2, line2, 16);

  messageLine1[16] = '\0';
  messageLine2[16] = '\0';

  messageUntil = millis() + durationMs;
}

// ===================== Melody player =====================
void resetMelody() {
  noTone(BUZZER_PIN);

  melodyIndex = 0;
  melodyNoteStart = 0;
  melodyNoteDuration = 0;
  melodyRunning = false;
}

void updateAlarmMelody() {
  unsigned long nowMs = millis();

  if (!melodyRunning || nowMs - melodyNoteStart >= melodyNoteDuration) {
    int note = alarmMelody[melodyIndex];
    int divider = alarmDurations[melodyIndex];

    int duration = 1000 / divider;

    melodyNoteStart = nowMs;
    melodyNoteDuration = duration;
    melodyRunning = true;

    if (note == REST) {
      noTone(BUZZER_PIN);
    } else {
      tone(BUZZER_PIN, note);
    }

    melodyIndex++;

    if (melodyIndex >= MELODY_LENGTH) {
      melodyIndex = 0;
    }
  }
}

// ===================== RFID =====================
bool isAnyRfidCardPresent() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return false;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return false;
  }

  Serial.print("RFID UID: ");

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }

  Serial.println();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  return true;
}

// ===================== Alarm control =====================
void startAlarm(const RtcDateTime& now) {
  alarmState = ALARM_BEEPING;
  alarmRound = 1;
  alarmPhaseStart = millis();

  resetMelody();

  lastTriggeredDate = makeDateCode(now);

  screenMode = SCREEN_MAIN;

  Serial.println("ALARM STARTED");
}

void stopAlarmByRfid() {
  stopAlarmSound();

  alarmState = ALARM_IDLE;
  alarmRound = 0;

  showMessage("Alarm stopped", "RFID accepted", 2500);

  Serial.println("ALARM STOPPED BY RFID");
}

void finishAlarmWithoutCard() {
  stopAlarmSound();

  alarmState = ALARM_IDLE;
  alarmRound = 0;

  showMessage("Missed alarm", "No RFID scan", 4000);

  Serial.println("ALARM FINISHED WITHOUT RFID");
}

void checkAlarmTime(const RtcDateTime& now) {
  if (!alarmEnabled) return;
  if (alarmState != ALARM_IDLE) return;

  int todayCode = makeDateCode(now);

  if (todayCode == lastTriggeredDate) {
    return;
  }

  if (now.Hour() == alarmHour &&
      now.Minute() == alarmMinute &&
      now.Second() == alarmSecond) {
    startAlarm(now);
  }
}

void runAlarmState() {
  if (alarmState == ALARM_IDLE) {
    return;
  }

  if (isAnyRfidCardPresent()) {
    stopAlarmByRfid();
    return;
  }

  unsigned long nowMs = millis();

  if (alarmState == ALARM_BEEPING) {
    updateAlarmMelody();

    if (nowMs - alarmPhaseStart >= ALARM_BEEP_MS) {
      stopAlarmSound();

      if (alarmRound >= 3) {
        finishAlarmWithoutCard();
      } else {
        alarmState = ALARM_PAUSE;
        alarmPhaseStart = nowMs;

        Serial.println("ALARM PAUSE");
      }
    }
  }

  else if (alarmState == ALARM_PAUSE) {
    stopAlarmSound();

    if (nowMs - alarmPhaseStart >= ALARM_PAUSE_MS) {
      alarmRound++;
      alarmState = ALARM_BEEPING;
      alarmPhaseStart = nowMs;

      resetMelody();

      Serial.print("ALARM ROUND ");
      Serial.println(alarmRound);
    }
  }
}

// ===================== Joystick UI =====================
void handleJoystick(JoyEvent ev) {
  if (ev == JOY_NONE) return;

  if (alarmState == ALARM_BEEPING || alarmState == ALARM_PAUSE) {
    return;
  }

  if (screenMode == SCREEN_MAIN) {
    if (ev == JOY_PRESS) {
      screenMode = SCREEN_MENU;
      menuIndex = 0;
    }
  }

  else if (screenMode == SCREEN_MENU) {
    if (ev == JOY_UP || ev == JOY_DOWN) {
      menuIndex = 1 - menuIndex;
    }

    if (ev == JOY_LEFT) {
      screenMode = SCREEN_MAIN;
    }

    if (ev == JOY_PRESS) {
      if (menuIndex == 0) {
        editHour = alarmHour;
        editMinute = alarmMinute;
        editSecond = alarmSecond;
        editField = 0;

        screenMode = SCREEN_SET_ALARM;
      }

      else if (menuIndex == 1) {
        alarmEnabled = !alarmEnabled;

        showMessage(alarmEnabled ? "Alarm ON" : "Alarm OFF", "Saved", 1500);

        screenMode = SCREEN_MAIN;
      }
    }
  }

  else if (screenMode == SCREEN_SET_ALARM) {
    if (ev == JOY_RIGHT) {
      editField++;

      if (editField > 2) {
        editField = 0;
      }
    }

    if (ev == JOY_LEFT) {
      editField--;

      if (editField < 0) {
        editField = 2;
      }
    }

    if (ev == JOY_UP) {
      if (editField == 0) {
        editHour++;

        if (editHour > 23) {
          editHour = 0;
        }
      }

      else if (editField == 1) {
        editMinute++;

        if (editMinute > 59) {
          editMinute = 0;
        }
      }

      else if (editField == 2) {
        editSecond++;

        if (editSecond > 59) {
          editSecond = 0;
        }
      }
    }

    if (ev == JOY_DOWN) {
      if (editField == 0) {
        editHour--;

        if (editHour < 0) {
          editHour = 23;
        }
      }

      else if (editField == 1) {
        editMinute--;

        if (editMinute < 0) {
          editMinute = 59;
        }
      }

      else if (editField == 2) {
        editSecond--;

        if (editSecond < 0) {
          editSecond = 59;
        }
      }
    }

    if (ev == JOY_PRESS) {
      alarmHour = editHour;
      alarmMinute = editMinute;
      alarmSecond = editSecond;

      alarmEnabled = true;

      lastTriggeredDate = 0;

      showMessage("Alarm saved", "Enabled", 2000);

      screenMode = SCREEN_MAIN;
    }
  }
}

// ===================== LCD =====================
void printPadded(const char* text) {
  int len = strlen(text);

  lcd.print(text);

  for (int i = len; i < 16; i++) {
    lcd.print(" ");
  }
}

void updateLcd(const RtcDateTime& now) {
  if (millis() - lastLcdUpdate < LCD_UPDATE_MS) {
    return;
  }

  lastLcdUpdate = millis();

  char line1[17];
  char line2[17];

  if (millis() < messageUntil) {
    lcd.setCursor(0, 0);
    printPadded(messageLine1);

    lcd.setCursor(0, 1);
    printPadded(messageLine2);

    return;
  }

  if (alarmState == ALARM_BEEPING) {
    snprintf(line1, sizeof(line1), "ALARM! Round %d", alarmRound);
    snprintf(line2, sizeof(line2), "Scan RFID card");

    lcd.setCursor(0, 0);
    printPadded(line1);

    lcd.setCursor(0, 1);
    printPadded(line2);

    return;
  }

  if (alarmState == ALARM_PAUSE) {
    unsigned long elapsed = millis() - alarmPhaseStart;
    unsigned long remainMs = 0;

    if (elapsed < ALARM_PAUSE_MS) {
      remainMs = ALARM_PAUSE_MS - elapsed;
    }

    int remainSec = remainMs / 1000;

    snprintf(line1, sizeof(line1), "Pause before %d", alarmRound + 1);
    snprintf(line2, sizeof(line2), "Wait %02d sec", remainSec);

    lcd.setCursor(0, 0);
    printPadded(line1);

    lcd.setCursor(0, 1);
    printPadded(line2);

    return;
  }

  if (screenMode == SCREEN_MAIN) {
    snprintf(line1, sizeof(line1), "Time %02u:%02u:%02u",
             now.Hour(), now.Minute(), now.Second());

    snprintf(line2, sizeof(line2), "A %02d:%02d:%02d %s",
             alarmHour, alarmMinute, alarmSecond,
             alarmEnabled ? "ON" : "OFF");

    lcd.setCursor(0, 0);
    printPadded(line1);

    lcd.setCursor(0, 1);
    printPadded(line2);
  }

  else if (screenMode == SCREEN_MENU) {
    if (menuIndex == 0) {
      strcpy(line1, ">Set Alarm");
      strcpy(line2, " Alarm ON/OFF");
    } else {
      strcpy(line1, " Set Alarm");
      strcpy(line2, ">Alarm ON/OFF");
    }

    lcd.setCursor(0, 0);
    printPadded(line1);

    lcd.setCursor(0, 1);
    printPadded(line2);
  }

  else if (screenMode == SCREEN_SET_ALARM) {
    strcpy(line1, "Set Alarm H/M/S");

    const char* fieldName;

    if (editField == 0) {
      fieldName = "HH";
    } else if (editField == 1) {
      fieldName = "MM";
    } else {
      fieldName = "SS";
    }

    snprintf(line2, sizeof(line2), "%02d:%02d:%02d     %s",
             editHour, editMinute, editSecond, fieldName);

    lcd.setCursor(0, 0);
    printPadded(line1);

    lcd.setCursor(0, 1);
    printPadded(line2);
  }
}

// ===================== RTC setup =====================
void setupRtc() {
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (Rtc.GetIsWriteProtected()) {
    Serial.println("RTC write protected. Disabling...");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not running. Starting...");
    Rtc.SetIsRunning(true);
  }

  if (FORCE_SET_RTC_TIME_ON_UPLOAD || !Rtc.IsDateTimeValid()) {
    Serial.println("Setting RTC time to compile time...");
    Rtc.SetDateTime(compiled);
  }

  RtcDateTime now = Rtc.GetDateTime();

  Serial.print("RTC time: ");
  Serial.print(now.Hour());
  Serial.print(":");
  Serial.print(now.Minute());
  Serial.print(":");
  Serial.println(now.Second());
}

// ===================== Setup =====================
void setup() {
  Serial.begin(9600);

  pinMode(BUZZER_PIN, OUTPUT);
  stopAlarmSound();

  pinMode(JOY_SW, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Alarm Project");

  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  setupRtc();

  SPI.begin();
  rfid.PCD_Init();

  delay(1200);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("System Ready");

  lcd.setCursor(0, 1);
  lcd.print("Press joystick");

  delay(1200);
}

// ===================== Loop =====================
void loop() {
  RtcDateTime now = Rtc.GetDateTime();

  JoyEvent ev = readJoystick();

  handleJoystick(ev);
  checkAlarmTime(now);
  runAlarmState();
  updateLcd(now);
}
