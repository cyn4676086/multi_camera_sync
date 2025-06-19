#include "mv_cam.h"
#include "infinite_sense.h"
#include "MvCameraControl.h"
#include <cstring>        // for memset, strlen
#include <string>         // for std::string
#include <vector>         // for std::vector

namespace infinite_sense {

bool IsColor(const MvGvspPixelType type) {
  switch (type) {
    case PixelType_Gvsp_BGR8_Packed:
    case PixelType_Gvsp_YUV422_Packed:
    case PixelType_Gvsp_YUV422_YUYV_Packed:
    case PixelType_Gvsp_BayerGR8:
    case PixelType_Gvsp_BayerRG8:
    case PixelType_Gvsp_BayerGB8:
    case PixelType_Gvsp_BayerBG8:
    case PixelType_Gvsp_BayerGB10:
    case PixelType_Gvsp_BayerGB10_Packed:
    case PixelType_Gvsp_BayerBG10:
    case PixelType_Gvsp_BayerBG10_Packed:
    case PixelType_Gvsp_BayerRG10:
    case PixelType_Gvsp_BayerRG10_Packed:
    case PixelType_Gvsp_BayerGR10:
    case PixelType_Gvsp_BayerGR10_Packed:
    case PixelType_Gvsp_BayerGB12:
    case PixelType_Gvsp_BayerGB12_Packed:
    case PixelType_Gvsp_BayerBG12:
    case PixelType_Gvsp_BayerBG12_Packed:
    case PixelType_Gvsp_BayerRG12:
    case PixelType_Gvsp_BayerRG12_Packed:
    case PixelType_Gvsp_BayerGR12:
    case PixelType_Gvsp_BayerGR12_Packed:
      return true;
    default:
      return false;
  }
}

bool IsBayer(const MvGvspPixelType type) {
  switch (type) {
    case PixelType_Gvsp_BayerGR8:
    case PixelType_Gvsp_BayerRG8:
    case PixelType_Gvsp_BayerGB8:
    case PixelType_Gvsp_BayerBG8:
    case PixelType_Gvsp_BayerGB10:
    case PixelType_Gvsp_BayerGB10_Packed:
    case PixelType_Gvsp_BayerBG10:
    case PixelType_Gvsp_BayerBG10_Packed:
    case PixelType_Gvsp_BayerRG10:
    case PixelType_Gvsp_BayerRG10_Packed:
    case PixelType_Gvsp_BayerGR10:
    case PixelType_Gvsp_BayerGR10_Packed:
    case PixelType_Gvsp_BayerGB12:
    case PixelType_Gvsp_BayerGB12_Packed:
    case PixelType_Gvsp_BayerBG12:
    case PixelType_Gvsp_BayerBG12_Packed:
    case PixelType_Gvsp_BayerRG12:
    case PixelType_Gvsp_BayerRG12_Packed:
    case PixelType_Gvsp_BayerGR12:
    case PixelType_Gvsp_BayerGR12_Packed:
      return true;
    default:
      return false;
  }
}

bool IsMono(const MvGvspPixelType type) {
  switch (type) {
    case PixelType_Gvsp_Mono8:
    case PixelType_Gvsp_Mono10:
    case PixelType_Gvsp_Mono10_Packed:
    case PixelType_Gvsp_Mono12:
    case PixelType_Gvsp_Mono12_Packed:
      return true;
    default:
      return false;
  }
}

bool PrintDeviceInfo(const MV_CC_DEVICE_INFO *info) {
  if (info == nullptr) {
    LOG(WARNING) << "[WARNING] Failed to get camera details. Skipping...";
    return false;
  }
  LOG(INFO) << "-------------------------------------------------------------";
  LOG(INFO) << "----------------------Camera Device Info---------------------";
  if (info->nTLayerType == MV_GIGE_DEVICE) {
    const unsigned int n_ip1 = ((info->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
    const unsigned int n_ip2 = ((info->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
    const unsigned int n_ip3 = ((info->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
    const unsigned int n_ip4 = (info->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);
    LOG(INFO) << "  Device Model Name    : " << info->SpecialInfo.stGigEInfo.chModelName;
    LOG(INFO) << "  Current IP Address   : " << n_ip1 << "." << n_ip2 << "." << n_ip3 << "." << n_ip4;
    LOG(INFO) << "  User Defined Name    : " << info->SpecialInfo.stGigEInfo.chUserDefinedName;
  } else if (info->nTLayerType == MV_USB_DEVICE) {
    LOG(INFO) << "  Device Model Name    : " << info->SpecialInfo.stUsb3VInfo.chModelName;
    LOG(INFO) << "  User Defined Name    : " << info->SpecialInfo.stUsb3VInfo.chUserDefinedName;
  } else {
    LOG(ERROR) << "[ERROR] Unsupported camera type!";
    return false;
  }
  LOG(INFO) << "-------------------------------------------------------------";
  return true;
}

MvCam::~MvCam() { 
  Stop(); 
}

// **新增：扫描相机名称但不初始化设备**
std::vector<std::string> MvCam::ScanCameraNames() {
  std::vector<std::string> names;
  int n_ret = MV_OK;
  MV_CC_DEVICE_INFO_LIST st_device_list{};
  memset(&st_device_list, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
  
  n_ret = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &st_device_list);
  if (MV_OK != n_ret) {
    LOG(ERROR) << "MV_CC_EnumDevices fail! n_ret [" << n_ret << "]";
    return names;
  }
  
  if (st_device_list.nDeviceNum > 0) {
    for (size_t i = 0; i < st_device_list.nDeviceNum; i++) {
      MV_CC_DEVICE_INFO *p_device_info = st_device_list.pDeviceInfo[i];
      if (nullptr == p_device_info) {
        continue;
      }
      PrintDeviceInfo(p_device_info);
      
      // 临时创建句柄只为获取名称
      void* temp_handle = nullptr;
      n_ret = MV_CC_CreateHandleWithoutLog(&temp_handle, p_device_info);
      if (MV_OK != n_ret) {
        LOG(ERROR) << "MV_CC_CreateHandle fail for name scan! n_ret [" << n_ret << "]";
        names.push_back("cam_" + std::to_string(i + 1));
        continue;
      }
      
      // 打开设备获取名称
      n_ret = MV_CC_OpenDevice(temp_handle);
      if (MV_OK != n_ret) {
        LOG(ERROR) << "MV_CC_OpenDevice fail for name scan! n_ret [" << n_ret << "]";
        MV_CC_DestroyHandle(temp_handle);
        names.push_back("cam_" + std::to_string(i + 1));
        continue;
      }
      
      // 获取用户自定义名称
      MVCC_STRINGVALUE pst_value;
      memset(&pst_value, 0, sizeof(MVCC_STRINGVALUE));
      n_ret = MV_CC_GetDeviceUserID(temp_handle, &pst_value);
      
      std::string camera_name;
      if (n_ret == MV_OK && strlen(pst_value.chCurValue) > 0) {
        camera_name = std::string(pst_value.chCurValue);
        LOG(INFO) << "Camera " << i << " user defined name: " << camera_name;
      } else {
        camera_name = "cam_" + std::to_string(i + 1);
        LOG(WARNING) << "Camera " << i << " has no user defined name, using: " << camera_name;
      }
      names.push_back(camera_name);
      
      // 清理临时句柄
      MV_CC_CloseDevice(temp_handle);
      MV_CC_DestroyHandle(temp_handle);
    }
  } else {
    LOG(ERROR) << "Not find GIGE or USB Cam Devices!";
  }
  
  LOG(INFO) << "Number of cameras detected for name scan: " << st_device_list.nDeviceNum;
  return names;
}

bool MvCam::Initialization() {
  int n_ret = MV_OK;
  MV_CC_DEVICE_INFO_LIST st_device_list{};
  memset(&st_device_list, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
  n_ret = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &st_device_list);
  if (MV_OK != n_ret) {
    LOG(ERROR) << "MV_CC_EnumDevices fail! n_ret [" << n_ret << "]";
  }
  if (st_device_list.nDeviceNum > 0) {
    for (size_t i = 0; i < st_device_list.nDeviceNum; i++) {
      MV_CC_DEVICE_INFO *p_device_info = st_device_list.pDeviceInfo[i];
      if (nullptr == p_device_info) {
        continue;
      }
      PrintDeviceInfo(p_device_info);
    }
  } else {
    LOG(ERROR) << "Not find GIGE or USB Cam Devices!";
    return false;
  }
  LOG(INFO) << "Number of cameras detected : " << st_device_list.nDeviceNum;
  for (size_t i = 0; i < st_device_list.nDeviceNum; i++) {
    handles_.emplace_back(nullptr);
    rets_.push_back(MV_OK);
  }
  
  // 清空之前的相机名称
  {
    std::lock_guard<std::mutex> lock(names_mutex_);
    camera_names_.clear();
  }
  
  // 初始化所有检测到的设备
  for (unsigned int i = 0; i < st_device_list.nDeviceNum; ++i) {
    // 创建句柄
    rets_[i] = MV_CC_CreateHandleWithoutLog(&handles_[i], st_device_list.pDeviceInfo[i]);
    if (MV_OK != rets_[i]) {
      LOG(ERROR) << "MV_CC_CreateHandle fail! n_ret [" << rets_[i] << "]";
    }
    
    // 打开设备
    rets_[i] = MV_CC_OpenDevice(handles_[i]);
    if (MV_OK != rets_[i]) {
      LOG(ERROR) << "MV_CC_OpenDevice fail! n_ret [" << rets_[i] << "]";
    }
    
    // **获取相机用户自定义名称并存储**
    MVCC_STRINGVALUE pst_value;
    memset(&pst_value, 0, sizeof(MVCC_STRINGVALUE));
    n_ret = MV_CC_GetDeviceUserID(handles_[i], &pst_value);
    
    std::string camera_name;
    if (n_ret == MV_OK && strlen(pst_value.chCurValue) > 0) {
      camera_name = std::string(pst_value.chCurValue);
      LOG(INFO) << "Camera " << i << " user defined name: " << camera_name;
    } else {
      // 如果没有用户自定义名称，使用默认名称
      camera_name = "cam_" + std::to_string(i + 1);
      LOG(WARNING) << "Camera " << i << " has no user defined name, using: " << camera_name;
    }
    
    {
      std::lock_guard<std::mutex> lock(names_mutex_);
      camera_names_.push_back(camera_name);
    }
    
    // 设置网络包大小（仅对GigE相机有效）
    if (st_device_list.pDeviceInfo[i]->nTLayerType == MV_GIGE_DEVICE) {
      int n_packet_size = MV_CC_GetOptimalPacketSize(handles_[i]);
      if (n_packet_size > 0) {
        rets_[i] = MV_CC_SetIntValue(handles_[i], "GevSCPSPacketSize", n_packet_size);
        if (rets_[i] != MV_OK) {
          LOG(WARNING) << "Set Packet Size fail n_ret [0x" << std::hex << rets_[i] << "]";
        } else {
          LOG(INFO) << camera_name << " packet size set to " << n_packet_size;
        }
      } else {
        LOG(WARNING) << "Get Packet Size fail n_ret [0x" << std::hex << n_packet_size << "]";
      }
    }
    
    // 设置缓存节点数量
    rets_[i] = MV_CC_SetImageNodeNum(handles_[i], 300);
    if (MV_OK != rets_[i]) {
      LOG(ERROR) << "MV_CC_SetImageNodeNum fail! n_ret [" << rets_[i] << "]";
    } else {
      LOG(INFO) << camera_name << " buffer nodes set to 300";
    }
    
    // GigE相机特定优化
    if (st_device_list.pDeviceInfo[i]->nTLayerType == MV_GIGE_DEVICE) {
      // 设置包延迟
      rets_[i] = MV_CC_SetIntValue(handles_[i], "GevSCPD", 200);
      if (rets_[i] != MV_OK) {
        LOG(WARNING) << "Set GevSCPD fail n_ret [0x" << std::hex << rets_[i] << "]";
      }
    }
    
    // 开始抓图
    rets_[i] = MV_CC_StartGrabbing(handles_[i]);
    if (MV_OK != rets_[i]) {
      LOG(ERROR) << "MV_CC_StartGrabbing fail! n_ret [" << rets_[i] << "]";
    }
    LOG(INFO) << camera_name << " initialized successfully.";
  }
  
  return (st_device_list.nDeviceNum > 0);
}

// **获取已初始化相机的名称**
std::vector<std::string> MvCam::GetCameraNames() const {
  std::lock_guard<std::mutex> lock(names_mutex_);
  return camera_names_;
}

// **新增：扫描相机名称但不初始化设备**

void MvCam::Stop() {
  Disable();
  std::this_thread::sleep_for(std::chrono::milliseconds{500});
  
  // **修复：安全地停止所有相机线程**
  for (auto &cam_thread : cam_threads) {
    if (cam_thread.joinable()) {
      try {
        cam_thread.join();
      } catch (const std::exception& e) {
        LOG(ERROR) << "Thread join error: " << e.what();
      }
    }
  }
  cam_threads.clear();
  
  // **修复：安全地关闭所有相机**
  for (size_t i = 0; i < handles_.size(); ++i) {
    if (handles_[i] != nullptr) {
      int n_ret = MV_OK;
      n_ret = MV_CC_StopGrabbing(handles_[i]);
      if (MV_OK != n_ret) {
        LOG(ERROR) << "MV_CC_StopGrabbing fail! n_ret [" << n_ret << "]";
      }
      n_ret = MV_CC_CloseDevice(handles_[i]);
      if (MV_OK != n_ret) {
        LOG(ERROR) << "MV_CC_CloseDevice fail! n_ret [" << n_ret << "]";
      }
      MV_CC_DestroyHandle(handles_[i]);
      handles_[i] = nullptr;
      LOG(INFO) << "Exit camera " << i;
    }
  }
}

void MvCam::Receive(void *handle, const std::string &name) {
  if (!handle) {
    LOG(ERROR) << name << " receive thread started with null handle";
    return;
  }
  
  MV_FRAME_OUT st_out_frame;
  CamData cam_data;
  Messenger &messenger = Messenger::GetInstance();
  
  // **修复：安全的内存分配，添加错误检查**
  unsigned char* p_convert_buffer = nullptr;
  const unsigned int MAX_IMAGE_SIZE = 2048 * 1536 * 4;
  
  try {
    p_convert_buffer = (unsigned char*)aligned_alloc(32, MAX_IMAGE_SIZE);
    if (!p_convert_buffer) {
      LOG(ERROR) << name << " failed to allocate conversion buffer";
      return;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << name << " buffer allocation exception: " << e.what();
    return;
  }
  
  unsigned long frame_counter = 0;
  unsigned long consecutive_timeouts = 0;
  unsigned long consecutive_errors = 0;
  
  LOG(INFO) << name << " receive thread started with safety fixes";
  
  while (is_running) {
    try {
      memset(&st_out_frame, 0, sizeof(MV_FRAME_OUT));
      
      // **动态超时调整**
      int timeout_ms = 300;  // 增加基础超时
      if (consecutive_timeouts > 10) {
        timeout_ms = 600;
      }
      
      int n_ret = MV_CC_GetImageBuffer(handle, &st_out_frame, timeout_ms);
      if (n_ret == MV_OK && st_out_frame.pBufAddr != nullptr) {
        frame_counter++;
        consecutive_timeouts = 0;
        consecutive_errors = 0;
        
        // **优化：缓存曝光时间**
        static MVCC_FLOATVALUE cached_expose_time = {0, 0, 10000.0f};
        if (frame_counter % 100 == 0) {
          if (MV_CC_GetExposureTime(handle, &cached_expose_time) != MV_OK) {
            cached_expose_time.fCurValue = 10000.0f;  // 默认值
          }
        }
        
        // 时间戳设置
        if (params.find(name) != params.end()) {
          if (uint64_t time; GET_LAST_TRIGGER_STATUS(params[name], time)) {
            cam_data.time_stamp_us = time + static_cast<uint64_t>(cached_expose_time.fCurValue / 2.);
          }
        }
        
        bool image_processed = false;
        
        // **修复：更安全的像素类型处理**
        try {
          if (IsBayer(st_out_frame.stFrameInfo.enPixelType)) {
            // **动态处理图像尺寸，不做严格限制**
            const unsigned int frame_width = st_out_frame.stFrameInfo.nWidth;
            const unsigned int frame_height = st_out_frame.stFrameInfo.nHeight;
            const unsigned int converted_size = frame_width * frame_height * 4;
            
            // 确保不超过缓冲区大小
            if (converted_size > MAX_IMAGE_SIZE) {
              LOG(WARNING) << name << " frame too large: " << frame_width << "x" << frame_height 
                          << " (max supported: 2048x1536)";
              continue;
            }
            
            MV_CC_PIXEL_CONVERT_PARAM st_convert_param;
            memset(&st_convert_param, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));
            st_convert_param.nWidth = frame_width;
            st_convert_param.nHeight = frame_height;
            st_convert_param.pSrcData = st_out_frame.pBufAddr;
            st_convert_param.nSrcDataLen = st_out_frame.stFrameInfo.nFrameLen;
            st_convert_param.enSrcPixelType = st_out_frame.stFrameInfo.enPixelType;
            st_convert_param.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
            st_convert_param.pDstBuffer = p_convert_buffer;
            st_convert_param.nDstBufferSize = converted_size;
            
            n_ret = MV_CC_ConvertPixelType(handle, &st_convert_param);
            if (MV_OK == n_ret) {
              cam_data.name = name;
              cam_data.image = GMat(frame_height, frame_width,
                                    GMatType<uint8_t, 3>::Type, 
                                    p_convert_buffer);
              
              // **线程安全发布**
              {
                std::lock_guard<std::mutex> lock(messenger_mutex_);
                messenger.PubStruct(name, &cam_data, sizeof(cam_data));
              }
              image_processed = true;
            } else {
              consecutive_errors++;
              if (consecutive_errors % 50 == 1) {
                LOG(ERROR) << name << " convert fail n_ret [0x" << std::hex << n_ret 
                          << "] errors: " << consecutive_errors;
              }
            }
          } else if (st_out_frame.stFrameInfo.enPixelType == PixelType_Gvsp_BGR8_Packed || 
                     st_out_frame.stFrameInfo.enPixelType == PixelType_Gvsp_RGB8_Packed) {
            // **直接使用，动态处理尺寸**
            cam_data.name = name;
            cam_data.image = GMat(st_out_frame.stFrameInfo.nHeight, 
                                  st_out_frame.stFrameInfo.nWidth,
                                  GMatType<uint8_t, 3>::Type, 
                                  st_out_frame.pBufAddr);
            
            {
              std::lock_guard<std::mutex> lock(messenger_mutex_);
              messenger.PubStruct(name, &cam_data, sizeof(cam_data));
            }
            image_processed = true;
          }
          
        } catch (const std::exception& e) {
          consecutive_errors++;
          LOG(ERROR) << name << " image processing exception: " << e.what();
        }
        
        // 减少日志频率，并显示实际分辨率
        if (image_processed && frame_counter % 5000 == 0) {
          LOG(INFO) << name << " processed " << frame_counter << " frames (" 
                    << st_out_frame.stFrameInfo.nWidth << "x" 
                    << st_out_frame.stFrameInfo.nHeight << ")";
        }
        
      } else {
        consecutive_timeouts++;
        
        if (consecutive_timeouts % 200 == 0) {
          LOG(WARNING) << name << " consecutive timeouts: " << consecutive_timeouts 
                       << " (0x" << std::hex << n_ret << ")";
        }
        
        // **增强的自动恢复机制**
        if (consecutive_timeouts > 1000) {  // 增加阈值
          LOG(ERROR) << name << " excessive timeouts, attempting recovery";
          
          try {
            MV_CC_StopGrabbing(handle);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            if (MV_CC_StartGrabbing(handle) == MV_OK) {
              LOG(INFO) << name << " recovery successful";
              consecutive_timeouts = 0;
            } else {
              LOG(ERROR) << name << " recovery failed";
            }
          } catch (const std::exception& e) {
            LOG(ERROR) << name << " recovery exception: " << e.what();
          }
        }
      }
      
      // **安全释放缓冲区**
      if (st_out_frame.pBufAddr != nullptr) {
        MV_CC_FreeImageBuffer(handle, &st_out_frame);
      }
      
      // **智能线程让步**
      if (consecutive_timeouts > 0) {
        std::this_thread::yield();
      }
      
    } catch (const std::exception& e) {
      LOG(ERROR) << name << " receive loop exception: " << e.what();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } catch (...) {
      LOG(ERROR) << name << " unknown receive loop exception";
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  
  // **安全清理**
  if (p_convert_buffer) {
    free(p_convert_buffer);
    p_convert_buffer = nullptr;
  }
  
  LOG(INFO) << name << " receive thread exiting safely, processed " << frame_counter << " frames total";
}

void MvCam::Start() {
  Enable();
  
  // 获取相机名称
  std::vector<std::string> names;
  {
    std::lock_guard<std::mutex> lock(names_mutex_);
    names = camera_names_;
  }
  
  for (size_t i = 0; i < handles_.size(); ++i) {
    void* handle = handles_[i];
    if (!handle) {
      LOG(ERROR) << "Camera " << i << " has null handle, skipping";
      continue;
    }
    
    // 使用预先获取的相机名称
    std::string name = (i < names.size()) ? names[i] : ("cam_" + std::to_string(i + 1));
    LOG(INFO) << "Starting camera " << i << " with name: " << name;
    
    // **修复：安全的线程启动**
    try {
      cam_threads.emplace_back([this, handle, name, i]() {
        try {
          // **设置线程优化（可选）**
          cpu_set_t cpuset;
          CPU_ZERO(&cpuset);
          CPU_SET(i % std::thread::hardware_concurrency(), &cpuset);
          
          pthread_t this_thread = pthread_self();
          if (pthread_setaffinity_np(this_thread, sizeof(cpu_set_t), &cpuset) == 0) {
            LOG(INFO) << name << " thread bound to CPU core " << (i % std::thread::hardware_concurrency());
          }
          
          // 适中的线程优先级
          struct sched_param params;
          params.sched_priority = 50;  // 中等优先级
          if (pthread_setschedparam(this_thread, SCHED_OTHER, &params) == 0) {
            LOG(INFO) << name << " thread priority set";
          }
        } catch (const std::exception& e) {
          LOG(WARNING) << name << " thread optimization failed: " << e.what();
        }
        
        // 运行接收循环
        this->Receive(handle, name);
      });
      
      LOG(INFO) << "Started receive thread for " << name;
      
    } catch (const std::exception& e) {
      LOG(ERROR) << "Failed to start thread for camera " << i << ": " << e.what();
    }
  }
  
  LOG(INFO) << "All " << handles_.size() << " cameras started with user-defined names";
}

}  // namespace infinite_sense