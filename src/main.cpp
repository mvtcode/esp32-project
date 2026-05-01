#include <Arduino.h>
#include "hardware_config.h"
#include "ui_manager.h"
#include "voice_manager.h"

// List of controlled GPIOs
const int control_pins[] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4, RELAY_5, RELAY_6};
const int num_pins = sizeof(control_pins) / sizeof(control_pins[0]);

// Command Handler
void handle_voice_command(int command_id) {
    Serial.printf("Received Command ID: %d\n", command_id);
    
    // Example: Command IDs 0-5 turn ON, 10-15 turn OFF
    if (command_id >= 0 && command_id < num_pins) {
        digitalWrite(control_pins[command_id], HIGH);
        ui_show_message("Device ON");
    } else if (command_id >= 10 && command_id < 10 + num_pins) {
        digitalWrite(control_pins[command_id - 10], LOW);
        ui_show_message("Device OFF");
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialize GPIOs
    for (int i = 0; i < num_pins; i++) {
        pinMode(control_pins[i], OUTPUT);
        digitalWrite(control_pins[i], LOW);
    }
    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, HIGH); // Status LED ON

    // Initialize UI
    ui_init();
    ui_show_message("Initializing...");

    // Initialize Voice
    voice_init();
    voice_set_command_callback(handle_voice_command);

    ui_show_message("Ready: Say Command");
    Serial.println("System Ready");
}

void loop() {
    ui_update();
    voice_update();
    
    // Check buttons
    if (digitalRead(BTN_WAKE) == LOW) {
        ui_show_message("Listening...");
        // In real ESP-SR, this would trigger a manual wake
        delay(200); 
    }

    delay(5);
}