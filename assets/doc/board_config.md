
# 第四代同步板快速上手

## 1. 更新固件
### 1.1 连接TYPC
### 1.2 下载并上传固件
按住BOOT按钮，不要松开，将TYPC插入到电脑上电后，松开BOOT按钮，电脑会自动识别并识别为USB存储设备。将最新的固件移动到U盘中。移动完成后系统会自动删除U盘。这时候最新的固件已经被成功加载了。
### 1.3 重启
按住RST按钮，然后松开，等待5秒左右。同步板USB将会打印如下信息：
```JSON
{"f":"log","t":1003530,"l":"INFO","msg":"[USB-TYPC] Configuration complet! baudrate:92160e0"}
{"f":"cfg","port":8888,"ip":[192,168,1,188],"subnet":[255,255,255,0],"hz_cam_1":1,"hz_cam_2":2,"hz_cam_3":4,"hz_cam_4":8,"hz_imu_2":10,"xtal_diff":0,"uart_0_baud_rate":921600,"uart_1_baud_rate":9600,"uart_2_baud_rate":115200,"use_gps":true,"use_pps":true,"version":400}
{"f":"log","t":1008121,"l":"INFO","msg":"Port parsed: 8888"}
{"f":"log","t":1008753,"l":"INFO","msg":"IP array set: 192.168.1.188"}
{"f":"log","t":1009493,"l":"INFO","msg":"Subnet array set: 255.255.255.0"}
{"f":"log","t":1010282,"l":"INFO","msg":"Device frequencies parsed: [1, 2, 4, 8, 10]"}
{"f":"log","t":1011140,"l":"INFO","msg":"UART baud rates parsed: [921600, 9600, 115200]"}
{"f":"log","t":1012011,"l":"INFO","msg":"GPS and PPS flags parsed: use_gps:1, use_pps:1"}
{"f":"log","t":1012871,"l":"INFO","msg":"Xtal diff set: 0"}
{"f":"log","t":1013492,"l":"INFO","msg":"Version set: 400"}
{"f":"log","t":1013991,"l":"INFO","msg":"Version parsed: 400"}
{"f":"log","t":2014635,"l":"INFO","msg":"[USB-TYPC] Configuration complete! baudrate:921600"}
{"f":"log","t":2596038,"l":"ERROR","msg":"[Ethernet] Turn off ethernet transfer."}
{"f":"log","t":2596851,"l":"ERROR","msg":"[Ethernet] Cable is not connected."}
{"f":"log","t":2597564,"l":"INFO","msg":"[Ethernet] Start using the serial to transfer data!"}
{"f":"log","t":3104973,"l":"INFO","msg":"[LIDAR] Configuration complete! baudrate:9600"}
{"f":"log","t":3205978,"l":"INFO","msg":"[GPS/RTK] Configuration complete!! baudrate:115200"}
{"f":"log","t":3788507,"l":"INFO","msg":"[IMU] Configuration complete!"}
{"f":"GNGGA","g":false,"t":3274884,"d":"$GNGGA,,,,,,0,00,99.99,,,,,,*56\r","pps":0}
{"f":"t","cpu":250000000,"s":1,"t":3799274,"c":5}
{"f":"imu","t":3799274,"c":5,"d":[1.685578,-0.253794,9.859674,0.001018,0.001038,-6.907073e-6,30.99336],"q":[1.000084,-0.001264,-0.008392,-0.000011]}
{"f":"t","cpu":250000000,"s":1,"t":3809274,"c":10}
```
## 2.连接网线
当网线插入同步板后,同步板重新上电后会使用网口传输信息.
如果同步板IP配置为192.168.1.188.则可以通过ping查看网口配置是否成功或者同步板是否在线.
```
 ping 192.168.1.188
```
用户可以使用nc指令查看上传的数据，Linux中nc命令是一个功能强大的网络工具，全称是netcat
```bash
# 192.168.1.188 8888 表示同步板的IP地址和端口
# 打开任意终端，并输入以下指令：
echo “Hello InfiniteSense” | nc -u 192.168.1.188 8888
```

## 3. 配置同步板参数
### 3.1 (可选)串口工具安装：

```bash
sudo apt-get install cutecom # 下载
sudo cutecom                 # 启动
```
### 3.2 参数配置上传到同步板
固件支持网口和串口配置，配置完成后自动重启并加载最新配置。打开任意串口工具发送以下指令：在输入以下指令，然后按回车键发送。

