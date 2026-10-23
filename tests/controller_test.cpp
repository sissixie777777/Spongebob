#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../firmware/controller.h"
using namespace sponge;

void airTests() {
  Config c;
  AirMonitor a(c);
  a.update(0, 999); assert(!a.high());
  a.update(0, 1000); assert(a.high() && !a.alert());
  a.update(1799999, 950); assert(!a.triggered());
  a.update(1800000, 950); assert(a.triggered() && a.alert());
  // Once alerting, a full uptime rollover must not reintroduce a 30-minute gate.
  a.update(2399999, 950); assert(!a.triggered());
  a.update(2400000, 950); assert(a.triggered());
  a.update(2400001, 900); assert(!a.high() && !a.alert() && a.level() == 1);
  a.update(2400100, 1100); assert(a.high() && !a.alert());
  a.update(2400101, 899); assert(!a.high());
  const uint32_t start = UINT32_MAX - 1000;
  a.update(start, 1100);
  a.update(start + c.highDurationMs - 1, 1100); assert(!a.alert());
  a.update(start + c.highDurationMs, 1100); assert(a.triggered());
  c.highDurationMs = 0; a.reset(); a.update(0, 1100);
  assert(a.triggered()); a.update(1, 1100); assert(!a.triggered());
  c.highDurationMs = 1800000; a.reset(); a.update(0, 1100);
  a.update(UINT32_MAX - 100, 1100); assert(a.triggered());
  a.update(c.repeatMs, 1100); assert(a.triggered());
}
void levels() {
  Config c;
  AirMonitor a(c);
  a.update(0, 1000); assert(a.level() == 2);
  a.update(1, 1400); assert(a.level() == 3);
  a.update(2, 1380); assert(a.level() == 3);
  a.update(3, 1359); assert(a.level() == 2);
  c.hallCount = 5; a.reset();
  for (uint8_t i = 0; i < 4; ++i) {
    a.update(i, c.levelThresholds[i]); assert(a.level() == i + 2);
  }
}
void vibrations() {
  Vibration v;
  assert(!v.update(0));
  const uint32_t start = UINT32_MAX - 200;
  v.start(start);
  for (uint32_t t = 0; t <= 3000; ++t)
    assert(v.update(start + t) == (t < 2500 && (t / 500) % 2 == 0));
  v.start(0); v.stop(); assert(!v.update(1));
}
void halls() {
  assert(decodeHall(0, 3) == 0);
  assert(decodeHall(4, 3) == 3);
  assert(decodeHall(3, 3) == -1);
  HallFilter f;
  assert(f.update(0, 1, 20) == 0);
  assert(f.update(19, 1, 20) == 0);
  assert(f.update(20, 1, 20) == 1);
  assert(f.update(21, 0, 20) == 1);
  assert(f.update(22, 1, 20) == 1);
}
void eyeTests() {
  Config c;
  Eye e(c);
  e.begin(0, 1); assert(e.angle() == c.servoOpen);
  e.update(0, 1); assert(e.homed() && !e.moving());
  e.request(3, 1); assert(e.direction() == 1);
  e.update(20, 0); assert(e.moving());
  e.update(100, 2); assert(e.position() == 2 && e.moving());
  e.update(200, 3); assert(!e.moving() && e.position() == 3);
  e.request(1, 201); e.update(300, 2); e.update(400, 1);
  assert(!e.moving() && e.fault() == Fault::None);
  e.request(3, 500); e.update(550, 0); e.request(1, 600);
  assert(e.moving() && e.direction() == -1); // recovery between sensors
  e.update(700, 1); assert(!e.moving());
  e.request(3, 800); e.request(2, 900);
  e.update(3800, 0); assert(e.fault() == Fault::MotionTimeout);
  assert(!e.moving() && e.direction() == 0);
  e.begin(UINT32_MAX - 50); e.update(2949, 0);
  assert(e.fault() == Fault::MotionTimeout);
  e.begin(0); e.update(1, -1); assert(e.fault() == Fault::HallConflict);
  e.begin(0); e.update(1, 3); e.update(2, 2); e.update(3, 3);
  assert(e.fault() == Fault::WrongDirection);
  c.servoOpen = 150; c.servoClosed = 30;
  e.begin(0, 1); e.update(0, 1); e.request(3, 1); e.update(20, 0);
  assert(e.angle() < 150);
}
void controllerTests(bool dc) {
  Config c; c.servo = !dc; c.hallCount = dc ? 5 : 3;
  c.warmupMs = 100; c.highDurationMs = 1000; c.repeatMs = 5000;
  Controller app(c);
  app.begin(0, 1);
  app.update(0, 1, true, 1800);
  app.update(20, 1, true, 1800);
  app.update(99, 1, true, 1800); assert(app.state() == State::Warmup);
  app.update(100, 1, true, 1100); assert(app.state() == State::HighAirTiming);
  app.update(101, 2, false, 0); app.update(121, 2, false, 0);
  app.update(1099, 2, true, 1100); assert(!app.vibrationOn());
  app.update(1100, 2, true, 1100); assert(app.state() == State::Alert && app.vibrationOn());
  const uint8_t closed = 1U << (c.hallCount - 1);
  app.update(1101, closed, false, 0); app.update(1121, closed, false, 0);
  assert(!app.eye().moving());
  app.update(1421, closed, false, 0); assert(app.eye().target() == 1);
  app.update(1500, 0, true, 900);
  assert(app.state() == State::Recovery && !app.vibrationOn() && app.eye().target() == 1);
  app.update(1510, 1, false, 0); app.update(1530, 1, false, 0);
  assert(app.state() == State::Normal);
  app.update(1540, 1, true, 1100); assert(app.state() == State::HighAirTiming);
  app.update(1541, 3, false, 0); assert(app.inhibited());
  app.update(1561, 3, false, 0); assert(app.fault() == Fault::HallConflict);
  assert(!app.vibrationOn());
  app.begin(2000, 1); app.update(2000, 1, false, 0); app.update(2020, 1, false, 0);
  assert(app.fault() == Fault::None);
  app.update(2100, 1, true, 500, false); assert(app.fault() == Fault::AirSensor);
}
int main() {
  airTests(); levels(); vibrations(); halls(); eyeTests();
  controllerTests(false); controllerTests(true);
  std::puts("PASS: timing, rollover, hysteresis, levels, pulses, Hall feedback, homing, timeout, recovery, fault retry (Nano + XIAO)");
}
