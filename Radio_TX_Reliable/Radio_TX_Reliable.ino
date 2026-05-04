// Hannah Tandang and Justin Nascimento 

#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

#include <Crypto.h>
#include <SHA256.h>
#include <Hash.h>

/************ Radio Setup ***************/
#define RF69_FREQ 915.0

#define RFM69_CS    4
#define RFM69_INT   3
#define RFM69_RST   2

// Defines Radio Status Output
#define Radio_Status 5

// Reliable Datagram addresses
#define MY_ADDRESS    2
#define DEST_ADDRESS  1

RH_RF69 rf69(RFM69_CS, RFM69_INT);
RHReliableDatagram manager(rf69, MY_ADDRESS);

/************ GPIO Voltage Inputs ************/
const uint8_t NUM_PINS = 5;
const uint8_t sensePins[NUM_PINS] = {A0, A1, A2, A3, A4};

const float VREF = 5.0f;
const float THRESHOLD_V = 1.5f;

// Track last status so we only send on change
bool lastFault[NUM_PINS] = {false, false, false, false, false};

// Signature Variable
SHA256 hash;

// Counter to prevent Replay (like a timestamp)
uint32_t messageCounter = 0;

// Set Encryption Key
// Scarlett 2020 /n/n/n/n
  uint8_t key[] = {
    0x53, 0x63, 0x61, 0x72,
    0x6C, 0x65, 0x74, 0x74,
    0x32, 0x30, 0x32, 0x30,
    0x0A, 0x0A, 0x0A, 0x0A
  };

// Set Signature Key - Separate key from RF AES key
const byte HMAC_KEY[] = {
  0x91, 0x22, 0xA7, 0x4C,
  0x58, 0xD1, 0x33, 0x9F,
  0x77, 0xE2, 0x19, 0xAB,
  0xCD, 0x44, 0x10, 0x72
};

void setup() {
  Serial.begin(115200);

  //pinMode(LED, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  // Set Radio Output
  pinMode(Radio_Status, OUTPUT);

  // TODO: REMOVE
  // Set output to high temporarily
  digitalWrite(Radio_Status, HIGH);

  Serial.println("RFM69 Reliable TX Test");
  Serial.println();

  // Manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!manager.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");

  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  rf69.setTxPower(10, true);  // RFM69HCW

  // AES Encryption
  rf69.setEncryptionKey(key);

  Serial.print("RFM69 radio @ ");
  Serial.print((int)RF69_FREQ);
  Serial.println(" MHz");

  // Set timeout variables and retries for RHReliableDatagram
  manager.setTimeout(300);  
  manager.setRetries(5);

  // Initialize inputs as ADC pins
  for (uint8_t i = 0; i < NUM_PINS; i++) {
    pinMode(sensePins[i], INPUT_PULLUP);
  }

}

void loop() {
  for (uint8_t i = 0; i < NUM_PINS; i++) {

    // Read ADC Pins and determine if there is a fault
    int adc = analogRead(sensePins[i]);
    float volts = (adc * VREF) / 1023.0f;
    bool faultNow = (volts > THRESHOLD_V);

    // If there is a fault, send a message
    if (faultNow != lastFault[i]) {
      lastFault[i] = faultNow;

      // Increment number of messages sent
      messageCounter++;

      char msg[32];
      snprintf(msg, sizeof(msg),
               "Pin %u %s",
               (unsigned)(i + 1),
               faultNow ? "faulted" : "okay");

      Serial.print("Sending: ");
      Serial.print(msg);
      Serial.print(" (");
      Serial.print(volts, 3);
      Serial.println(" V)");

      // Create message to be sent
      uint8_t packet[64];
      uint8_t msg_len = strlen(msg);

      // Add counter to messages(Similar to a timestamp - Big Endian)
      packet[0] = (messageCounter >> 24) & 0xFF;
      packet[1] = (messageCounter >> 16) & 0xFF;
      packet[2] = (messageCounter >> 8) & 0xFF;
      packet[3] = messageCounter & 0xFF;

      // Append payload after counter
      memcpy(packet + 4, msg, msg_len);

      // Compute HMAC over [counter + message]
      byte mac[32];

      hash.resetHMAC(HMAC_KEY, sizeof(HMAC_KEY));
      hash.update(packet, 4 + msg_len);
      hash.finalize(mac, sizeof(mac));

      // Append first 16 bytes of MAC to end of message
      memcpy(packet + 4 + msg_len, mac, 16);

      uint8_t total_len = 4 + msg_len + 16;

      // Send the message
      if (manager.sendtoWait(packet, total_len, DEST_ADDRESS)) {

        // If ACK Received, set status signal to high
        digitalWrite(Radio_Status, HIGH);
        Serial.println("ACK received");
      } else {

        // If ACK Not received, set status signal to low
        digitalWrite(Radio_Status, LOW);
        Serial.println("No ACK! RX may be offline");
      }

      delay(20);
    }
  }

  delay(100); // 10 Hz polling
}