```json lines
{"f":"cfg","port":8888,"ip":[192,168,1,188],"subnet":[255,255,255,0],"hz_cam_1":1,"hz_cam_2":2,"hz_cam_3":4,"hz_cam_4":8,"hz_imu_2":10,"xtal_diff":0,"uart_0_baud_rate":921600,"uart_1_baud_rate":9600,"uart_2_baud_rate":115200,"use_gps":true,"use_pps":true,"version":400}\n
```
对应指令说明：
```json
  "f": "cfg",                 // 配置文件类型标识符（固定为"cfg"）
  "port": 8888,               // 网络通信端口号
  "ip": [192,168,1,188],      // 设备IP地址
  "subnet": [255,255,255,0],  // 子网掩码配置
  "hz_cam_1": 20,             // 相机1触发频率（单位：Hz，建议≤100Hz）
  "hz_cam_2": 30,             // 相机2触发频率（单位：Hz，建议≤100Hz）
  "hz_cam_3": 40,             // 相机3触发频率（单位：Hz，建议≤100Hz）
  "hz_cam_4": 50,             // 相机4触发频率（单位：Hz，建议≤100Hz）
  "hz_imu_2":1                // IMU2触发频率（单位：Hz，建议≤100Hz）
  "uart_0_baud_rate": 921600, // 通讯波特率(Typc)（高速模式，用于时间同步/传感器数据）
  "uart_1_baud_rate": 9600,   // PPS波特率(Lidar)（低速模式，用于雷达同步）
  "uart_2_baud_rate": 115200, // GPS波特率(GPS/RTK)（中速模式，用于GPS数据读取）
  "use_gps": true,            // GPS模块启用标志
  "use_pps": true,            // PPS精确时钟同步信号启用标志
  "xtal_diff":0,              // 晶振偏差修正
  "version": 400              // 固件版本号(V3/MINI：300，V4：400)
```
配置完成后串口助手打印如下信息：
```json
{"f":"log","t":11320243,"l":"INFO","msg":"Config file received!"}
{"f":"log","t":11321532,"l":"INFO","msg":"Config set ok"}
{"f":"cfg","port":8888,"ip":[192,168,1,188],"subnet":[255,255,255,0],"hz_cam_1":1,"hz_cam_2":2,"hz_cam_3":4,"hz_cam_4":8,"hz_imu_2":10,"xtal_diff":0,"uart_0_baud_rate":921600,"uart_1_baud_rate":9600,"uart_2_baud_rate":115200,"use_gps":true,"use_pps":true,"version":400}
{"f":"log","t":11325745,"l":"INFO","msg":"Port parsed: 8888"}
{"f":"log","t":11326439,"l":"INFO","msg":"IP array set: 192.168.1.188"}
{"f":"log","t":11327234,"l":"INFO","msg":"Subnet array set: 255.255.255.0"}
{"f":"log","t":11328054,"l":"INFO","msg":"Device frequencies parsed: [1, 2, 4, 8, 10]"}
{"f":"log","t":11329034,"l":"INFO","msg":"UART baud rates parsed: [921600, 9600, 115200]"}
{"f":"log","t":11329939,"l":"INFO","msg":"GPS and PPS flags parsed: use_gps:1, use_pps:1"}
{"f":"log","t":11330800,"l":"INFO","msg":"Xtal diff set: 0"}
{"f":"log","t":11331380,"l":"INFO","msg":"Version set: 400"}
{"f":"log","t":11331935,"l":"INFO","msg":"Version parsed: 400"}
{"f":"log","t":1003932,"l":"INFO","msg":"[USB-TYPC] Configuration complete! baudrate:921600"}
{"f":"cfg","port":8888,"ip":[192,168,1,188],"subnet":[255,255,255,0],"hz_cam_1":1,"hz_cam_2":2,"hz_cam_3":4,"hz_cam_4":8,"hz_imu_2":10,"xtal_diff":0,"uart_0_baud_rate":921600,"uart_1_baud_rate":9600,"uart_2_baud_rate":115200,"use_gps":true,"use_pps":true,"version":400}
```

### 3.3 恢复出厂设置

由于同步板的配置参数较多，部分用户参数配置错误，导致无法正常工作。因此，需要恢复出厂设置。
1. 更新重置固件SDK2_FIRMWARE_FACTORY_RESET.uf2。
在重置固件中默认使用以下配置命令进行的配置：
```txt
    "port":8888,"ip":[192,168,1,188],"subnet":[255,255,255,0],"hz_cam_1":1,"hz_cam_2":2,"hz_cam_3":4,"hz_cam_4":8,"hz_imu_2":10,"xtal_diff":0,"uart_0_baud_rate":921600,"uart_1_baud_rate":9600,"uart_2_baud_rate":115200,"use_gps":true,"use_pps":true,"version":400
```
2. 重置完成后，重新上电，系统自动加载上述配置。
3. 正常运行后。可以发送3.2节的指令进行配置。
4. 默认配置更新后，刷入最新固件即可。

# 第三代同步板(V3/MINI)
固件支持网口和串口配置，配置完成后自动重启并加载最新配置。打开任意串口工具发送以下指令：
```python
{"f":"cfg","port":8888,"ip":[192,168,1,188],"subnet":[255,255,255,0],"hz_cam_1":10,"uart_0_baud_rate":921600,"uart_1_baud_rate":9600,"uart_2_baud_rate":115200,"use_gps":true,"use_pps":true,"version":300}\n
```

