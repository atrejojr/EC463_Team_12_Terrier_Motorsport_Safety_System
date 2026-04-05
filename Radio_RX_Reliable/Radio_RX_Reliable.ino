// Hannah Tandang and Justin Nascimento 

#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

#include <Crypto.h>
#include <SHA256.h>

#define RF69_FREQ 915.0

// Arduino Nano pins
#define RFM69_CS    4
#define RFM69_INT   3
#define RFM69_RST   2

// Reliable Datagram addresses
#define MY_ADDRESS    1
#define DEST_ADDRESS  2

// Driver instance
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Reliable Datagram manager
RHReliableDatagram manager(rf69, MY_ADDRESS);

// Signature Variables
SHA256 hash;

// Counter Variables (Similar to timestamp)
uint32_t lastSeenCounter = 0;

// MUST match transmitter HMAC key
const byte HMAC_KEY[] = {
  0x91, 0x22, 0xA7, 0x4C,
  0x58, 0xD1, 0x33, 0x9F,
  0x77, 0xE2, 0x19, 0xAB,
  0xCD, 0x44, 0x10, 0x72
};

/************ Setup Function ***************/
void setup() {
  Serial.begin(115200);

  // Radio Setup
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("RFM69 Reliable RX Test");
  Serial.println();

  // Manually reset radio
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

  rf69.setTxPower(10, true);  // Required for RFM69HCW

  // AES Encryption (must match sender)
  uint8_t key[] = {
    0x53, 0x63, 0x61, 0x72,
    0x6C, 0x65, 0x74, 0x74,
    0x32, 0x30, 0x32, 0x30,
    0x0A, 0x0A, 0x0A, 0x0A
  };

  rf69.setEncryptionKey(key);

  Serial.print("RFM69 radio @ ");
  Serial.print((int)RF69_FREQ);
  Serial.println(" MHz");
}

void loop() { 
  if (manager.available()) { 
      uint8_t buf[RH_RF69_MAX_MESSAGE_LEN]; 
      uint8_t len = sizeof(buf); 
      uint8_t from; 
      if (manager.recvfromAck(buf, &len, &from)) {
        // Minimum size: 4B counter + 16B MAC
        if (len < 20) {
          Serial.println("Packet too short");
          return;
        }

        // Extract counter (big endian)
        uint32_t receivedCounter =
            ((uint32_t)buf[0] << 24) |
            ((uint32_t)buf[1] << 16) |
            ((uint32_t)buf[2] << 8)  |
            (uint32_t)buf[3];

        // Calculate message length
        uint8_t msg_len = len - 4 - 16;

        // Extract received MAC
        uint8_t received_mac[16];
        memcpy(received_mac, buf + 4 + msg_len, 16);

        // Compute expected MAC - HMAC over (counter + message)
        byte computed_mac[32];
        hash.resetHMAC(HMAC_KEY, sizeof(HMAC_KEY));
        hash.update(buf, 4 + msg_len);
        hash.finalize(computed_mac, sizeof(computed_mac));

        // Verify HMAC
        if (memcmp(received_mac, computed_mac, 16) != 0) {
          Serial.println("HMAC INVALID - packet rejected");
          return;
        }

        // Replay protection
        if (receivedCounter != (lastSeenCounter + 1)) {
          Serial.println("Replay detected - packet rejected");
          return;
        }

        lastSeenCounter = receivedCounter;

        // Extract message
        buf[4 + msg_len] = 0;  // null terminate

        Serial.print("Valid packet #");
        Serial.print(receivedCounter);
        Serial.print(" from node ");
        Serial.print(from);
        Serial.print(": ");
        Serial.println((char*)(buf + 4));

        Serial.print("RSSI: ");
        Serial.println(rf69.lastRssi());
      } 
      else { 
        Serial.println("Receive failed"); 
      } 
    }
}
