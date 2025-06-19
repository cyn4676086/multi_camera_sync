#pragma once
#include "practical_socket.h"

#include <thread>
#include <memory>
namespace infinite_sense {
class Ptp;
class NetManager {
 public:
  explicit NetManager(std::string target_ip, unsigned short port);
  ~NetManager();
  void Start();
  void Stop();
 private:
  void Receive() const;
  void TimeStampSynchronization() const;
  std::shared_ptr<UDPSocket> net_ptr_;
  std::shared_ptr<Ptp> ptp_;
  unsigned short port_{};
  std::string target_ip_;
  std::thread rx_thread_, tx_thread_;
  bool started_{false};
};
}  // namespace infinite_sense
