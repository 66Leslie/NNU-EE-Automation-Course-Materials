# 安装指南

## 系统要求

- Python 3.8+
- CUDA 11.3+ (推荐，用于GPU加速，CPU版本亦可)
- 至少8GB RAM
- 至少100MB硬盘空间
- Windows 10+ / Linux / macOS

## 使用pip安装

1. 创建虚拟环境：
```bash
# 创建虚拟环境
python -m venv dam_env

# 激活虚拟环境
# Windows
dam_env\Scripts\activate
# Linux/Mac
source dam_env/bin/activate
```

2. 安装依赖：
```bash
pip install -r requirements.txt
```

3. 如果使用GPU加速，请确保安装正确版本的CUDA和cuDNN，对应于requirements.txt中的PyTorch版本。

## 使用Conda安装

1. 创建环境：
```bash
conda env create -f environment.yml
```

2. 激活环境：
```bash
conda activate dam_env
```

## 模型权重

1. 请确保模型权重文件位于正确位置：
   - 裂缝分类模型: `dam_crack_model.pth`
   - 图像增强模型: 
     - `5A/FiveAPlus-Network/model/FAPlusNet-alpha-0.1.pth` 或
     - `models/enhancement/FiveAPlus-Network/model/FAPlusNet-alpha-0.1.pth`

2. 如果模型权重文件缺失，请联系管理员获取。

## 验证安装

运行以下命令验证安装：

```bash
python -c "import torch; import cv2; import flask; print('PyTorch版本:', torch.__version__); print('OpenCV版本:', cv2.__version__); print('Flask版本:', flask.__version__); print('CUDA是否可用:', torch.cuda.is_available())"
```

## 启动应用

```bash
python app.py
```

应用将在 http://127.0.0.1:5000 启动
