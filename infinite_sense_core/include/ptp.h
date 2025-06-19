#pragma once
#include "usb.h"
#include <json.h>
#include <practical_socket.h>
namespace infinite_sense {
class Ptp {
 public:

  Ptp() = default;
  void ReceivePtpData(const nlohmann::json &);
  void SendPtpData() const;
  void SetUsbPtr(const std::shared_ptr<serial::Serial> &);
  void SetNetPtr(const std::shared_ptr<UDPSocket> &, const std::string &, unsigned short);

 private:
  void HandleTimeSyncRequest(const nlohmann::json &data);
  void HandleTimeSyncResponse(const nlohmann::json &data);
  static uint64_t GetCurrentTimeUs();
  void SendJson(const nlohmann::json &data) const;

  std::shared_ptr<serial::Serial> serial_ptr_{nullptr};
  std::shared_ptr<UDPSocket> net_ptr_{nullptr};
  unsigned short port_{};
  std::string target_ip_{};
  uint64_t time_t1_{0};
  uint64_t time_t2_{0};
  bool updated_t1_t2_{false};
};

}  // namespace infinite_sense