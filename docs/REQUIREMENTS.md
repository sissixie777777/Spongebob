# 要求对应与来源

来源：上游 [README](https://github.com/sissixie777777/Spongebob/blob/9155529475b4fe909f7dd7117acaea084a8bc48e/README.md)，读取 commit `9155529475b4fe909f7dd7117acaea084a8bc48e`；本地 `消费表格(1).xlsx` 的 `BOM_最终方案`。表格为只读，没有修改。

| 上游章节 | 软件要求 | 实现 |
|---|---|---|
| 1、14、39 | 空气状态感知，无显示屏 | `main.cpp` 定期 ADC 采样、平滑，相对指标输出到调试串口 |
| 12、13、15、17、20 | 原方案引脚和驱动 | `SPONGE_XIAO` 独立适配，接线说明记录 ADC 分压修正 |
| 16、19、27 | 霍尔实际位置反馈、归位、定位超时 | `HallFilter`、`Eye`，默认 3 个霍尔，原配置 5 个 |
| 18 | 正反转、停止、PWM | XIAO 的 `applyOutputs()`；Nano 使用 Servo 库逐步调角 |
| 24 | NORMAL / HIGH_AIR_TIMING / ALERT / RECOVERY | `Controller`，增加 WARMUP 与锁定 FAULT |
| 25、26 | 连续 30 分钟、每 10 分钟重复、1000/900 回差 | `AirMonitor`，无阻塞、uint32_t 无符号差值计时 |
| 26 | 恢复时清零、开眼、停止周期性提醒 | `Controller` 取消振动/眨眼、请求 H1、重置异常计时 |
| 28 | 三次 500 ms 震动，间隔 500 ms | `Vibration`，完整持续 2500 ms |
| 29 | 越差越困 | `AirMonitor::level()`，消费表格三档 / 原方案五档 |
| 37 | 30 秒调试模式、传感器观察 | `*_demo` 构建配置与串口诊断，验收步骤见 CALIBRATION |
| 38 | 电机超时保护 | 单次动作预算 3000 ms，运动中改变目标不重置预算 |

采购表与原设计有冲突，因此实现两套配置而不是将 XIAO 的引脚及直流电机代码直接用于 Nano 舵机。默认配置不要求采购表之外的 L293D / N20 或额外两只霍尔。

原 README 中示例包含 `while` 等待霍尔、`delay` 振动和零值时间戳哨兵。本实现保留交互含义，改为协同状态机与显式布尔状态，使提醒中仍能检查传感器和超时。增加测试覆盖从 millis=0 启动与计数回绕。

电源模块、机械结构、SolidWorks 零件、装配体和外壳尺寸属于硬件设计，未生成 CAD 文件。没有新增未被硬件支持的电池电量检测、温湿度补偿或准确 CO₂ ppm 输出。
