#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// OLED I2C Pins (SH1106)
#define I2C_SDA 8
#define I2C_SCL 9

// I2S Microphone Pins (INMP441)
#define I2S_WS 42
#define I2S_SD 2
#define I2S_SCK 41

// Buttons
#define BTN_UP    10
#define BTN_DOWN  11
#define BTN_ENTER 12
#define BTN_BACK  13
#define BTN_WAKE  0

// LED
#define LED_STATUS 48 // Common RGB LED on S3 Devkits or Built-in

// GPIOs to control (Relays/LEDs)
#define RELAY_1 1
#define RELAY_2 3
#define RELAY_3 4
#define RELAY_4 5
#define RELAY_5 6
#define RELAY_6 7

#endif
