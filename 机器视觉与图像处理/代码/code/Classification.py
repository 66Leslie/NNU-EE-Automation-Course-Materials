import torch
import torch.nn as nn
import torch.optim as optim
from torchvision import datasets, transforms
from torch.utils.data import DataLoader
import matplotlib.pyplot as plt
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

# 训练模型函数
def train_model():
    # 加载数据集
    dataset = datasets.ImageFolder(root=r"C:\Users\zhc\Desktop\机设——大坝检测维修\分类数据集", transform=transform)
    dataloader = DataLoader(dataset, batch_size=8, shuffle=True)
    
    # 使用轻量级模型
    model = LightViT()
    
    # 简化优化器设置
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    
    # 使用标准交叉熵损失
    criterion = nn.CrossEntropyLoss()
    
    num_epochs = 10
    print("开始训练模型...")
    
    for epoch in range(num_epochs):
        model.train()
        for images, targets in dataloader:
            optimizer.zero_grad()
            outputs = model(images)
            loss = criterion(outputs, targets)
            loss.backward()
            optimizer.step()
        print(f"完成训练周期 {epoch+1}/{num_epochs}")
    
    print("模型训练完成！")
    torch.save(model.state_dict(), "dam_crack_model.pth")
    return model

# 简化的测试函数
def test_model_simple(model_path, test_data_path):
    # 裂缝类型名称
    crack_types = ["网状裂缝", "龟裂型裂缝", "不规则短裂缝", "纵裂缝", "横向裂缝", "斜裂缝"]
    
    # 加载模型
    model = LightViT()  
    model.load_state_dict(torch.load(model_path))
    model.eval()
    
    # 扫描测试目录
    if os.path.isdir(test_data_path):
        if any(os.path.isdir(os.path.join(test_data_path, d)) for d in os.listdir(test_data_path)):
            # 按类别组织的测试集
            for subdir in os.listdir(test_data_path):
                subdir_path = os.path.join(test_data_path, subdir)
                if not os.path.isdir(subdir_path):
                    continue
                    
                print(f"\n文件夹 {subdir} 中的图片分类结果:")
                for img_file in os.listdir(subdir_path):
                    if not img_file.lower().endswith(('.jpg', '.jpeg', '.png', '.bmp')):
                        continue
                        
                    img_path = os.path.join(subdir_path, img_file)
                    # 处理图像
                    img = Image.open(img_path).convert('RGB')
                    img_tensor = transform(img).unsqueeze(0)
                    
                    # 预测
                    with torch.no_grad():
                        output = model(img_tensor)
                        _, predicted = torch.max(output, 1)
                        pred_class = predicted.item()
                    
                    print(f"图片 {img_file} 预测类型: {crack_types[pred_class]}")
        else:
            # 未按类别组织的测试集
            print("\n测试集图片分类结果:")
            for img_file in os.listdir(test_data_path):
                if not img_file.lower().endswith(('.jpg', '.jpeg', '.png', '.bmp')):
                    continue
                    
                img_path = os.path.join(test_data_path, img_file)
                # 处理图像
                img = Image.open(img_path).convert('RGB')
                img_tensor = transform(img).unsqueeze(0)
                
                # 预测
                with torch.no_grad():
                    output = model(img_tensor)
                    _, predicted = torch.max(output, 1)
                    pred_class = predicted.item()
                
                print(f"图片 {img_file} 预测类型: {crack_types[pred_class]}")
    else:
        print(f"错误: 测试路径 {test_data_path} 不是一个有效的目录")
    
    print("\n分类完成!")

# 主函数
if __name__ == "__main__":
    # 训练模型
    model = train_model()
    
    # 测试模型
    test_data_path = r"C:\Users\zhc\Desktop\机设——大坝检测维修\分类测试集"
    test_model_simple("dam_crack_model.pth", test_data_path)