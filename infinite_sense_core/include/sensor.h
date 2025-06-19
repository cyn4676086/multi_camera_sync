#pragma once
#include <thread>
#include <vector>
#include "messenger.h"
#include "trigger.h"
namespace infinite_sense {
class Sensor {
 public:
  Sensor();
  virtual ~Sensor();

  virtual bool Initialization() = 0;

  virtual void Stop() = 0;

  virtual void Start() = 0;

  void Restart();
  void Enable() { is_running = true; }
  void Disable() { is_running = false; }
  void SetParams(const std::map<std::string, TriggerDevice> &params_in) { params = params_in; };

 private:
  virtual void Receive(void *, const std::string &) = 0;

 protected:
  bool is_running{false};
  std::vector<std::thread> cam_threads{};
  std::map<std::string, TriggerDevice> params{};
};

}  // namespace infinite_sense
