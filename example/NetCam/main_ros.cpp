#include "infinite_sense.h"
#include "mv_cam.h"
#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/Image.h>
#include <std_msgs/Header.h>
#include <map>
#include <atomic>
#include <mutex>

using namespace infinite_sense;

inline ros::Time CreateRosTimestamp(const uint64_t mico_sec) {
  uint32_t nsec_per_second = 1e9;
  auto u64 = mico_sec * 1000;
  uint32_t sec = u64 / nsec_per_second;
  uint32_t nsec = u64 - (sec * nsec_per_second);
  return {sec, nsec};
}

class OptimizedMultiCamDriver {
 public:
  OptimizedMultiCamDriver(ros::NodeHandle &nh) 
    : node_(nh), it_(node_), imu_name_("imu_1") {
    // 动态获取相机名称，不再硬编码
  }

  void ImuCallback(const void *msg, size_t size) {
    if (!msg || !ros::ok()) return;
    
    try {
      const auto *imu_data = static_cast<const ImuData *>(msg);
      
      // 检查指针有效性
      if (!imu_data) return;
      
      sensor_msgs::Imu imu_msg_data;
      imu_msg_data.header.frame_id = "imu";
      imu_msg_data.header.stamp = CreateRosTimestamp(imu_data->time_stamp_us);
      
      imu_msg_data.angular_velocity.x = imu_data->g[0];
      imu_msg_data.angular_velocity.y = imu_data->g[1];
      imu_msg_data.angular_velocity.z = imu_data->g[2];
      
      imu_msg_data.linear_acceleration.x = imu_data->a[0];
      imu_msg_data.linear_acceleration.y = imu_data->a[1];
      imu_msg_data.linear_acceleration.z = imu_data->a[2];
      
      imu_msg_data.orientation.w = imu_data->q[0];
      imu_msg_data.orientation.x = imu_data->q[1];
      imu_msg_data.orientation.y = imu_data->q[2];
      imu_msg_data.orientation.z = imu_data->q[3];
      
      // 线程安全发布
      std::lock_guard<std::mutex> lock(publish_mutex_);
      if (imu_pub_.getNumSubscribers() > 0) {
        imu_pub_.publish(imu_msg_data);
      }
    } catch (const std::exception& e) {
      ROS_ERROR_THROTTLE(5.0, "IMU callback error: %s", e.what());
    } catch (...) {
      ROS_ERROR_THROTTLE(5.0, "IMU callback unknown error");
    }
  }

  // **修复段错误的安全图像处理**
  void ImageCallback(const std::string& camera_name, const void *msg, size_t size) {
    if (!msg || !ros::ok()) {
      return;
    }
    
    try {
      const auto *cam_data = static_cast<const CamData *>(msg);
      
      // 更严格的数据验证
      if (!cam_data || !cam_data->image.data) {
        return;
      }
      
      // 获取实际图像尺寸
      const int actual_height = cam_data->image.rows;
      const int actual_width = cam_data->image.cols;
      const size_t actual_data_size = actual_height * actual_width * 4;
      
      // **关键修复：线程安全的发布器检查**
      bool has_subscribers = false;
      {
        std::lock_guard<std::mutex> lock(publish_mutex_);
        auto it = image_publishers_.find(camera_name);
        if (it != image_publishers_.end()) {
          has_subscribers = (it->second.getNumSubscribers() > 0);
        }
      }
      
      // **性能优化：无订阅者时跳过图像处理**
      if (!has_subscribers) {
        // 只更新计数，不处理图像
        std::lock_guard<std::mutex> lock(stats_mutex_);
        camera_counters_[camera_name]++;
        return;
      }
      
      // **安全的图像消息创建**
      sensor_msgs::ImagePtr image_msg;
      try {
        image_msg = boost::make_shared<sensor_msgs::Image>();
        if (!image_msg) {
          ROS_ERROR_THROTTLE(5.0, "%s: Failed to create image message", camera_name.c_str());
          return;
        }
        
        image_msg->header.stamp = CreateRosTimestamp(cam_data->time_stamp_us);
        image_msg->header.frame_id = camera_name;
        image_msg->height = actual_height;
        image_msg->width = actual_width;
        image_msg->encoding = "bgr8";
        image_msg->is_bigendian = false;
        image_msg->step = actual_width * 4;
        
        // **安全的内存分配和拷贝**
        image_msg->data.resize(actual_data_size);
        
        // 验证源数据指针
        if (cam_data->image.data) {
          memcpy(image_msg->data.data(), cam_data->image.data, actual_data_size);
        } else {
          ROS_ERROR_THROTTLE(5.0, "%s: NULL image data pointer", camera_name.c_str());
          return;
        }
        
        // **线程安全发布**
        {
          std::lock_guard<std::mutex> lock(publish_mutex_);
          auto it = image_publishers_.find(camera_name);
          if (it != image_publishers_.end() && it->second.getNumSubscribers() > 0) {
            it->second.publish(image_msg);
          }
        }
        
        // 更新统计
        {
          std::lock_guard<std::mutex> lock(stats_mutex_);
          camera_counters_[camera_name]++;
        }
        
      } catch (const std::bad_alloc& e) {
        ROS_ERROR_THROTTLE(5.0, "%s: Memory allocation failed: %s", camera_name.c_str(), e.what());
      } catch (const std::exception& e) {
        ROS_ERROR_THROTTLE(5.0, "%s: Image processing error: %s", camera_name.c_str(), e.what());
      }
      
    } catch (const std::exception& e) {
      ROS_ERROR_THROTTLE(5.0, "%s: Callback error: %s", camera_name.c_str(), e.what());
    } catch (...) {
      ROS_ERROR_THROTTLE(5.0, "%s: Unknown callback error", camera_name.c_str());
    }
  }

