# 展示素材

这些资源用于 GitHub 首页展示，不参与固件构建。

| 资源 | 用途 |
| --- | --- |
| `hero.gif` | 四阶段角色动图：正常、困倦、异常提醒、恢复 |
| `control-loop.gif` | 采样、判断、机械反馈、提醒四环节动画 |
| `hero-still.png` / `control-loop-still.png` | 与动图对应的静态首帧，便于低动态或离线展示 |
| `specs.svg` | 默认 Nano 方案的时序与位置反馈信息 |
| `divider.svg` / `favicon.svg` | 静态分隔线与原创几何角色图标 |

GIF、SVG 与角色图形由项目内 `tools/build_readme_assets.py` 绘制，按项目 MIT 许可提供。它们是原创示意设计，**不是实物录像、传感器测试数据或机械仿真结果**。动图压缩了工作流程的时间，不表示正式模式的 30 分钟在几秒内完成；提醒阶段的视觉抖动也只是示意，不用来测量振动脉冲。

实际振动时序为 500ms 开、500ms 关、500ms 开、500ms 关、500ms 开；具体交互由固件定义。角色的上电、预热和机械故障分支未在封面动画里逐项展示。

## 重新生成

```bash
pip install Pillow
python tools/build_readme_assets.py
```

支持 Windows 的 Arial 字体与 Linux 的 DejaVu Sans 字体。GIF 使用共享调色板减少闪烁与文件体积，循环播放；需要静态展示时使用对应 `*-still.png`。

SVG 仅使用静态形状与文字，不含脚本、外部资源或 SVG 动画。GitHub 的 SVG 展示不支持内联脚本与动画，动效因此使用 GIF，参见 [GitHub 官方格式说明](https://docs.github.com/en/repositories/working-with-files/using-files/working-with-non-code-files)。

仓库根目录的两张 `spongebob_*_20260929.png` 沿用原项目，展示的是原 XIAO / N20 / 五霍尔概念方案。它们与默认 Nano / SG90 的具体器件不同，图旁已标明；当前接线以 `docs/WIRING.md` 为准。
