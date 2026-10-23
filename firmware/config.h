#pragma once
#include <stdint.h>

namespace sponge {
// All air thresholds use a dimensionless 0..2000 ADC trend index, NOT ppm.
struct Config {
  uint32_t warmupMs = 180000UL; // runtime warm-up, not initial sensor burn-in
  uint32_t highDurationMs = 30UL * 60UL * 1000UL;
  uint32_t repeatMs = 10UL * 60UL * 1000UL;
  uint32_t moveTimeoutMs = 3000;
  uint32_t hallDebounceMs = 20;
  uint32_t servoStepMs = 20;
  uint16_t highThreshold = 1000;
  uint16_t resetThreshold = 900;
  uint16_t levelThresholds[4] = {1000, 1200, 1400, 1600};
  uint16_t levelHysteresis = 40;
  uint8_t hallCount = 3;
  uint8_t servoOpen = 30;
  uint8_t servoClosed = 150;
  uint8_t servoStep = 1;
  bool servo = true;
};

inline Config defaultConfig() {
  Config c;
#ifdef SPONGE_XIAO
  c.servo = false;
  c.hallCount = 5;
#endif
#ifdef SPONGE_DEMO
  c.warmupMs = 5000;
  c.highDurationMs = 30000;
  c.repeatMs = 10000;
#endif
  return c;
}
} // namespace sponge
