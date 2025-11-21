// Justin Nascimento, Version 1 of CAN Transmitter Firmware
#include <ESP32-TWAI-CAN.hpp>

// Default for ESP32
#define CAN_TX 21
#define CAN_RX 22
#define buttonPin 16
#define AMSLatchPin 17
#define IMDLatchPin 18
#define CANWatchdogPin 19
#define VehichleStartPin 23

// Variable to increment so we know messages are being sent
uint8_t i = 0;

// Function to set the CAN bus message
void sendObdFrame(uint8_t obdId, uint8_t buttonValue, uint8_t AMSLatchValue, uint8_t IMDLatchValue, uint8_t CANWatchdogValue, uint8_t VechicleStartValue){
    CanFrame obdFrame         = {0};
    obdFrame.identifier       = 0x7DF; // Default OBD2 address;
    obdFrame.extd             = 0;
    obdFrame.data_length_code = 8;
    obdFrame.data[0]          = 0xCC;
    obdFrame.data[1]          = buttonValue;
    obdFrame.data[2]          = AMSLatchValue;
    obdFrame.data[3]          = IMDLatchValue; // Best use 0xAA (0b10101010) instead of 0
    obdFrame.data[4]          = CANWatchdogValue; // TWAI / CAN works better this way, as it
    obdFrame.data[5]          = VechicleStartValue; // needs to avoid bit-stuffing
    obdFrame.data[6]          = 0xAA;
    obdFrame.data[7]          = 0xCC;
    // Accepts both pointers and references
    ESP32Can.writeFrame(obdFrame); // timeout defaults to 1 ms
}

void sendHeartBeatFrame(uint8_t obdId, CanFrame receivedFrame) {
    CanFrame heartBeatFrame         = {0};
    heartBeatFrame.identifier       = obdId; // Default OBD2 address;
    heartBeatFrame.extd             = 0;
    heartBeatFrame.data_length_code = receivedFrame.data_length_code;
    heartBeatFrame.data[0]          = receivedFrame.data[0];
    heartBeatFrame.data[1]          = receivedFrame.data[1];
    heartBeatFrame.data[2]          = receivedFrame.data[2];
    heartBeatFrame.data[3]          = receivedFrame.data[3];
    heartBeatFrame.data[4]          = receivedFrame.data[4];
    heartBeatFrame.data[5]          = receivedFrame.data[5];
    heartBeatFrame.data[6]          = receivedFrame.data[6];
    heartBeatFrame.data[7]          = receivedFrame.data[7];
    // Accepts both pointers and references
    ESP32Can.writeFrame(heartBeatFrame); // timeout defaults to 1 ms
}

// Gets the status value of a pinNumber
uint8_t getStatusValue(int pinNumber){
    uint8_t status;
    bool temp = digitalRead(pinNumber);
    if(temp == HIGH){
        status = 0xAA;
    }
    else{
        status = 0x55;
    }
    return status;
}

void setup() {
  // Setup serial for debugging.
    Serial.begin(115200);

    Serial.println("Start!");

    // Set pins
    pinMode(buttonPin,INPUT_PULLUP);
    pinMode(AMSLatchPin,INPUT);
    pinMode(IMDLatchPin,INPUT);
    pinMode(CANWatchdogPin,INPUT);
    pinMode(VehichleStartPin,INPUT);
    ESP32Can.setPins(CAN_TX, CAN_RX);

    // I upped the size of the buffers from the default
    ESP32Can.setRxQueueSize(10);
    ESP32Can.setTxQueueSize(10);

    // .setSpeed() and .begin() functions require to use TwaiSpeed enum,
    // but you can easily convert it from numerical value using .convertSpeed()
    ESP32Can.setSpeed(ESP32Can.convertSpeed(250));

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
    uint8_t         buttonValue = 0;
    uint8_t         AMSLatchValue;
    uint8_t         IMDLatchValue;
    uint8_t         CANWatchdogValue;
    uint8_t         VechicleStartValue;
    // Code to echo the Heartbeat message for the CAN Watchdog
    CanFrame receivedFrame;

    // Checks button press, setting the value to whatever appropriate
    buttonValue = getStatusValue(buttonPin);
    AMSLatchValue = getStatusValue(AMSLatchPin);
    IMDLatchValue = getStatusValue(IMDLatchPin);
    CANWatchdogValue = getStatusValue(CANWatchdogPin);
    VechicleStartValue = getStatusValue(VehichleStartPin);

    if(currentStamp - lastStamp > 100) { // sends OBD2 request every half second
        lastStamp = currentStamp;
        i = (i+1) % 256;    // Increments i, wrapping it around back to 0 once it hits 256.
        sendObdFrame(i, buttonValue, AMSLatchValue, IMDLatchValue, CANWatchdogValue, VechicleStartValue); // For coolant temperature

        // Check if the received message is the CAN Heartbeat, and if it is, echo it
        if (ESP32Can.readFrame(&receivedFrame)) {
            Serial.print("ID: 0x");
            Serial.print(receivedFrame.identifier, HEX);
            if(receivedFrame.identifier == 0x123){
                sendHeartBeatFrame(0x123,receivedFrame);
            }
        } 
    }

}
