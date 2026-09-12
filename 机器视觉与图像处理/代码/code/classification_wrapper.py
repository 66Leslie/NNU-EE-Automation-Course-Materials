import torch
import torch.nn as nn
from torchvision import transforms
from PIL import Image
import os

# 定义数据预处理 - 减小图像尺寸
transform = transforms.Compose([
    transforms.Grayscale(num_output_channels=1),  # 转换为单通道灰度图
    transforms.Resize((224, 224)),               # 减小尺寸到224x224
    transforms.ToTensor(),                       # 转换为Tensor
    transforms.Normalize((0.5,), (0.5,))         # 归一化
])

# 简化的轻量级ViT模型
class LightViT(nn.Module):
    def __init__(self, image_size=224, patch_size=16, num_classes=6, dim=256, depth=6, heads=8):
        super(LightViT, self).__init__()
        self.patch_size = patch_size
        num_patches = (image_size // patch_size) ** 2
        
        # 图像分块和嵌入
        self.patch_embed = nn.Conv2d(1, dim, kernel_size=patch_size, stride=patch_size)
        self.pos_embed = nn.Parameter(torch.randn(1, num_patches + 1, dim))
        self.cls_token = nn.Parameter(torch.randn(1, 1, dim))
        self.dropout = nn.Dropout(0.1)
        
        # 简化的Transformer层
        self.transformer = nn.TransformerEncoder(
            nn.TransformerEncoderLayer(
                d_model=dim, 
                nhead=heads,
                dim_feedforward=dim*4,
                dropout=0.1,
                activation='gelu',
                batch_first=True
            ),
            num_layers=depth
        )
        
        # 分类头
        self.norm = nn.LayerNorm(dim)
        self.head = nn.Linear(dim, num_classes)
        
    def forward(self, x):
        # 分块嵌入
        x = self.patch_embed(x)  # (B, C, H, W) -> (B, dim, H/p, W/p)
        B = x.shape[0]
        x = x.flatten(2).transpose(1, 2)  # (B, num_patches, dim)
        
        # 添加CLS token
        cls_tokens = self.cls_token.expand(B, -1, -1)
        x = torch.cat((cls_tokens, x), dim=1)
        x = x + self.pos_embed
        x = self.dropout(x)
        
        # Transformer 层
        x = self.transformer(x)
        
        # 分类：取CLS token
        x = self.norm(x[:, 0])
        x = self.head(x)
        return x

# 添加ClassificationModel类用于app.py集成
class ClassificationModel:
    def __init__(self, model_path="dam_crack_model.pth"):
        self.model = LightViT()
        self.crack_types = ["网状裂缝", "龟裂型裂缝", "不规则短裂缝", "纵裂缝", "横向裂缝", "斜裂缝"]
        self.transform = transform
        
        # 尝试加载模型权重
        try:
            self.model.load_state_dict(torch.load(model_path))
            self.model.eval()
            self.model_loaded = True
            print("成功加载裂缝分类模型")
        except Exception as e:
            print(f"无法加载裂缝分类模型: {e}")
            self.model_loaded = False
    
    def classify_image(self, image_path):
        """分类图像并返回结果"""
        result = {
            "success": False,
            "class_name": "未知",
            "class_id": -1,
            "error": None
        }
        
        if not self.model_loaded:
            result["error"] = "模型未加载"
            return result
            
        try:
            # 加载并处理图像
            img = Image.open(image_path).convert('RGB')
            img_tensor = self.transform(img).unsqueeze(0)
            
            # 预测
            with torch.no_grad():
                output = self.model(img_tensor)
                _, predicted = torch.max(output, 1)
                pred_class = predicted.item()
            
            result["success"] = True
            result["class_id"] = pred_class
            result["class_name"] = self.crack_types[pred_class]
            
        except Exception as e:
            result["error"] = str(e)
            
        return result
