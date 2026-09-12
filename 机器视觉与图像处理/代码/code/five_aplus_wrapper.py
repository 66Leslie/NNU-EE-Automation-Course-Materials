import os
import torch
import torch.nn as nn
import torch.nn.functional as F
from PIL import Image
import torchvision.transforms as transforms
from torchvision.utils import save_image

# FIVE_APLUSNet model components based on open-source codes
class PALayer(nn.Module):
    def __init__(self, channel):
        super(PALayer, self).__init__()
        self.pa = nn.Sequential(
            nn.Conv2d(channel, channel // 8, 1, padding=0, bias=True),
            nn.ReLU(inplace=True),  
            nn.Conv2d(channel // 8, 1, 1, padding=0, bias=True),
            nn.Sigmoid()
        )

    def forward(self, x):
        y = self.pa(x)
        return x * y

# Multi-Scale Pyramid Module
class Enhance(nn.Module):
    def __init__(self):
        super(Enhance, self).__init__()
        self.relu = nn.ReLU(inplace=True)
        self.tanh = nn.Tanh()
        self.refine2 = nn.Conv2d(16, 16, kernel_size=3, stride=1, padding=1)
        self.conv1010 = nn.Conv2d(16, 1, kernel_size=1, stride=1, padding=0)  # 1mm
        self.conv1020 = nn.Conv2d(16, 1, kernel_size=1, stride=1, padding=0)  # 1mm
        self.conv1030 = nn.Conv2d(16, 1, kernel_size=1, stride=1, padding=0)  # 1mm
        self.refine3 = nn.Conv2d(16+3, 16, kernel_size=3, stride=1, padding=1)

    def forward(self, x):
        dehaze = self.relu((self.refine2(x)))
        shape_out = dehaze.data.size()
        shape_out = shape_out[2:4]

        # 使用自适应平均池化代替固定大小的池化
        x101 = F.adaptive_avg_pool2d(dehaze, (1, 5))
        x102 = F.adaptive_avg_pool2d(dehaze, (1, 5))
        x103 = F.adaptive_avg_pool2d(dehaze, (1, 5))

        # 使用F.interpolate替代旧的upsample_nearest
        x1010 = F.interpolate(self.relu(self.conv1010(x101)), size=shape_out, mode='nearest')
        x1020 = F.interpolate(self.relu(self.conv1020(x102)), size=shape_out, mode='nearest')
        x1030 = F.interpolate(self.relu(self.conv1030(x103)), size=shape_out, mode='nearest')

        dehaze = torch.cat((x1010, x1020, x1030, dehaze), 1)
        dehaze = self.tanh(self.refine3(dehaze))

        return dehaze

class FreBlock(nn.Module):
    def __init__(self):
        super(FreBlock, self).__init__()

    def forward(self, x):
        x = x + 1e-8
        mag = torch.abs(x)
        pha = torch.angle(x)
        return mag, pha

class SFDIM(nn.Module):
    def __init__(self, n_feats):   
        super().__init__()
        self.Conv1 = nn.Sequential(
            nn.Conv2d(n_feats, 2*n_feats, 1, 1, 0),
            nn.LeakyReLU(0.1, inplace=True),
            nn.Conv2d(2*n_feats, n_feats, 1, 1, 0)) 
        self.Conv1_1 = nn.Sequential(
            nn.Conv2d(n_feats, 2*n_feats, 1, 1, 0),
            nn.LeakyReLU(0.1, inplace=True),
            nn.Conv2d(2*n_feats, n_feats, 1, 1, 0)) 
    
        self.Conv2 = nn.Conv2d(n_feats, n_feats, 1, 1, 0)
        self.FF = FreBlock()
        self.scale = nn.Parameter(torch.zeros((1, n_feats, 1, 1)), requires_grad=True)
        
    def forward(self, x, y):     
        b, c, H, W = x.shape 
        a = 0.1
        mix = x + y
        mix_mag, mix_pha = self.FF(mix)
        # Ghost Expand      
        mix_mag = self.Conv1(mix_mag)
        mix_pha = self.Conv1_1(mix_pha)

        real_main = mix_mag * torch.cos(mix_pha)
        imag_main = mix_mag * torch.sin(mix_pha)
        x_out_main = torch.complex(real_main, imag_main)
        x_out_main = torch.abs(torch.fft.irfft2(x_out_main, s=(H, W), norm='backward')) + 1e-8

        return self.Conv2(a*x_out_main + (1-a)*mix)

# Multi-branch Color Enhancement Module
class MCEM(nn.Module):
    def __init__(self, in_channels, channels):
        super(MCEM, self).__init__()
        self.conv_first_r = nn.Conv2d(in_channels//4, channels//2, kernel_size=1, stride=1, padding=0, bias=False)
        self.conv_first_g = nn.Conv2d(in_channels//4, channels//2, kernel_size=1, stride=1, padding=0, bias=False)
        self.conv_first_b = nn.Conv2d(in_channels//4, channels//2, kernel_size=1, stride=1, padding=0, bias=False)
        self.instance_r = nn.InstanceNorm2d(channels//2, affine=True)
        self.instance_g = nn.InstanceNorm2d(channels//2, affine=True)
        self.instance_b = nn.InstanceNorm2d(channels//2, affine=True)
        
        self.conv_out_r = nn.Conv2d(channels//2, in_channels//4, kernel_size=1, stride=1, padding=0, bias=False)
        self.conv_out_g = nn.Conv2d(channels//2, in_channels//4, kernel_size=1, stride=1, padding=0, bias=False)
        self.conv_out_b = nn.Conv2d(channels//2, in_channels//4, kernel_size=1, stride=1, padding=0, bias=False)

    def forward(self, x):
        x1, x2, x3, x4 = torch.chunk(x, 4, dim=1)
        
        x_1 = self.conv_first_r(x1)
        x_2 = self.conv_first_g(x2)
        x_3 = self.conv_first_b(x3)
        
        out_instance_r = self.instance_r(x_1)
        out_instance_g = self.instance_g(x_2)
        out_instance_b = self.instance_b(x_3)

        out_instance_r = self.conv_out_r(out_instance_r)
        out_instance_g = self.conv_out_g(out_instance_g)
        out_instance_b = self.conv_out_b(out_instance_b)

        mix = out_instance_r + out_instance_g + out_instance_b + x4
        
        out_instance = torch.cat((out_instance_r, out_instance_g, out_instance_b, mix), dim=1)

        return out_instance

class MCEM_2(nn.Module):
    def __init__(self, in_channels, channels):
        super(MCEM_2, self).__init__()
        self.conv_first_r = nn.Conv2d(in_channels//4, channels//2, kernel_size=1, stride=1, padding=0, bias=False)
        self.conv_first_g = nn.Conv2d(in_channels//4, channels//2, kernel_size=1, stride=1, padding=0, bias=False)
        self.conv_first_b = nn.Conv2d(in_channels//4, channels//2, kernel_size=1, stride=1, padding=0, bias=False)
        self.instance_r = nn.InstanceNorm2d(channels//2, affine=True)
        self.instance_g = nn.InstanceNorm2d(channels//2, affine=True)
        self.instance_b = nn.InstanceNorm2d(channels//2, affine=True)
        
        self.conv_out_r = nn.Conv2d(channels//2, in_channels//4, kernel_size=1, stride=1, padding=0, bias=False)
        self.conv_out_g = nn.Conv2d(channels//2, in_channels//4, kernel_size=1, stride=1, padding=0, bias=False)
        self.conv_out_b = nn.Conv2d(channels//2, in_channels//4, kernel_size=1, stride=1, padding=0, bias=False)

    def forward(self, x):
        x1, x2, x3, x4 = torch.chunk(x, 4, dim=1)
        
        x_1 = self.conv_first_r(x1)
        x_2 = self.conv_first_g(x2)
        x_3 = self.conv_first_b(x3)
        
        out_instance_r = self.instance_r(x_1)
        out_instance_g = self.instance_g(x_2)
        out_instance_b = self.instance_b(x_3)

        out_instance_r = self.conv_out_r(out_instance_r)
        out_instance_g = self.conv_out_g(out_instance_g)
        out_instance_b = self.conv_out_b(out_instance_b)

        mix = out_instance_r + out_instance_g + out_instance_b + x4
        
        out_instance = torch.cat((out_instance_r, out_instance_g, out_instance_b, mix), dim=1)

        return out_instance

# MAIN-Net
class FIVE_APLUSNet(nn.Module):
    def __init__(self, in_nc=3, out_nc=3, base_nf=16):
        super(FIVE_APLUSNet, self).__init__()

        self.base_nf = base_nf
        self.out_nc = out_nc
        self.pyramid_enhance = Enhance()
        self.color_cer_1 = MCEM(base_nf, base_nf*2)
        self.color_cer_2 = MCEM_2(base_nf, base_nf*2)
       
        self.fusion_mixer = SFDIM(base_nf)
        self.conv1 = nn.Conv2d(in_nc, base_nf, 1, 1, bias=True) 
        self.conv2 = nn.Conv2d(base_nf, base_nf, 1, 1, bias=True)
        self.conv3 = nn.Conv2d(base_nf, out_nc, 1, 1, bias=True)
        self.conv4 = nn.Conv2d(base_nf, out_nc, 1, 1, bias=True)
        self.stage2 = PALayer(base_nf)
        self.act = nn.ReLU(inplace=True)

    def forward(self, x):
        out = self.conv1(x)
        out_1 = self.color_cer_1(out)
        out_2 = self.pyramid_enhance(out)
        mix_out = self.fusion_mixer(out_1, out_2)

        out_stage2 = self.act(mix_out)
        out_stage2_head = self.conv4(out_stage2)

        out_stage2 = self.conv2(out_stage2)
        out_stage2 = self.color_cer_2(out_stage2)
        out = self.stage2(out_stage2)
        out = self.act(out)

        out = self.conv3(out)

        return out, out_stage2_head

# Wrapper class for the FIVE_APLUSNet model to use in app.py
class FiveAPlusEnhancer:
    def __init__(self, model_path="5A/FiveAPlus-Network/model/FAPlusNet-alpha-0.1.pth"):
        # 检查模型文件是否存在
        if not os.path.exists(model_path):
            # 尝试备用路径
            alt_path = "5A/FiveAPlus-Network/model/FAPlusNet-alpha-0.1.pth"
            if os.path.exists(alt_path):
                model_path = alt_path
            else:
                print(f"警告：模型文件在两个路径都不存在: {model_path} 和 {alt_path}")
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        self.model = FIVE_APLUSNet().to(self.device)
        self.transform = transforms.Compose([
            transforms.ToTensor()
        ])
        
        # Try to load model weights
        try:
            ckpt = torch.load(model_path, map_location=self.device)
            self.model.load_state_dict(ckpt)
            self.model.eval()
            self.model_loaded = True
            print("成功加载FIVE_APLUSNet图像增强模型")
        except Exception as e:
            print(f"无法加载FIVE_APLUSNet图像增强模型: {e}")
            self.model_loaded = False
    
    def enhance_image(self, image_path, timeout=30):
        """
        Enhance an underwater image using the FIVE_APLUSNet model
        
        Args:
            image_path: 图像文件路径
            timeout: 处理超时时间（秒）
        """
        result = {
            "success": False,
            "enhanced_image": None,
            "error": None
        }
        
        # 检查模型是否已加载
        if not self.model_loaded:
            result["error"] = "模型未加载，请检查模型文件路径"
            print(f"错误: {result['error']}")
            return result
        
        # 检查输入文件是否存在
        if not os.path.exists(image_path):
            result["error"] = f"输入图像文件不存在: {image_path}"
            print(f"错误: {result['error']}")
            return result
            
        try:
            # 设置超时处理（Windows兼容版本）
            import threading
            from functools import wraps
            
            def timeout_handler(seconds):
                def decorator(func):
                    @wraps(func)
                    def wrapper(*args, **kwargs):
                        result = [TimeoutError(f"图像处理超时（{seconds}秒）"), None]
                        
                        def target():
                            try:
                                result[1] = func(*args, **kwargs)
                                result[0] = None
                            except Exception as e:
                                result[0] = e
                        
                        t = threading.Thread(target=target)
                        t.daemon = True
                        t.start()
                        t.join(seconds)
                        
                        if t.is_alive():
                            # 线程仍在运行，超时
                            return result[0], None
                        
                        if result[0]:
                            # 有异常
                            raise result[0]
                        
                        return None, result[1]
                    return wrapper
                return decorator
            
            # 加载并处理图像
            try:
                img = Image.open(image_path).convert('RGB')
            except Exception as e:
                result["error"] = f"无法打开或转换图像: {str(e)}"
                print(f"错误: {result['error']}")
                return result
                
            img_tensor = self.transform(img).unsqueeze(0).to(self.device)
            
            # 使用超时限制处理模型
            @timeout_handler(timeout)
            def process_model(model, img_tensor):
                with torch.no_grad():
                    return model(img_tensor)
            
            try:
                error, model_output = process_model(self.model, img_tensor)
                if error:
                    result["error"] = f"网络响应错误: {str(error)}"
                    print(f"错误: {result['error']}")
                    return result
                enhanced_img, _ = model_output
            except Exception as e:
                result["error"] = f"模型处理错误: {str(e)}"
                print(f"错误: {result['error']}")
                return result
            
            # 保存增强图像
            try:
                output_path = image_path.replace('.', '_enhanced.')
                save_image(enhanced_img, output_path, normalize=False)
                
                # 验证输出文件是否成功创建
                if not os.path.exists(output_path):
                    result["error"] = "无法保存增强图像"
                    print(f"错误: {result['error']}")
                    return result
                    
                result["success"] = True
                result["enhanced_image"] = output_path
                print(f"成功: 图像已增强并保存到 {output_path}")
            except Exception as e:
                result["error"] = f"保存图像错误: {str(e)}"
                print(f"错误: {result['error']}")
                
        except Exception as e:
            result["error"] = f"网络响应错误: {str(e)}"
            print(f"错误: {result['error']}")
            
        return result
