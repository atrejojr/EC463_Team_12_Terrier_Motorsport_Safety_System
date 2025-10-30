// Justin Nascimento, Version 1 of CAN Transmitter Firmware
#include <ESP32-TWAI-CAN.hpp>

// Default for ESP32
#define CAN_TX 21
#define CAN_RX 22

// Variable to increment so we know messages are being sent
int i = 0;

// Function to set the CAN bus message
void sendObdFrame(uint8_t obdId) {
    CanFrame obdFrame         = {0};
    obdFrame.identifier       = 0x7DF; // Default OBD2 address;
    obdFrame.extd             = 0;
    obdFrame.data_length_code = 8;
    obdFrame.data[0]          = 0xAA;
    obdFrame.data[1]          = 0xCC;
    obdFrame.data[2]          = obdId;
    obdFrame.data[3]          = 0x33; // Best use 0xAA (0b10101010) instead of 0
    obdFrame.data[4]          = 0xAA; // TWAI / CAN works better this way, as it
    obdFrame.data[5]          = 0xAA; // needs to avoid bit-stuffing
    obdFrame.data[6]          = 0xAA;
    obdFrame.data[7]          = 0xAA;
    // Accepts both pointers and references
    ESP32Can.writeFrame(obdFrame); // timeout defaults to 1 ms
}

void setup() {
  // Setup serial for debugging.
    Serial.begin(115200);

    // Set pins
    ESP32Can.setPins(CAN_TX, CAN_RX);

    // I upped the size of the buffers from the default
    ESP32Can.setRxQueueSize(10);
    ESP32Can.setTxQueueSize(10);

    // .setSpeed() and .begin() functions require to use TwaiSpeed enum,
    // but you can easily convert it from numerical value using .convertSpeed()
    ESP32Can.setSpeed(ESP32Can.convertSpeed(500));

    // You can also just use .begin()..
    if(ESP32Can.begin()) {
        Serial.println("CAN bus started!");
    } else {
        Serial.println("CAN bus failed!");
    }

}

void loop() {
    // Code to send data indefinitely 
    static uint32_t lastStamp    = 0;
    uint32_t        currentStamp = millis();

    i = (i+1) % 256;    // Increments i, wrapping it around back to 0 once it hits 256.

    if(currentStamp - lastStamp > 1000) { // sends OBD2 request every second
        lastStamp = currentStamp;
        sendObdFrame(i); // For coolant temperature
    }
}
