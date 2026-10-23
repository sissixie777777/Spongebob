# Drowsy Sponge 固件

按 [Spongebob README](https://github.com/sissixie777777/Spongebob/blob/9155529475b4fe909f7dd7117acaea084a8bc48e/README.md) 的交互要求实现，并以本地采购表中的已购器件作为默认硬件。采购表仅保留在本地。以下命令均在仓库根目录运行。

## 硬件版本

| 配置 | 主控 | 眼睑驱动 | 霍尔位置 | 振动 |
|---|---|---|---|---|
| `nano`，默认 | Nano ATmega328P，旧 bootloader | SG90 舵机 | 3：全开、半闭、全闭 | 带驱动三线模块 |
| `nano_new_bootloader` | Nano ATmega328P，新 bootloader | 同上 | 同上 | 同上 |
| `xiao` | XIAO ESP32S3 | N20 + L293D | 5：全开至全闭 | L293D 第二通道 |

`nano_demo`、`xiao_demo` 将预热改为 5 秒、首次提醒改为 30 秒、重复提醒间隔改为 10 秒。生产配置分别为 3 分钟、30 分钟、10 分钟。Nano 的硬件连接请使用 [接线说明](WIRING.md)，不要照原仓库的 ESP32/N20 图连接 SG90。

## 已实现行为

- 每 100 ms 采集 MQ135，采用 1/8 指数平滑，串口每秒输出诊断。
- 空气指标达到 1000 开始连续异常计时，降到 900 或以下清零；900～1000 保持已有异常状态。
- 空气越差，眼睑依霍尔位置逐级闭合。Nano 使用三个实际反馈位置；XIAO 使用五个。
- 连续异常 30 分钟后，三次振动（开 500 ms、关 500 ms、开 500 ms、关 500 ms、开 500 ms），同时全闭、停留 300 ms、全开、停留 300 ms，然后回到空气等级对应位置。
- 异常持续时每 10 分钟重复一次；环境恢复立即取消剩余提醒、清零计时、返回全开。
- 上电先寻找全开 H1。MQ135 预热期间不累计异常时间，归位可以同时进行。
- 霍尔消抖 20 ms；动作超过 3 秒、霍尔信号冲突、运动顺序反向或 ADC 持续贴近量程边缘时锁定故障并停止执行器。正常故障恢复需人工检查后串口输入 `r`，或重新上电。
- 所有采样、计时、归位、眨眼和振动都使用无阻塞状态机；毫秒计数回绕可正确处理。没有 `delay()` 或等待目标霍尔的阻塞循环。

MQ135 的 `air_index` 是 `平滑ADC / ADC满量程 × 2000` 的**相对空气指标**，不是 CO₂ ppm。1000/900 只是演示初值。不同供电、分压、模块、温湿度下必须重新标定，不能将两块板的相同指标理解为相同气体浓度。详情见 [标定与验收](CALIBRATION.md)。

## 构建与烧录

安装 [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) 或 VS Code 的 PlatformIO 扩展后，在仓库根目录运行：

```powershell
pio run -e nano
pio run -e nano -t upload --upload-port COM5
pio device monitor --port COM5 --baud 115200
```

将 COM5 换成实际端口。新 bootloader 选择 `nano_new_bootloader`；30 秒演示选择 `nano_demo`；原 README 硬件选择 `xiao` 或 `xiao_demo`。上传新程序之前检查所选配置，软件不会自动识别硬件版本。

本次工作已将 PlatformIO 和编译器安装到项目忽略目录 `.tools/`。在当前机器也可直接使用：

```powershell
.\tools\pio.ps1 run -e nano
.\tools\pio.ps1 run -e nano -t upload --upload-port COM5
.\tools\pio.ps1 device monitor --port COM5 --baud 115200
```

新环境若希望同样隔离工具：`python -m pip install --target .tools/python platformio`，然后使用上述脚本。普通全局安装的 `pio` 也可直接使用本项目。

`firmware/config.h` 集中定义阈值、计时和舵机范围；`firmware/main.cpp` 定义引脚、N20 方向及 PWM。修改后需重新构建、上传。当前不写入 EEPROM，不会保存校准值或断电前异常时间。

## 串口诊断

波特率 115200。发送单个 `s` 输出一次状态；`r` 仅在故障锁定时重试归位和预热，检查卡阻后再执行。

```text
ms=180000 state=NORMAL raw=350 air_index=684 hall_mask=1 position=1 target=1 moving=0 angle=30 vibration=0 fault=0
```

`position` 是最后确认的位置，在两个霍尔之间时会保留旧值；是否仍在该位置应同时查看 `hall_mask` 和 `moving`。霍尔掩码：H1=1、H2=2、H3=4，XIAO 另外有 H4=8、H5=16。

| fault | 含义 | 检查 |
|---|---|---|
| 0 | 无锁定故障 | — |
| 1 | 多个霍尔同时有效 | 磁铁间距、安装位置、短路 |
| 2 | 3 秒未到达目标 | 卡阻、角度范围、目标霍尔及电源 |
| 3 | 霍尔经过顺序与运动方向相反 | 霍尔编号、舵机角度方向、N20 接线 |
| 4 | 预热结束后，ADC 连续 5 秒接近 0 或满量程 | AO、共地、供电、分压或传感器饱和 |

ADC 故障检测不能证明传感器健康：悬空引脚可能读到中间值，某个恒定中间值也无法靠单个 MQ135 自检判定故障。应配合接线检查与实际空气变化验证。

## 测试与文件

```powershell
.\tools\pio.ps1 pkg install --global --tool platformio/toolchain-gccmingw32
.\tools\test.ps1
```

测试直接运行固件的 C++ 控制核心，覆盖计时边界、回绕、阈值回差、位置等级、三次振动、归位、反向顺序、超时、提醒期间恢复和故障重试。`pio run` 默认只构建 Nano；检查全部配置用 `pio run -e nano -e nano_new_bootloader -e nano_demo -e xiao -e xiao_demo`。软件测试不能替代实机验收。

- `firmware/controller.h`：与硬件 API 分离的控制核心。
- `firmware/main.cpp`：Arduino 入口、采样和引脚适配。
- `docs/WIRING.md`：两套硬件接线。
- `docs/CALIBRATION.md`：参数标定与逐项验收。
- `docs/REQUIREMENTS.md`：与上游 README 的要求对应关系。
- `docs/upstream/README.md`：读取时的上游说明快照，commit `9155529475b4fe909f7dd7117acaea084a8bc48e`。

机械 CAD、外壳模型和电源接线是原 README 的硬件设计内容，不由固件生成。此实现不使用显示屏、Wi-Fi、摄像头或 microSD。
