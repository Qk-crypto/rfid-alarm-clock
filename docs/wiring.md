# Wiring Guide

## 16x2 I2C LCD

| LCD pin | Arduino Uno |
|---|---|
| GND | GND |
| VCC | 5V |
| SDA | A4 |
| SCL | A5 |

## DS1302 RTC

| DS1302 pin | Arduino Uno |
|---|---|
| VCC | 5V |
| GND | GND |
| CLK / SCLK | D4 |
| DAT / IO | D5 |
| RST / CE | D6 |

## RFID RC522

| RC522 pin | Arduino Uno |
|---|---|
| SDA / SS | D10 |
| SCK | D13 |
| MOSI | D11 |
| MISO | D12 |
| RST | D9 |
| 3.3V | 3.3V |
| GND | GND |

Important: RC522 must use 3.3V power.

## HW-504 Joystick

| Joystick pin | Arduino Uno |
|---|---|
| VRx | A0 |
| VRy | A1 |
| SW | D2 |
| +5V | 5V |
| GND | GND |

## Buzzer Module

| Buzzer pin | Arduino Uno |
|---|---|
| S | D3 |
| + | 5V |
| - | GND |
