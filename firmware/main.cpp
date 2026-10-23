#include <Arduino.h>
#include "controller.h"
#ifndef SPONGE_XIAO
#include <Servo.h>
#endif
using namespace sponge;
const Config config = defaultConfig();
Controller controller(config);
#ifdef SPONGE_XIAO
const uint8_t MQ_PIN = A0;
const uint8_t HALL_PINS[] = {D1, D2, D3, D4, D5};
const uint8_t EYE_IN1 = D6, EYE_IN2 = D7, EYE_EN = D8, VIB_PIN = D9;
const uint16_t ADC_MAX = 4095;
const bool DC_REVERSED = false;
const uint8_t MOTOR_PWM = 140;
#else
const uint8_t MQ_PIN = A0;
const uint8_t HALL_PINS[] = {2, 3, 4};
const uint8_t SERVO_PIN = 9, VIB_PIN = 6;
const uint16_t ADC_MAX = 1023;
Servo eyelid;
#endif
const uint32_t SAMPLE_MS = 100, TELEMETRY_MS = 1000, RAIL_FAULT_MS = 5000;
uint32_t sampleAt = 0, telemetryAt = 0, railSince = 0, filteredQ8 = 0;
uint16_t rawAdc = 0, airIndex = 0;
uint8_t hallMask = 0;
bool firstSample = true, railActive = false;

const __FlashStringHelper* stateName(State s) {
  switch (s) {
    case State::Warmup: return F("WARMUP");
    case State::Normal: return F("NORMAL");
    case State::HighAirTiming: return F("HIGH_AIR_TIMING");
    case State::Alert: return F("ALERT");
    case State::Recovery: return F("RECOVERY");
    default: return F("FAULT");
  }
}
void applyOutputs() {
  digitalWrite(VIB_PIN, controller.vibrationOn() ? HIGH : LOW);
#ifdef SPONGE_XIAO
  const int8_t direction = controller.inhibited() ? 0 : controller.eye().direction();
  analogWrite(EYE_EN, 0);
  const bool forward = (direction > 0) != DC_REVERSED;
  digitalWrite(EYE_IN1, direction && forward ? HIGH : LOW);
  digitalWrite(EYE_IN2, direction && !forward ? HIGH : LOW);
  if (direction) analogWrite(EYE_EN, MOTOR_PWM);
#else
  if (controller.inhibited()) {
    if (eyelid.attached()) eyelid.detach();
  } else {
    eyelid.write(controller.eye().angle());
    if (!eyelid.attached()) eyelid.attach(SERVO_PIN);
  }
#endif
}
void printStatus(uint32_t now) {
  Serial.print(F("ms=")); Serial.print(now);
  Serial.print(F(" state=")); Serial.print(stateName(controller.state()));
  Serial.print(F(" raw=")); Serial.print(rawAdc);
  Serial.print(F(" air_index=")); Serial.print(airIndex);
  Serial.print(F(" hall_mask=")); Serial.print(hallMask);
  Serial.print(F(" position=")); Serial.print(controller.eye().position());
  Serial.print(F(" target=")); Serial.print(controller.eye().target());
  Serial.print(F(" moving=")); Serial.print(controller.eye().moving());
  Serial.print(F(" angle=")); Serial.print(controller.eye().angle());
  Serial.print(F(" vibration=")); Serial.print(controller.vibrationOn());
  Serial.print(F(" fault=")); Serial.println(static_cast<uint8_t>(controller.fault()));
}
void setup() {
  digitalWrite(VIB_PIN, LOW); pinMode(VIB_PIN, OUTPUT);
#ifdef SPONGE_XIAO
  digitalWrite(EYE_EN, LOW); pinMode(EYE_EN, OUTPUT);
  digitalWrite(EYE_IN1, LOW); pinMode(EYE_IN1, OUTPUT);
  digitalWrite(EYE_IN2, LOW); pinMode(EYE_IN2, OUTPUT);
  analogReadResolution(12);
#endif
  for (uint8_t i = 0; i < config.hallCount; ++i) pinMode(HALL_PINS[i], INPUT_PULLUP);
  pinMode(MQ_PIN, INPUT);
  Serial.begin(115200);
  Serial.println(F("Drowsy Sponge: relative air index, NOT CO2 ppm"));
#ifdef SPONGE_DEMO
  Serial.println(F("DEMO: warmup 5s, alert 30s, repeat 10s"));
#endif
  Serial.println(F("Commands: s=status; r=retry fault after mechanical inspection"));
  for (uint8_t i = 0; i < config.hallCount; ++i)
    if (digitalRead(HALL_PINS[i]) == LOW) hallMask |= 1U << i;
  controller.begin(millis(), hallMask);
}
void loop() {
  const uint32_t now = millis();
  hallMask = 0;
  for (uint8_t i = 0; i < config.hallCount; ++i)
    if (digitalRead(HALL_PINS[i]) == LOW) hallMask |= 1U << i;
  const bool sampleReady = firstSample || elapsed(now, sampleAt) >= SAMPLE_MS;
  bool valid = true;
  if (sampleReady) {
    sampleAt = now;
    rawAdc = analogRead(MQ_PIN);
    if (firstSample) { filteredQ8 = static_cast<uint32_t>(rawAdc) << 8; firstSample = false; }
    else filteredQ8 = (filteredQ8 * 7 + (static_cast<uint32_t>(rawAdc) << 8)) / 8;
    airIndex = (filteredQ8 * 2000UL) / (static_cast<uint32_t>(ADC_MAX) * 256UL);
    const bool rail = rawAdc <= 2 || rawAdc >= ADC_MAX - 2;
    if (rail && !railActive) { railActive = true; railSince = now; }
    if (!rail) railActive = false;
    valid = !railActive || elapsed(now, railSince) < RAIL_FAULT_MS;
  }
  controller.update(now, hallMask, sampleReady, airIndex, valid);
  applyOutputs();
  if (Serial.available()) {
    const char command = Serial.read();
    if (command == 's') printStatus(now);
    if (command == 'r' && controller.fault() != Fault::None) {
      controller.begin(now, hallMask); firstSample = true; railActive = false;
    }
  }
  if (elapsed(now, telemetryAt) >= TELEMETRY_MS) { telemetryAt = now; printStatus(now); }
}
