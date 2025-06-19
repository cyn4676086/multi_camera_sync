#pragma once
#include "infinite_sense.h"
#include "MvCameraControl.h"
#include <vector>         // for std::vector
#include <string>         // for std::string
#include <mutex>          // for std::mutex

// 新增：性能优化所需的头文件
#include <sched.h>        // CPU亲和性设置
#include <pthread.h>      // 线程优化
#include <thread>         // 硬件并发检测
#include <cstdlib>        // aligned_alloc

namespace infinite_sense {
class MvCam final : public Sensor {
 public:
  ~MvCam() override;

  bool Initialization() override;
  void Stop() override;
  void Start() override;

  // 新增：获取所有相机的用户自定义名称（不初始化相机）
  std::vector<std::string> ScanCameraNames();
  
  // 新增：获取已初始化相机的名称
  std::vector<std::string> GetCameraNames() const;
  
  // 新增：获取相机数量
  size_t GetCameraCount() const { return handles_.size(); }

 private:
  void Receive(void* handle, const std::string&) override;
  std::vector<int> rets_;
  std::vector<void*> handles_;
  std::mutex messenger_mutex_;  // 保护Messenger::PubStruct调用
  
  // 新增：存储相机名称
  mutable std::vector<std::string> camera_names_;
  mutable std::mutex names_mutex_;
};
}  // namespace infinite_sense