  void Init() {
    // 网络配置
    std::string ip_address;
    int port;
    node_.param("sync_ip", ip_address, std::string("192.168.1.188"));
    node_.param("sync_port", port, 8888);
    
    ROS_INFO("Connecting to synchronizer at %s:%d", ip_address.c_str(), port);
    synchronizer_.SetNetLink(ip_address, port);
    
    // 初始化多相机
    mv_cam_ = std::make_shared<MvCam>();
    
    // **关键修改：首先扫描相机名称，不进行实际初始化**
    camera_names_ = mv_cam_->ScanCameraNames();
    if (camera_names_.empty()) {
      ROS_ERROR("No cameras detected during scan");
      throw std::runtime_error("No cameras detected");
    }
    
    ROS_INFO("Detected %zu cameras with user-defined names:", camera_names_.size());
    for (size_t i = 0; i < camera_names_.size(); ++i) {
      ROS_INFO("  Camera %zu: %s", i, camera_names_[i].c_str());
    }
    
    // **根据实际检测到的相机数量设置配置**
    std::map<std::string, TriggerDevice> camera_configs;
    const std::vector<TriggerDevice> trigger_devices = {CAM_1, CAM_2, CAM_3, CAM_4};
    
    for (size_t i = 0; i < camera_names_.size() && i < trigger_devices.size(); ++i) {
      camera_configs[camera_names_[i]] = trigger_devices[i];
    }
    
    mv_cam_->SetParams(camera_configs);
    synchronizer_.UseSensor(mv_cam_);
    
    // 设置发布器
    imu_pub_ = node_.advertise<sensor_msgs::Imu>(imu_name_, 50);
    
    {
      std::lock_guard<std::mutex> lock(publish_mutex_);
      for (const auto& cam_name : camera_names_) {
        image_publishers_[cam_name] = it_.advertise(cam_name, 5);
        camera_counters_[cam_name] = 0;  // 初始化计数器
        ROS_INFO("Created publisher for camera: %s", cam_name.c_str());
      }
    }
    
    // 启动同步器
    ROS_INFO("Starting synchronizer...");
    synchronizer_.Start();
    
    // **验证实际初始化后的相机名称**
    std::vector<std::string> actual_names = mv_cam_->GetCameraNames();
    if (actual_names.size() != camera_names_.size()) {
      ROS_WARN("Camera count mismatch after initialization: scanned %zu, initialized %zu", 
               camera_names_.size(), actual_names.size());
      camera_names_ = actual_names;  // 使用实际初始化的名称
      
      // 重新创建发布器映射以匹配实际的相机
      {
        std::lock_guard<std::mutex> lock(publish_mutex_);
        image_publishers_.clear();
        camera_counters_.clear();
        
        for (const auto& cam_name : camera_names_) {
          image_publishers_[cam_name] = it_.advertise(cam_name, 5);
          camera_counters_[cam_name] = 0;
          ROS_INFO("Updated publisher for camera: %s", cam_name.c_str());
        }
      }
    }
    
    // 等待稳定
    ROS_INFO("Waiting for system stabilization...");
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    
    // 订阅所有传感器
    ROS_INFO("Subscribing to sensors...");
    
    // 订阅IMU
    Messenger::GetInstance().SubStruct(
        "imu_1", std::bind(&OptimizedMultiCamDriver::ImuCallback, this, 
                          std::placeholders::_1, std::placeholders::_2));
    
    // **动态订阅所有检测到的相机**
    for (const auto& cam_name : camera_names_) {
      Messenger::GetInstance().SubStruct(
          cam_name, [this, cam_name](const void* msg, size_t size) {
            this->ImageCallback(cam_name, msg, size);
          });
      ROS_INFO("Subscribed to camera: %s", cam_name.c_str());
    }
    
    ROS_INFO("Successfully subscribed to all sensors (IMU + %zu cameras)", camera_names_.size());
  }

