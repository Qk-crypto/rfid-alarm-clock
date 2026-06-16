# RFID Alarm Clock with Joystick Menu

A compact Arduino alarm clock that displays real time on a 16x2 LCD, lets the user set an alarm using a joystick, plays a melody through a buzzer, and can only be stopped by scanning an RFID card.

## Features

- Real-time clock display with seconds
- 16x2 I2C LCD interface
- Joystick-controlled menu
- Alarm time setting: hours, minutes, seconds
- Melody alarm using a buzzer module
- Alarm repeats 3 times:
  - 60 seconds sound
  - 30 seconds pause
  - 60 seconds sound
  - 30 seconds pause
  - 60 seconds sound
- RFID card required to stop the alarm
- Non-blocking alarm logic using `millis()`
- DS1302 RTC support, so time can continue running after power loss if the RTC battery is installed

## Project idea

The project is designed as a simple but functional smart alarm clock.  
Unlike a normal alarm clock, the user cannot stop the alarm with a button.  
To stop it, the user must scan an RFID card.

This makes the project more interactive and demonstrates:

- embedded UI design
- RTC timekeeping
- RFID authentication
- non-blocking timing logic
- buzzer melody generation
- joystick input handling

## Components

| Component | Quantity |
|---|---:|
| Arduino Uno | 1 |
| 16x2 I2C LCD | 1 |
| DS1302 RTC module | 1 |
| RFID RC522 module | 1 |
| HW-504 joystick module | 1 |
| 3-pin buzzer module | 1 |
| RFID card or tag | 1+ |
| Jumper wires | several |

## Wiring

### 16x2 I2C LCD

| LCD pin | Arduino Uno |
|---|---|
| GND | GND |
| VCC | 5V |
| SDA | A4 |
| SCL | A5 |

### DS1302 RTC

| DS1302 pin | Arduino Uno |
|---|---|
| VCC | 5V |
| GND | GND |
| CLK / SCLK | D4 |
| DAT / IO | D5 |
| RST / CE | D6 |

### RFID RC522

| RC522 pin | Arduino Uno |
|---|---|
| SDA / SS | D10 |
| SCK | D13 |
| MOSI | D11 |
| MISO | D12 |
| RST | D9 |
| 3.3V | 3.3V |
| GND | GND |

> Warning: RC522 must be powered from 3.3V, not 5V.

### HW-504 Joystick

| Joystick pin | Arduino Uno |
|---|---|
| VRx | A0 |
| VRy | A1 |
| SW | D2 |
| +5V | 5V |
| GND | GND |

### Buzzer module

| Buzzer pin | Arduino Uno |
|---|---|
| S | D3 |
| + | 5V |
| - | GND |

## Required Arduino libraries

Install these libraries from Arduino IDE Library Manager:

- `LiquidCrystal I2C`
- `MFRC522`
- `Rtc by Makuna`

## How to use

1. Connect all modules according to the wiring table.
2. Open `src/RFID_Alarm_Clock/RFID_Alarm_Clock.ino` in Arduino IDE.
3. Select:
   - Board: `Arduino Uno`
   - Port: your Arduino port
4. Upload the code.
5. If the LCD does not show text, try changing the LCD address:
   ```cpp
   LiquidCrystal_I2C lcd(0x27, 16, 2);
   ```
   to:
   ```cpp
   LiquidCrystal_I2C lcd(0x3F, 16, 2);
   ```

## Setting RTC time

If the RTC time is incorrect, change this line:

```cpp
const bool FORCE_SET_RTC_TIME_ON_UPLOAD = false;
```

to:

```cpp
const bool FORCE_SET_RTC_TIME_ON_UPLOAD = true;
```

Upload the code once.  
Then change it back to:

```cpp
const bool FORCE_SET_RTC_TIME_ON_UPLOAD = false;
```

and upload again.

This prevents the RTC from resetting to the compile time every time the Arduino restarts.

## Controls

| Joystick action | Function |
|---|---|
| Press | Open menu / select / save |
| Up | Increase selected value |
| Down | Decrease selected value |
| Right | Move to next field |
| Left | Move to previous field / exit menu |

## Menu

Main screen:

```text
Time 14:25:08
A 07:30:00 OFF
```

Menu screen:

```text
>Set Alarm
 Alarm ON/OFF
```

Alarm setting screen:

```text
Set Alarm H/M/S
07:30:00     HH
```

Alarm screen:

```text
ALARM! Round 1
Scan RFID card
```

## Future improvements

- Stop alarm only with a specific authorized RFID card UID
- Add multiple alarm times
- Save alarm time to EEPROM
- Add a 3D-printed enclosure
- Add a louder external buzzer or mini siren using a transistor module
- Add a battery backup for the whole device
- Add a sleep mode to reduce power consumption

## Repository structure

```text
rfid-alarm-clock/
├── README.md
├── LICENSE
├── .gitignore
├── src/
│   └── RFID_Alarm_Clock/
│       └── RFID_Alarm_Clock.ino
└── docs/
    ├── wiring.md
    └── libraries.md
```

## Author

Created by Islam Yekiya.
