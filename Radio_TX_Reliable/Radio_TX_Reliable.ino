#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

/************ Radio Setup ***************/
#define RF69_FREQ 915.0

#define RFM69_CS    4
#define RFM69_INT   3
#define RFM69_RST   2
//#define LED        13

// Reliable Datagram addresses
#define MY_ADDRESS    2
#define DEST_ADDRESS  1

RH_RF69 rf69(RFM69_CS, RFM69_INT);
RHReliableDatagram manager(rf69, MY_ADDRESS);

/************ GPIO Voltage Inputs ************/
const uint8_t NUM_PINS = 5;
const uint8_t sensePins[NUM_PINS] = {A0, A1, A2, A3, A4};

const float VREF = 5.0f;
const float THRESHOLD_V = 1.0f;

// Track last status so we only send on change
bool lastFault[NUM_PINS] = {false, false, false, false, false};

void setup() {
  Serial.begin(115200);

  //pinMode(LED, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

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

  for (uint8_t i = 0; i < NUM_PINS; i++) {
    pinMode(sensePins[i], INPUT);
  }

  manager.setTimeout(300);   // ms
  manager.setRetries(5);

}

void loop() {
  for (uint8_t i = 0; i < NUM_PINS; i++) {
    int adc = analogRead(sensePins[i]);
    float volts = (adc * VREF) / 1023.0f;
    bool faultNow = (volts > THRESHOLD_V);

    if (faultNow != lastFault[i]) {
      lastFault[i] = faultNow;

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

      // Reliable send with ACK + retries
      if (manager.sendtoWait((uint8_t*)msg, strlen(msg), DEST_ADDRESS)) {
        Serial.println("ACK received");
        //Blink(LED, 40, 1);
      } else {
        Serial.println("No ACK! RX may be offline");
        //Blink(LED, 100, 3);  // visible failure indication
      }

      delay(20);
    }
  }

  delay(100); // 10 Hz polling
}

/*
void Blink(byte pin, byte delay_ms, byte loops) {
  while (loops--) {
    digitalWrite(pin, HIGH);
    delay(delay_ms);
    digitalWrite(pin, LOW);
    delay(delay_ms);
  }
}
*/
