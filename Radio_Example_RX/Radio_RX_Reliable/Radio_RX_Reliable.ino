// rf69 reliable rx demo
// Uses RH_ReliableDatagram for addressed + acknowledged messaging


#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>


/************ Radio Setup ***************/

#define RF69_FREQ 915.0

// Arduino Nano pins
#define RFM69_CS    4
#define RFM69_INT   3
#define RFM69_RST   2
#define LED        13

// Reliable Datagram addresses
#define MY_ADDRESS    1
#define DEST_ADDRESS  2

// Driver instance
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Reliable Datagram manager
RHReliableDatagram manager(rf69, MY_ADDRESS);

void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("RFM69 Reliable RX Test");
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

  rf69.setTxPower(20, true);  // Required for RFM69HCW

  // Encryption key (must match sender)
  uint8_t key[] = {
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08
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
      buf[len] = 0;

      Serial.print("Received from node ");
      Serial.print(from);
      Serial.print(" [");
      Serial.print(len);
      Serial.print("]: ");
      Serial.println((char*)buf);

      Serial.print("RSSI: ");
      Serial.println(rf69.lastRssi());

      if (strstr((char*)buf, "Hello World")) {
        uint8_t reply[] = "And hello back to you";

        if (manager.sendtoWait(reply, sizeof(reply), from)) {
          Serial.println("Reply sent with ACK");
          Blink(LED, 40, 3);
        } else {
          Serial.println("Reply failed (no ACK)");
        }
      }
    } else {
      Serial.println("Receive failed");
    }
  }
}

void Blink(byte pin, byte delay_ms, byte loops) {
  while (loops--) {
    digitalWrite(pin, HIGH);
    delay(delay_ms);
    digitalWrite(pin, LOW);
    delay(delay_ms);
  }
}
