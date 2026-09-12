# 图像处理 / 机器视觉课程期末论文：代码与复现说明

本目录用于配合期末论文/课程报告提交，包含：
- 第 2 章（2.1–2.4）传统图像处理流程的**可复现实验脚本**（根目录 `2_*.py`）
- 完整系统工程代码（子目录 `code/`，包含 Web 演示、增强/检测/量化等模块）

论文正文（Word/PDF）与中间产物默认不纳入版本控制，详见 `.gitignore`。

## 1. 实验环境
- 操作系统：Windows 10 / 11（64-bit）
- Python：3.8+
- 依赖（按需）：
  - `opencv-python`、`numpy`（运行 `2_*.py` 必需）
  - `matplotlib`（可选：绘图/可视化）
  - `flask`（可选：运行 `code/app.py` Web 演示）
  - `torch` / `torchvision`（可选：启用 Five A+ 增强时需要）

## 2. 目录结构
```text
report/
├── cracks.jpg                     # 示例输入图（可替换为自己的图片）
├── 2_1_smoothing.py               # 2.1 平滑去噪（输出：cracks_2_1_smooth.jpg）
├── 2_2_threshold.py               # 2.2 全局阈值/OTSU（输出：cracks_2_2_threshold.jpg）
├── 2_3_adaptive_threshold.py      # 2.3 自适应阈值（输出：cracks_2_3_adaptive.jpg）
├── 2_4_morphology.py              # 2.4 形态学优化（输出：cracks_2_4_opening.jpg / cracks_2_4_closing.jpg）
├── .gitignore                     # 忽略论文文档/中间结果等
└── code/                          # 完整系统工程代码（Web + 增强/检测/量化）
```

## 3. 运行方式（生成报告 2.1–2.4 的结果图）
在本目录下执行（默认读取同目录 `cracks.jpg`；也可在命令行传入图片路径）：
```bash
python 2_1_smoothing.py
python 2_2_threshold.py
python 2_3_adaptive_threshold.py
python 2_4_morphology.py
```
