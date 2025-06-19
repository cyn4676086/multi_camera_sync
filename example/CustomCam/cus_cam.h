#pragma once
#include "infinite_sense.h"
namespace infinite_sense {
class CustomCam final : public Sensor {
 public:
  ~CustomCam() override;
  bool Initialization() override;
  void Stop() override;
  void Start() override;

 private:
  void Receive(void* handle, const std::string&) override;
};
}  // namespace infinite_sense
