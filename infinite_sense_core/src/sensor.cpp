#include "sensor.h"

namespace infinite_sense {
Sensor::Sensor() = default;

Sensor::~Sensor() = default;

void Sensor::Restart() {
  Stop();
  std::this_thread::sleep_for(std::chrono::milliseconds{500});
  if (!Initialization()) {
    LOG(INFO) << "Camera initialization failed after restart!";
  } else {
    Start();
    LOG(INFO) << "Cameras successfully restarted!";
  }
}
}  // namespace infinite_sense