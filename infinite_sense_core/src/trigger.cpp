#include "trigger.h"

namespace infinite_sense {

std::map<TriggerDevice, std::string> device_map_topics{
    {IMU_1, "imu_1_trigger"},
    {IMU_2, "imu_2_trigger"},
    {CAM_1, "cam_1_trigger"},
    {CAM_2, "cam_2_trigger"},
    {CAM_3, "cam_3_trigger"},
    {CAM_4, "cam_4_trigger"},
    {LASER, "laser_trigger"},
    {GPS, "gps_trigger"}};

TriggerManger::TriggerManger() {
  uint64_t time = std::numeric_limits<uint64_t>::max();
  const std::tuple<bool, uint64_t> empty = std::tuple(0, time);
  status_map_[IMU_1] = empty;
  status_map_[IMU_2] = empty;
  status_map_[CAM_1] = empty;
  status_map_[CAM_2] = empty;
  status_map_[CAM_3] = empty;
  status_map_[CAM_4] = empty;
  status_map_[LASER] = empty;
  status_map_[GPS] = empty;
}
void TriggerManger::SetLastTriggerStatus(const uint64_t& time, const uint8_t& status) {
  std::lock_guard lock(lock_);
  status_ = status;
  UpdateAndPublishDevice(IMU_1, 0, time);
  UpdateAndPublishDevice(IMU_2, 1, time);
  UpdateAndPublishDevice(CAM_1, 2, time);
  UpdateAndPublishDevice(CAM_2, 3, time);
  UpdateAndPublishDevice(CAM_3, 4, time);
  UpdateAndPublishDevice(CAM_4, 5, time);
  UpdateAndPublishDevice(LASER, 6, time);
  UpdateAndPublishDevice(GPS, 7, time);
}

bool TriggerManger::GetLastTriggerStatus(const TriggerDevice dev, uint64_t& time) {
  std::lock_guard lock(lock_);
  if (status_map_.find(dev) == status_map_.end()) {
    return false;
  }
  time = std::get<1>(status_map_[dev]);
  return std::get<0>(status_map_[dev]);
}
void TriggerManger::UpdateAndPublishDevice(const TriggerDevice dev, const int bit_index, uint64_t time) {
  if (bool status = GetBool(status_, bit_index)) {
    status_map_[dev] = std::tuple(status, time);
    PublishDeviceStatus(dev, time, status);
  }
}
void TriggerManger::PublishDeviceStatus(const TriggerDevice dev, const uint64_t time, const bool status) {
  try {
    const struct DeviceStatus {
      uint64_t timestamp;
      bool status;
    } dev_data{time, status};
    Messenger::GetInstance().PubStruct(device_map_topics[dev], &dev_data, sizeof(DeviceStatus));
  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to publish " << device_map_topics[dev] << " status: " << e.what();
  }
}
}  // namespace infinite_sense