  void Run() {
    // 高频处理循环 - 确保及时处理回调
    ros::Rate loop_rate(2000);  // 200Hz 确保不会成为瓶颈
    
    ROS_INFO("Optimized multi-camera driver running at 200Hz with %zu cameras...", camera_names_.size());
    auto last_stats_time = ros::Time::now();
    
    while (ros::ok()) {
      ros::spinOnce();
      
      // 每15秒打印一次统计信息
      if (ros::Time::now() - last_stats_time > ros::Duration(15.0)) {
        PrintStats();
        last_stats_time = ros::Time::now();
      }
      
      loop_rate.sleep();
    }
    
    ROS_INFO("Stopping synchronizer...");
    synchronizer_.Stop();
  }

  void PrintStats() {
    try {
      std::lock_guard<std::mutex> stats_lock(stats_mutex_);
      std::lock_guard<std::mutex> pub_lock(publish_mutex_);
      
      ROS_INFO("=== Multi-Camera Driver Statistics ===");
      
      unsigned long total_received = 0;
      
      for (const auto& cam_name : camera_names_) {
        auto counter_it = camera_counters_.find(cam_name);
        if (counter_it != camera_counters_.end()) {
          unsigned long received = counter_it->second;
          total_received += received;
          
          // 安全获取订阅者数量
          int subscribers = 0;
          auto pub_it = image_publishers_.find(cam_name);
          if (pub_it != image_publishers_.end()) {
            subscribers = pub_it->second.getNumSubscribers();
          }
          
          ROS_INFO("%s: %lu frames processed, %d subscribers", 
                   cam_name.c_str(), received, subscribers);
        }
      }
      
      ROS_INFO("Total: %lu frames processed across %zu cameras", total_received, camera_names_.size());
      ROS_INFO("IMU subscribers: %d", imu_pub_.getNumSubscribers());
      ROS_INFO("======================================");
      
    } catch (const std::exception& e) {
      ROS_ERROR("Stats printing error: %s", e.what());
    }
  }

 private:
  ros::NodeHandle &node_;
  ros::Publisher imu_pub_;
  image_transport::ImageTransport it_;
  
  // 线程安全的发布器映射
  std::map<std::string, image_transport::Publisher> image_publishers_;
  
  std::shared_ptr<MvCam> mv_cam_;
  Synchronizer synchronizer_;
  
  // **修改：动态获取的相机名称**
  std::vector<std::string> camera_names_;
  std::string imu_name_;
  
  std::map<std::string, unsigned long> camera_counters_;
  
  // 线程安全保护
  mutable std::mutex publish_mutex_;  // 保护发布器访问
  mutable std::mutex stats_mutex_;    // 保护统计数据
};

int main(int argc, char **argv) {
  ros::init(argc, argv, "optimized_multi_cam_driver", ros::init_options::NoSigintHandler);
  ros::NodeHandle node;
  
  // 启用ROS日志
  if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Info)) {
    ros::console::notifyLoggerLevelsChanged();
  }
  
  // **安全的驱动器创建和运行**
  try {
    auto driver = std::make_unique<OptimizedMultiCamDriver>(node);
    
    driver->Init();
    driver->Run();
    
  } catch (const std::exception& e) {
    ROS_ERROR("Driver exception: %s", e.what());
    return -1;
  } catch (...) {
    ROS_ERROR("Unknown driver exception");
    return -1;
  }
  
  ROS_INFO("Optimized multi-camera driver shutdown complete");
  return 0;
}