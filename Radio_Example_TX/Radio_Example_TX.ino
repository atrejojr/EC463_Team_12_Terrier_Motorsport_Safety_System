#include <SPI.h>
#include <RH_RF69.h>

/************ Radio Setup ***************/
#define RF69_FREQ 915.0

#define RFM69_CS    4
#define RFM69_INT   3
#define RFM69_RST   2
#define LED        13

RH_RF69 rf69(RFM69_CS, RFM69_INT);

/************ GPIO Voltage Inputs ************/
// Use analog pins so we can compare to 1.0V
const uint8_t NUM_PINS = 5;
const uint8_t sensePins[NUM_PINS] = {A0, A1, A2, A3, A4};
const char* signalNames[NUM_PINS] = {
  "VSB_STATUS",    // A0
  "CANWD_STATUS",  // A1
  "RADIO_STATUS",  // A2
  "IMD_STATUS",    // A3
  "AMS_STATUS"     // A4
};

// Nano uses DEFAULT analog reference = 5.0V (unless you change it)
const float VREF = 5.0f;
const float THRESHOLD_V = 1.0f; // "fault" if above 1V

// Track last status so we only send on change
bool lastFault[NUM_PINS] = {false, false, false, false, false};

void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather RFM69 TX Test!");
  Serial.println();

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");

  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  rf69.setTxPower(20, true);  // for RFM69HCW

  uint8_t key[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
  };
  rf69.setEncryptionKey(key);

  Serial.print("RFM69 radio @"); Serial.print((int)RF69_FREQ); Serial.println(" MHz");

  // Analog pins don’t require pinMode, but it’s okay to be explicit
  for (uint8_t i = 0; i < NUM_PINS; i++) {
    pinMode(sensePins[i], INPUT);
  }
}

void loop() {
  // Poll pins
  for (uint8_t i = 0; i < NUM_PINS; i++) {
    int adc = analogRead(sensePins[i]);                 // 0..1023
    float volts = (adc * VREF) / 1023.0f;               // convert to volts
    bool faultNow = (volts > THRESHOLD_V);

    // Only send if changed since last time
    if (faultNow != lastFault[i]) {
      lastFault[i] = faultNow;

      // Build message: "Pin # faulted" or "Pin # okay"
      char msg[64];
      // i+1 gives Pin 1..5 (change to i if you want 0..4)
      snprintf(msg, sizeof(msg), "%s %s", signalNames[i],
               faultNow ? "faulted" : "okay");

      Serial.print("Sending: ");
      Serial.print(msg);
      Serial.print(" (");
      Serial.print(volts, 3);
      Serial.println(" V)");

      rf69.send((uint8_t*)msg, strlen(msg));
      rf69.waitPacketSent();

      Blink(LED, 40, 1); // quick blink on each status-change transmit
      delay(20);         // small spacing between packets
    }
  }

  delay(100); // poll rate (10 Hz). Increase if you want faster updates.
}

void Blink(byte pin, byte delay_ms, byte loops) {
  while (loops--) {
    digitalWrite(pin, HIGH);
    delay(delay_ms);
    digitalWrite(pin, LOW);
    delay(delay_ms);
  }
}
