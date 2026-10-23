#pragma once
#include "config.h"

namespace sponge {
inline uint32_t elapsed(uint32_t now, uint32_t start) { return now - start; }
enum class State : uint8_t { Warmup, Normal, HighAirTiming, Alert, Recovery, Fault };
enum class Fault : uint8_t { None, HallConflict, MotionTimeout, WrongDirection, AirSensor };

// Zero means between sensors; -1 means mutually contradictory Hall signals.
inline int8_t decodeHall(uint8_t mask, uint8_t count) {
  int8_t result = 0;
  for (uint8_t i = 0; i < count; ++i) {
    if (mask & (1U << i)) {
      if (result) return -1;
      result = static_cast<int8_t>(i + 1);
    }
  }
  return result;
}

class HallFilter {
 public:
  int8_t update(uint32_t now, int8_t raw, uint32_t debounce) {
    if (!initialized_ || raw != candidate_) {
      initialized_ = true;
      candidate_ = raw;
      since_ = now;
    }
    if (elapsed(now, since_) >= debounce) stable_ = candidate_;
    return stable_;
  }
 private:
  bool initialized_ = false;
  int8_t candidate_ = 0, stable_ = 0;
  uint32_t since_ = 0;
};

class AirMonitor {
 public:
  explicit AirMonitor(const Config& config) : c_(config) {}
  void reset() { high_ = alert_ = trigger_ = false; level_ = 1; }
  void update(uint32_t now, uint16_t value) {
    trigger_ = false;
    if (value <= c_.resetThreshold) { reset(); return; }
    if (!high_ && value >= c_.highThreshold) {
      high_ = true; start_ = now; alert_ = false;
    }
    if (!high_) return;
    uint8_t desired = 1;
    // Three purchased sensors represent open / half / closed.
    for (uint8_t i = 1; i < c_.hallCount; ++i) {
      const uint8_t index = c_.hallCount == 3 ? (i - 1) * 2 : i - 1;
      uint16_t threshold = c_.levelThresholds[index];
      if (i < level_) threshold -= c_.levelHysteresis;
      if (value >= threshold) desired = i + 1;
    }
    level_ = desired;
    if ((alert_ || elapsed(now, start_) >= c_.highDurationMs) &&
        (!alert_ || elapsed(now, last_) >= c_.repeatMs)) {
      alert_ = trigger_ = true; last_ = now;
    }
  }
  bool high() const { return high_; }
  bool alert() const { return alert_; }
  bool triggered() const { return trigger_; }
  uint8_t level() const { return level_; }
 private:
  const Config& c_;
  bool high_ = false, alert_ = false, trigger_ = false;
  uint8_t level_ = 1;
  uint32_t start_ = 0, last_ = 0;
};

class Vibration {
 public:
  void start(uint32_t now) { active_ = true; start_ = now; }
  void stop() { active_ = false; }
  bool update(uint32_t now) {
    if (!active_) return false;
    const uint32_t t = elapsed(now, start_);
    if (t >= 2500) { active_ = false; return false; }
    return (t / 500) % 2 == 0;
  }
 private:
  bool active_ = false;
  uint32_t start_ = 0;
};

class Eye {
 public:
  explicit Eye(const Config& c) : c_(c), angle_(c.servoOpen) {}
  void begin(uint32_t now, int8_t initialHall = 0) {
    fault_ = Fault::None; homed_ = false; moving_ = true;
    target_ = 1; position_ = 0; direction_ = -1; lastHall_ = 0; started_ = stepAt_ = now;
    // At startup an SG90's absolute angle is unknown; use a conservative
    // midpoint command, then walk toward the open endpoint with Hall feedback.
    angle_ = (c_.servoOpen + c_.servoClosed) / 2;
    if (initialHall > 0) angle_ = static_cast<uint8_t>(
      c_.servoOpen + (static_cast<int>(c_.servoClosed) - c_.servoOpen) *
      (initialHall - 1) / (c_.hallCount - 1));
    if (initialHall > 0) position_ = initialHall;
  }
  void request(uint8_t target, uint32_t now) {
    if (fault_ != Fault::None || !homed_ || target < 1 || target > c_.hallCount) return;
    if (target == target_) return;
    target_ = target;
    // Keep the original timeout budget when retargeting an in-flight move.
    if (!moving_) {
      if (position_ == target_) return;
      started_ = now; moving_ = true;
    }
    direction_ = target_ == position_ ? -direction_ : (target_ > position_ ? 1 : -1);
  }
  void update(uint32_t now, int8_t hall) {
    if (fault_ != Fault::None) return;
    if (hall < 0) { fail(Fault::HallConflict); return; }
    if (hall > 0) {
      if (moving_ && lastHall_ &&
          ((direction_ > 0 && hall < lastHall_) || (direction_ < 0 && hall > lastHall_))) {
        fail(Fault::WrongDirection); return;
      }
      lastHall_ = hall;
      position_ = hall;
      if (moving_ && hall == target_) {
        moving_ = false; homed_ = true; direction_ = 0;
      }
    }
    if (!moving_) return;
    if (elapsed(now, started_) >= c_.moveTimeoutMs) { fail(Fault::MotionTimeout); return; }
    if (c_.servo && elapsed(now, stepAt_) >= c_.servoStepMs) {
      stepAt_ = now;
      const int sign = c_.servoClosed > c_.servoOpen ? 1 : -1;
      int next = angle_ + sign * direction_ * c_.servoStep;
      const int lo = c_.servoOpen < c_.servoClosed ? c_.servoOpen : c_.servoClosed;
      const int hi = c_.servoOpen > c_.servoClosed ? c_.servoOpen : c_.servoClosed;
      if (next < lo) next = lo;
      if (next > hi) next = hi;
      angle_ = static_cast<uint8_t>(next);
    }
  }
  void fail(Fault reason) { fault_ = reason; moving_ = false; direction_ = 0; }
  bool homed() const { return homed_; }
  bool moving() const { return moving_; }
  uint8_t position() const { return position_; }
  uint8_t target() const { return target_; }
  uint8_t angle() const { return angle_; }
  int8_t direction() const { return direction_; }
  Fault fault() const { return fault_; }
 private:
  const Config& c_;
  Fault fault_ = Fault::None;
  bool homed_ = false, moving_ = false;
  uint8_t position_ = 0, target_ = 1, angle_;
  int8_t direction_ = 0, lastHall_ = 0;
  uint32_t started_ = 0, stepAt_ = 0;
};

class Controller {
 public:
  explicit Controller(const Config& c) : c_(c), air_(c), eye_(c) {}
  void begin(uint32_t now, uint8_t initialMask = 0) {
    boot_ = now; state_ = State::Warmup; fault_ = Fault::None;
    halls_ = HallFilter();
    air_.reset(); vibration_.stop(); eye_.begin(now, decodeHall(initialMask, c_.hallCount));
    blink_ = 0; vibrationOn_ = false; inhibited_ = false;
  }
  void update(uint32_t now, uint8_t hallMask, bool sampleReady, uint16_t index,
              bool sampleValid = true) {
    const int8_t raw = decodeHall(hallMask, c_.hallCount);
    const int8_t hall = halls_.update(now, raw, c_.hallDebounceMs);
    // Never power an actuator into an un-debounced contradictory reading.
    inhibited_ = raw < 0 || elapsed(now, boot_) < c_.hallDebounceMs;
    eye_.update(now, hall);
    if (eye_.fault() != Fault::None) fail(eye_.fault());
    // MQ135 may be near a rail during heater warm-up. Diagnose it after
    // the configured stabilization interval rather than latching immediately.
    if (sampleReady && !sampleValid && elapsed(now, boot_) >= c_.warmupMs)
      fail(Fault::AirSensor);
    if (fault_ != Fault::None) { vibrationOn_ = false; return; }
    if (elapsed(now, boot_) < c_.warmupMs || !eye_.homed()) {
      vibrationOn_ = false; return;
    }
    if (state_ == State::Warmup) state_ = State::Normal;
    if (sampleReady) {
      const bool wasHigh = air_.high();
      air_.update(now, index);
      if (!air_.high()) {
        vibration_.stop(); blink_ = 0;
        state_ = wasHigh || state_ == State::Recovery ? State::Recovery : State::Normal;
        eye_.request(1, now);
      } else {
        state_ = air_.alert() ? State::Alert : State::HighAirTiming;
        if (air_.triggered()) {
          vibration_.start(now); blink_ = 1;
          eye_.request(c_.hallCount, now);
        } else if (!blink_) eye_.request(air_.level(), now);
      }
    }
    if (blink_ == 1 && !eye_.moving()) { blink_ = 2; blinkAt_ = now; }
    if (blink_ == 2 && elapsed(now, blinkAt_) >= 300) {
      blink_ = 3; eye_.request(1, now);
    }
    if (blink_ == 3 && !eye_.moving()) { blink_ = 4; blinkAt_ = now; }
    if (blink_ == 4 && elapsed(now, blinkAt_) >= 300) {
      blink_ = 0; eye_.request(air_.level(), now);
    }
    if (state_ == State::Recovery && !eye_.moving() && eye_.position() == 1)
      state_ = State::Normal;
    vibrationOn_ = !inhibited_ && vibration_.update(now);
  }
  void fail(Fault f) { fault_ = f; state_ = State::Fault; eye_.fail(f); vibration_.stop(); }
  State state() const { return state_; }
  Fault fault() const { return fault_; }
  const Eye& eye() const { return eye_; }
  bool vibrationOn() const { return vibrationOn_; }
  bool inhibited() const { return inhibited_ || fault_ != Fault::None; }
 private:
  const Config& c_;
  AirMonitor air_;
  Eye eye_;
  HallFilter halls_;
  Vibration vibration_;
  State state_ = State::Warmup;
  Fault fault_ = Fault::None;
  bool vibrationOn_ = false, inhibited_ = false;
  uint8_t blink_ = 0;
  uint32_t boot_ = 0, blinkAt_ = 0;
};
} // namespace sponge
