#pragma once
#include "log.h"
#include "config.h"
#include "messenger.h"
#include "sensor.h"
#include "trigger.h"
namespace infinite_sense {

class NetManager;
class UsbManager;
class Sensor;
class TriggerManger;
class Messenger;
/**
 * @class Synchronizer
 * @brief 同步器类，负责统一协调网络、串口、相机等模块的启动与配置，管理多传感器系统的时间同步。
 */
class Synchronizer {
 public:
  /**
   * @brief 构造函数，初始化成员变量。
   */
  Synchronizer();

  /// 默认析构函数。
  ~Synchronizer() = default;

  /**
   * @brief 启动同步器，初始化并启动各组件（网络、串口、相机等）。
   */
  void Start() const;

  /**
   * @brief 停止同步器，释放资源。
   */
  void Stop() const;

  /**
   * @brief 设置日志输出路径（静态方法）。
   *
   * @param path 日志保存路径。
   */
  static void SetLogPath(const std::string& path);

  /**
   * @brief 配置串口连接参数。
   *
   * @param serial_dev 串口设备名称（如 "/dev/ttyUSB0"）。
   * @param serial_baud_rate 波特率（如 115200）。
   */
  void SetUsbLink(std::string serial_dev, int serial_baud_rate);

  /**
   * @brief 配置网络连接参数。
   *
   * @param net_dev 目标网络 IP 地址。
   * @param port 网络端口号。
   */
  void SetNetLink(std::string net_dev, unsigned int port);

  /**
   * @brief 使用传感器，并配置其对应的触发设备。
   *
   * @param sensor
   */
  void UseSensor(const std::shared_ptr<Sensor>&);

 private:
  /// 网络地址
  std::string net_ip_;

  /// 网络端口号
  unsigned short net_port_{};

  /// 串口设备
  std::string serial_dev_;

  /// 串口波特率
  int serial_baud_rate_{};

  /// 网络管理器
  std::shared_ptr<NetManager> net_manager_{nullptr};

  /// 串口管理器
  std::shared_ptr<UsbManager> serial_manager_{nullptr};

  /// 相机管理器
  std::shared_ptr<Sensor> sensor_manager_{nullptr};
};

}  // namespace infinite_sense
