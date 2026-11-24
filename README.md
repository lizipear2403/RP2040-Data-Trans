# RP2040-Data-Trans

RP2040 作为 SPI 主机，从 [S3-Cam](https://github.com/lizipear2403/S3-Cam) 读完整帧，通过单路 nRF24L01+ 发给 [P4-Recv](https://github.com/lizipear2403/P4-Recv)。无线驱动使用社区维护的 [RF24 v1.6.2](https://github.com/nRF24/RF24/tree/v1.6.2) Pico SDK 后端。构建使用 Pico SDK 2.x、Arm GNU 工具链、CMake 和 Ninja。

```sh
cmake -S . -B build -G Ninja -DPICO_BOARD=pico -DPICO_SDK_PATH=/path/to/pico-sdk
cmake --build build
```

固件 `build/rp2040_data_trans.uf2` 可用于 BOOTSEL 刷写。初始化 Pico SDK 的 `lib/tinyusb` 子模块，以启用 USB CDC 日志。

首次配置由 CMake 获取固定版本的 RF24。已有同版本本地源码时，可以在配置命令中传入 `-DFETCHCONTENT_SOURCE_DIR_RF24=/path/to/RF24`。

| 信号 | S3 GPIO | RP2040 GPIO |
| --- | ---: | ---: |
| SCK | 42 | 2 |
| MOSI | 41 | 3 |
| MISO | 46 | 4 |
| CS | 47 | 5 |
| READY | 14 | 6 |

两板共地。nRF24 的接线见 `pinout.h`，IRQ 当前可留空。两端默认同用信道 76（2476 MHz），取代草稿的联调信道 90。地址、32 B 包格式、1 字节 CRC、2 Mbps 和自动 ACK 保持既定协议。

USB CDC 日志会报告 JPEG 长度和包数。先断开无线模块也可用日志检查 S3 取帧，得到帧长后再接模块并重启。超过 7140 B 的 JPEG 整帧跳过。遇到一包重试耗尽，停止发送该帧剩余分片并继续下一帧。
