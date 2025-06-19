#pragma once

#include <mutex>
#include <map>

#include "infinite_sense.h"
namespace infinite_sense {

/**
 * @brief 宏定义：设置所有设备的最新触发状态。
 *
 * @param timestamp 触发时间戳。
 * @param status 触发状态位掩码（每一位对应一个设备）。
 */
#define SET_LAST_TRIGGER_STATUS(timestamp, status) TriggerManger::GetInstance().SetLastTriggerStatus(timestamp, status)

/**
 * @brief 宏定义：获取指定设备的最新触发状态时间。
 *
 * @param device 要查询的设备。
 * @param timestamp 返回设备的最后触发时间戳。
 */
#define GET_LAST_TRIGGER_STATUS(device, timestamp) TriggerManger::GetInstance().GetLastTriggerStatus(device, timestamp)

/**
 * @class TriggerManger
 * @brief 管理各传感器或设备的触发状态，并通过消息机制发布状态信息。
 *
 * 使用单例模式管理系统中所有的触发设备。支持位掩码形式更新所有设备的状态，
 * 并可查询特定设备的最后一次有效触发时间。
 */
class TriggerManger {
 public:
  /**
   * @brief 获取 TriggerManger 单例对象。
   * @return TriggerManger& 单例引用。
   */
  static TriggerManger &GetInstance() {
    static TriggerManger instance;
    return instance;
  }

  // 删除拷贝构造函数和赋值运算符，确保单例语义。
  TriggerManger(const TriggerManger &) = delete;
  TriggerManger &operator=(const TriggerManger &) = delete;

  /**
   * @brief 设置所有设备的最新触发状态。
   *
   * @param time 当前触发的时间戳。
   * @param status 状态掩码，每一位表示一个设备是否触发。
   */
  void SetLastTriggerStatus(const uint64_t &time, const uint8_t &status);

  /**
   * @brief 获取指定设备的最后一次触发状态及时间。
   *
   * @param dev 设备枚举。
   * @param time 返回该设备最后一次触发的时间戳。
   * @return true 如果该设备有有效的触发状态。
   * @return false 如果该设备无记录。
   */
  bool GetLastTriggerStatus(TriggerDevice dev, uint64_t &time);

 private:
  /**
   * @brief 从位掩码中获取对应设备的状态。
   *
   * @param data 状态字节。
   * @param index 目标设备在位掩码中的索引。
   * @return true 对应位置为1，表示已触发。
   * @return false 对应位置为0，表示未触发。
   */
  static bool GetBool(const uint8_t data, const int index) { return (data >> index) & 1; }

  /**
   * @brief 更新设备状态，并发布触发信息。
   *
   * @param dev 目标设备。
   * @param bit_index 设备对应的位掩码索引。
   * @param time 触发时间戳。
   */
  void UpdateAndPublishDevice(TriggerDevice dev, int bit_index, uint64_t time);

  /**
   * @brief 发布指定设备的状态信息。
   *
   * @param dev 目标设备。
   * @param time 触发时间戳。
   * @param status 当前触发状态。
   */
  static void PublishDeviceStatus(TriggerDevice dev, uint64_t time, bool status);
  TriggerManger();
  ~TriggerManger() = default;
  uint8_t status_{0};
  std::mutex lock_{};
  std::map<TriggerDevice, std::tuple<bool, uint64_t>> status_map_{};
};

}  // namespace infinite_sense
