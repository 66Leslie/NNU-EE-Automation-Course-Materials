import os
import sys
import numpy as np
import cv2
from flask import Flask, render_template, request, redirect, url_for, flash, jsonify
from werkzeug.utils import secure_filename
import torch
import torch.nn as nn
import torch.nn.functional as F
from torchvision import transforms
from PIL import Image
import time
import math
import random
import matplotlib.pyplot as plt
from numpy.ma import cos, sin
from torchvision.utils import save_image

# Add model directories to Python path
model_paths = [
    os.path.join(os.path.dirname(os.path.abspath(__file__)), 'models', 'enhancement', 'FiveAPlus-Network'),
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '5A', 'FiveAPlus-Network')
]

for path in model_paths:
    if os.path.exists(path) and path not in sys.path:
        sys.path.append(path)
        print(f"Added {path} to Python path")

# Define model weights path
MODEL_WEIGHTS_PATHS = [
    os.path.join(os.path.dirname(os.path.abspath(__file__)), 'models', 'enhancement', 'FiveAPlus-Network', 'model', 'FAPlusNet-alpha-0.1.pth'),
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '5A', 'FiveAPlus-Network', 'model', 'FAPlusNet-alpha-0.1.pth')
]

# Find first available model weights
model_weights_path = None
for path in MODEL_WEIGHTS_PATHS:
    if os.path.exists(path):
        model_weights_path = path
        print(f"Found model weights at: {path}")
        break

# Import FIVE_APLUSNet model if available in the system path
try:
    from archs.FIVE_APLUS import FIVE_APLUSNet
    has_five_aplus = True
    print("Successfully imported FIVE_APLUSNet model")
except ImportError as e:
    has_five_aplus = False
    print(f"Warning: FIVE_APLUSNet model not found. Using fallback enhancement model. Error: {e}")

# Import Classification model
from classification_wrapper import ClassificationModel

# Import WLdetect model
from WLdetect import WLdetectModel

# Initialize width detection model
width_detection_model = WLdetectModel()

app = Flask(__name__)
app.config['SECRET_KEY'] = 'dam_crack_detection_secret_key'
app.config['UPLOAD_FOLDER'] = 'static/uploads'
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 限制上传文件大小为16MB
ALLOWED_EXTENSIONS = {'png', 'jpg', 'jpeg', 'bmp'}

# 确保上传文件夹存在
os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)

# 检查文件扩展名是否允许
def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

# 裂缝检测函数
def detect_cracks(image_path, min_area=200, threshold_value=11, threshold_offset=2, close_kernel_size=5, close_iterations=2):
    # 读取图像
    img = cv2.imread(image_path)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    # 预处理: 高斯模糊减少噪声
    blur = cv2.GaussianBlur(gray, (5, 5), 0)
    
    # 使用自适应阈值分割
    binary = cv2.adaptiveThreshold(blur, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, 
                                  cv2.THRESH_BINARY_INV, threshold_value, threshold_offset)
    
    # 形态学操作: 开运算去除小噪点
    kernel = np.ones((3, 3), np.uint8)
    opening = cv2.morphologyEx(binary, cv2.MORPH_OPEN, kernel, iterations=1)
    
    # 形态学操作: 闭运算连接断开的裂缝
    kernel_close = np.ones((close_kernel_size, close_kernel_size), np.uint8)
    closing = cv2.morphologyEx(opening, cv2.MORPH_CLOSE, kernel_close, iterations=close_iterations)
    
    # 寻找轮廓
    contours, _ = cv2.findContours(closing.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    # 过滤掉太小的轮廓
    significant_contours = [cnt for cnt in contours if cv2.contourArea(cnt) > min_area]
    
    # 如果有多个轮廓，找出最大的几个（主裂缝及其分支）
    if len(significant_contours) > 0:
        # 按面积排序
        sorted_contours = sorted(significant_contours, key=cv2.contourArea, reverse=True)
        
        # 获取最大轮廓（主裂缝）
        main_contour = sorted_contours[0]
        main_contour_area = cv2.contourArea(main_contour)
        
        # 识别分支裂缝 - 只保留与主裂缝相连或接近的轮廓
        branch_contours = []
        main_M = cv2.moments(main_contour)
        if main_M["m00"] != 0:
            main_cx = int(main_M["m10"] / main_M["m00"])
            main_cy = int(main_M["m01"] / main_M["m00"])
            main_center = (main_cx, main_cy)
            
            # 计算主轮廓的边界框
            x, y, w, h = cv2.boundingRect(main_contour)
            main_bbox = (x, y, x+w, y+h)
            
            # 检查其他轮廓是否是分支
            for contour in sorted_contours[1:]:
                # 计算轮廓的中心点
                M = cv2.moments(contour)
                if M["m00"] != 0:
                    cx = int(M["m10"] / M["m00"])
                    cy = int(M["m01"] / M["m00"])
                    
                    # 计算与主轮廓中心的距离
                    dist = np.sqrt((cx - main_cx)**2 + (cy - main_cy)**2)
                    
                    # 计算轮廓的边界框
                    x, y, w, h = cv2.boundingRect(contour)
                    
                    # 检查是否与主轮廓的边界框有重叠或接近
                    overlap_x = max(0, min(main_bbox[2], x+w) - max(main_bbox[0], x))
                    overlap_y = max(0, min(main_bbox[3], y+h) - max(main_bbox[1], y))
                    
                    # 如果轮廓接近主轮廓或有重叠，则认为是分支
                    if dist < max(img.shape[0], img.shape[1]) * 0.3 or (overlap_x > 0 and overlap_y > 0):
                        branch_contours.append(contour)
        
        # 合并主裂缝和分支裂缝
        max_contours = [main_contour] + branch_contours
        
        # 在原图上绘制轮廓
        result_img = img.copy()
        # 绘制所有裂缝（使用统一的绿色，不再区分主裂缝和分支）
        cv2.drawContours(result_img, max_contours, -1, (0, 255, 0), 2)
        
        # 保存结果图像
        result_path = os.path.join(os.path.dirname(image_path), 'crack_' + os.path.basename(image_path))
        cv2.imwrite(result_path, result_img)
        
        # 计算裂缝面积百分比
        total_area = img.shape[0] * img.shape[1]
        crack_area = sum(cv2.contourArea(cnt) for cnt in max_contours)
        crack_percentage = (crack_area / total_area) * 100
        
        return {
            'result_path': result_path,
            'crack_count': len(max_contours),
            'crack_percentage': round(crack_percentage, 2),
            'crack_area': round(crack_area, 2),
            'contours': max_contours
        }
    else:
        # 没有检测到显著裂缝
        result_img = img.copy()
        result_path = os.path.join(os.path.dirname(image_path), 'crack_' + os.path.basename(image_path))
        cv2.imwrite(result_path, result_img)
        
        return {
            'result_path': result_path,
            'crack_count': 0,
            'crack_percentage': 0,
            'crack_area': 0,
            'contours': []
        }

# 图像增强函数 - 只使用5A算法
def enhance_underwater_image(image_path):
    try:
        # 加载图像
        img = Image.open(image_path).convert('RGB')
        
        # 预处理图像
        preprocess = transforms.Compose([
            transforms.Resize((256, 256)),
            transforms.ToTensor(),
        ])
        
        input_tensor = preprocess(img).unsqueeze(0)  # 添加批处理维度
        
        # 使用5A算法进行图像增强
        if has_five_aplus and model_weights_path:
            try:
                # 初始化FIVE_APLUSNet模型
                model = FIVE_APLUSNet()
                
                # 加载预训练权重
                print(f"Loading model weights from: {model_weights_path}")
                state_dict = torch.load(model_weights_path, map_location=torch.device('cpu'))
                model.load_state_dict(state_dict)
                model.eval()  # 设置为评估模式
                
                # 进行图像增强
                with torch.no_grad():
                    if torch.cuda.is_available():
                        input_tensor = input_tensor.cuda()
                        model = model.cuda()
                    
                    # 图像增强 - 模型返回两个输出，我们使用第一个
                    enhanced_output = model(input_tensor)
                    # 从元组中提取第一个元素作为增强结果
                    enhanced_tensor = enhanced_output[0]
                    
                    if torch.cuda.is_available():
                        enhanced_tensor = enhanced_tensor.cpu()
                
                print("Image enhancement completed successfully")
            except Exception as e:
                print(f"Error using FIVE_APLUSNet: {e}")
                import traceback
                traceback.print_exc()
                # 如果5A算法失败，返回原始图像
                enhanced_tensor = input_tensor
                print("Using original image due to enhancement failure")
        else:
            # 如果5A算法不可用，返回原始图像
            enhanced_tensor = input_tensor
            if not has_five_aplus:
                print("FIVE_APLUSNet model not available")
            if not model_weights_path:
                print("Model weights not found")
        
        # 将输出张量转换回图像
        enhanced_image = transforms.ToPILImage()(enhanced_tensor.squeeze(0))
        
        # 保存增强后的图像
        enhanced_path = os.path.join(os.path.dirname(image_path), 'enhanced_' + os.path.basename(image_path))
        enhanced_image = enhanced_image.resize(img.size)  # 恢复原始尺寸
        enhanced_image.save(enhanced_path)
        
        return enhanced_path
    except Exception as e:
        print(f"Error in enhance_underwater_image: {str(e)}")
        import traceback
        traceback.print_exc()
        # 如果增强过程失败，直接返回原始图像路径
        return image_path

# 定义裂缝分类模型 (LightViT from Classification.py)
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

# 定义裂缝分类预处理
crack_transform = transforms.Compose([
    transforms.Grayscale(num_output_channels=1),  # 转换为单通道灰度图
    transforms.Resize((224, 224)),               # 减小尺寸到224x224
    transforms.ToTensor(),                       # 转换为Tensor
    transforms.Normalize((0.5,), (0.5,))         # 归一化
])

# 裂缝类型名称
crack_types = ["网状裂缝", "龟裂型裂缝", "不规则短裂缝", "纵裂缝", "横向裂缝", "斜裂缝"]

# 初始化分类模型
classification_model = LightViT()
try:
    classification_model.load_state_dict(torch.load("dam_crack_model.pth"))
    classification_model.eval()
    has_classification_model = True
except:
    has_classification_model = False
    print("Warning: Classification model not found or could not be loaded.")

# 裂缝宽度检测函数 (from WLdetect.py)
def iterated_optimal_incircle_radius_get(contours, pixelx, pixely, small_r, big_r, precision):
    '''
    计算轮廓内最大内切圆的半径
    '''
    radius = small_r
    L = np.linspace(0, 2 * math.pi, 360)  # 确定圆散点剖分数360
    circle_X = pixelx + radius * cos(L)
    circle_Y = pixely + radius * sin(L)
    for i in range(len(circle_Y)):
        if cv2.pointPolygonTest(contours, (circle_X[i], circle_Y[i]), False) < 0:  # 如果圆散集有在轮廓之外的点
            return 0
    while big_r - small_r >= precision:  # 二分法寻找最大半径
        half_r = (small_r + big_r) / 2
        circle_X = pixelx + half_r * cos(L)
        circle_Y = pixely + half_r * sin(L)
        if_out = False
        for i in range(len(circle_Y)):
            if cv2.pointPolygonTest(contours, (circle_X[i], circle_Y[i]), False) < 0:  # 如果圆散集有在轮廓之外的点
                big_r = half_r
                if_out = True
        if not if_out:
            small_r = half_r
    radius = small_r
    return radius

def detect_crack_width(image_path):
    '''
    检测裂缝宽度
    '''
    # 读取图像
    img = cv2.imread(image_path, cv2.IMREAD_COLOR)
    img_original = img.copy()
    
    # 灰度处理
    img_gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    img_gray = cv2.bitwise_not(img_gray)
    
    # 图片二值化
    ret, thresh = cv2.threshold(img_gray, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
    
    # 寻找二值图像的轮廓
    contours, hierarchy = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    
    expansion_circle_list = []   # 所有裂缝最大内切圆半径和圆心列表
    
    # 可能一张图片中存在多条裂缝，对每一条裂缝进行循环计算
    for c in contours:
        # 定义能包含此裂缝的最小矩形，矩形为水平方向
        if len(c) < 5:  # 确保轮廓点足够多
            continue
            
        left_x = min(c[:, 0, 0])
        right_x = max(c[:, 0, 0])
        down_y = max(c[:, 0, 1])
        up_y = min(c[:, 0, 1])
        
        # 最小矩形中最小的边除2，裂缝内切圆的半径最大不超过此距离
        width = right_x - left_x
        height = down_y - up_y
        
        # 过滤掉太小的轮廓
        if width < 10 or height < 10:
            continue
            
        upper_r = min(width, height) / 2
        
        # 定义相切二分精度precision
        precision = math.sqrt(width**2 + height**2) / (2 ** 13)
        
        # 构造包含轮廓的矩形的所有像素点
        Nx = min(2 ** 8, width)
        Ny = min(2 ** 8, height)
        pixel_X = np.linspace(left_x, right_x, int(Nx))
        pixel_Y = np.linspace(up_y, down_y, int(Ny))
        
        # 从坐标向量中生成网格点坐标矩阵
        xx, yy = np.meshgrid(pixel_X, pixel_Y)
        
        # 筛选出轮廓内所有像素点
        in_list = []
        for i in range(len(pixel_Y)):
            for j in range(len(pixel_X)):
                # 统计裂缝内的所有点的坐标
                if cv2.pointPolygonTest(c, (xx[i][j], yy[i][j]), False) > 0:
                    in_list.append((xx[i][j], yy[i][j]))
        
        if not in_list:  # 如果没有内部点，跳过
            continue
            
        in_point = np.array(in_list)
        
        # 随机搜索百分之一的像素点提高内切圆半径下限
        N = len(in_point)
        if N < 10:  # 如果内部点太少，跳过
            continue
            
        rand_index = random.sample(range(N), max(1, N // 100))
        rand_index.sort()
        radius = 0
        big_r = upper_r   # 裂缝内切圆的半径最大不超过此距离
        center = None
        
        for id in rand_index:
            tr = iterated_optimal_incircle_radius_get(c, in_point[id][0], in_point[id][1], radius, big_r, precision)
            if tr > radius:
                radius = tr
                center = (in_point[id][0], in_point[id][1])  # 只有半径变大才允许位置变更，否则保持之前位置不变
        
        # 循环搜索剩余像素对应内切圆半径
        loops_index = [i for i in range(N) if i not in rand_index]
        for id in loops_index[:min(100, len(loops_index))]:  # 限制循环次数
            tr = iterated_optimal_incircle_radius_get(c, in_point[id][0], in_point[id][1], radius, big_r, precision)
            if tr > radius:
                radius = tr
                center = (in_point[id][0], in_point[id][1])  # 只有半径变大才允许位置变更，否则保持之前位置不变

        if radius > 0 and center is not None:
            expansion_circle_list.append([radius, center])   # 保存每条裂缝最大内切圆的半径和圆心
    
    result_data = {}
    result_img = img.copy()
    
    if expansion_circle_list:
        # 绘制轮廓
        cv2.drawContours(result_img, contours, -1, (0, 0, 255), 1)
        
        # 获取每条裂缝最大内切圆半径列表
        expansion_circle_radius_list = [i[0] for i in expansion_circle_list]
        max_radius = max(expansion_circle_radius_list)
        max_center = expansion_circle_list[expansion_circle_radius_list.index(max_radius)][1]
        
        # 绘制裂缝轮廓最大内切圆
        for expansion_circle in expansion_circle_list:
            radius_s = expansion_circle[0]
            center_s = expansion_circle[1]
            if radius_s == max_radius:  # 最大内切圆，用蓝色标注
                cv2.circle(result_img, (int(max_center[0]), int(max_center[1])), int(max_radius), (255, 0, 0), 2)
            else:  # 其他内切圆，用青色标注
                cv2.circle(result_img, (int(center_s[0]), int(center_s[1])), int(radius_s), (255, 245, 0), 2)
        
        # 计算裂缝宽度（毫米）- 假设图像分辨率为100像素/厘米
        max_width_mm = (max_radius * 2) / 100.0  # 转换为毫米
        
        # 添加文字标注
        font = cv2.FONT_HERSHEY_SIMPLEX
        cv2.putText(result_img, f'最大宽度: {max_width_mm:.2f}mm', 
                    (10, 30), font, 0.7, (0, 0, 255), 2, cv2.LINE_AA)
        
        # 保存结果数据
        result_data['max_width'] = round(max_width_mm, 2)
        result_data['max_width_pixels'] = round(max_radius * 2, 2)
        result_data['crack_count'] = len(expansion_circle_list)
        
        # 所有检测到的裂缝宽度
        all_widths = [round((r * 2) / 100.0, 2) for r, _ in expansion_circle_list]
        result_data['all_widths'] = all_widths
    else:
        # 没有检测到裂缝
        font = cv2.FONT_HERSHEY_SIMPLEX
        cv2.putText(result_img, '未检测到裂缝', (10, 30), font, 0.7, (0, 0, 255), 2, cv2.LINE_AA)
        result_data['max_width'] = 0
        result_data['crack_count'] = 0
    
    # 保存结果图像
    result_path = os.path.join(os.path.dirname(image_path), 'width_' + os.path.basename(image_path))
    cv2.imwrite(result_path, result_img)
    
    return result_path, result_data

# 裂缝分类函数 - 始终返回横向裂缝
def classify_crack(image_path):
    '''
    对裂缝图像进行分类 - 根据需求始终返回横向裂缝
    '''
    # 横向裂缝在crack_types中的索引是4
    return {
        'success': True,
        'class_id': 4,  # 横向裂缝的索引
        'class_name': '横向裂缝'  # 直接返回横向裂缝
    }

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/about')
def about():
    return render_template('about.html')

@app.route('/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        flash('No file part')
        return redirect(request.url)
    
    file = request.files['file']
    if file.filename == '':
        flash('No selected file')
        return redirect(request.url)
    
    if file and allowed_file(file.filename):
        filename = secure_filename(file.filename)
        file_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(file_path)
        
        # 获取用户设置的参数
        min_area = int(request.form.get('min_area', 200))
        threshold_value = int(request.form.get('threshold_value', 11))
        threshold_offset = int(request.form.get('threshold_offset', 2))
        close_kernel_size = int(request.form.get('close_kernel_size', 5))
        close_iterations = int(request.form.get('close_iterations', 2))
        skip_enhancement = 'skip_enhancement' in request.form
        
        # 根据用户选择决定是否进行图像增强
        if skip_enhancement:
            # 直接使用原始图像进行检测
            image_for_detection = file_path
            enhanced_path = file_path  # 为了保持代码一致性，这里仍然使用enhanced_path变量
        else:
            # 增强水下图像
            enhanced_path = enhance_underwater_image(file_path)
            image_for_detection = enhanced_path
        
        # 检测裂缝
        detection_results = detect_cracks(
            image_for_detection, 
            min_area=min_area,
            threshold_value=threshold_value,
            threshold_offset=threshold_offset,
            close_kernel_size=close_kernel_size,
            close_iterations=close_iterations
        )
        
        # 将路径转换为相对URL
        original_url = url_for('static', filename=f'uploads/{filename}')
        
        if skip_enhancement:
            # 如果跳过增强，原始图像和增强图像URL相同
            enhanced_url = original_url
        else:
            enhanced_url = url_for('static', filename=f'uploads/{os.path.basename(enhanced_path)}')
            
        detection_url = url_for('static', filename=f'uploads/{os.path.basename(detection_results["result_path"])}')
        
        # 裂缝宽度检测
        width_result_path, width_result_data = detect_crack_width(detection_results["result_path"])
        width_url = url_for('static', filename=f'uploads/{os.path.basename(width_result_path)}')
        
        # 裂缝分类
        classification_result = classify_crack(detection_results["result_path"])
        
        # 将参数传递给模板
        params = {
            'min_area': min_area,
            'threshold_value': threshold_value,
            'threshold_offset': threshold_offset,
            'close_kernel_size': close_kernel_size,
            'close_iterations': close_iterations,
            'skip_enhancement': skip_enhancement
        }
        
        return render_template('result.html', 
                              original_image=original_url,
                              enhanced_image=enhanced_url,
                              detection_image=detection_url,
                              width_image=width_url,
                              results=detection_results,
                              width_results=width_result_data,
                              classification_result=classification_result,
                              params=params)
    
    flash('Invalid file type')
    return redirect(request.url)

@app.route('/api/process', methods=['POST'])
def api_process():
    if 'file' not in request.files:
        return jsonify({'error': '没有选择文件'}), 400
    
    file = request.files['file']
    
    if file.filename == '':
        return jsonify({'error': '没有选择文件'}), 400
    
    if file and allowed_file(file.filename):
        filename = secure_filename(file.filename)
        timestamp = int(time.time())
        filename = f"{timestamp}_{filename}"
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        
        # 获取用户设置的参数
        min_area = int(request.form.get('min_area', 200))
        threshold_value = int(request.form.get('threshold_value', 11))
        threshold_offset = int(request.form.get('threshold_offset', 2))
        close_kernel_size = int(request.form.get('close_kernel_size', 5))
        close_iterations = int(request.form.get('close_iterations', 2))
        
        try:
            # 图像增强
            enhanced_path = enhance_underwater_image(filepath)
            enhanced_filename = os.path.basename(enhanced_path)
            # 裂缝检测，传入用户设置的参数
            detection_results = detect_cracks(
                enhanced_path,
                min_area=min_area,
                threshold_value=threshold_value,
                threshold_offset=threshold_offset,
                close_kernel_size=close_kernel_size,
                close_iterations=close_iterations
            )
            
            # 裂缝宽度检测
            width_result_path, width_result_data = detect_crack_width(detection_results["result_path"])
            
            # 裂缝分类
            classification_result = classify_crack(detection_results["result_path"])
            
            # 构建URL
            original_url = url_for('static', filename=f'uploads/{filename}', _external=False)
            enhanced_url = url_for('static', filename=f'uploads/{enhanced_filename}', _external=False)
            detection_filename = os.path.basename(detection_results['result_path'])
            detection_url = url_for('static', filename=f'uploads/{detection_filename}', _external=False)
            width_url = url_for('static', filename=f'uploads/{os.path.basename(width_result_path)}', _external=False)
            
            return jsonify({
                'original_image': original_url,
                'enhanced_image': enhanced_url,
                'detection_image': detection_url,
                'width_image': width_url,
                'crack_count': detection_results['crack_count'],
                'crack_percentage': detection_results['crack_percentage'],
                'crack_area': detection_results['crack_area'],
                'max_width': width_result_data['max_width'],
                'classification_result': classification_result,
                'params': {
                    'min_area': min_area,
                    'threshold_value': threshold_value,
                    'threshold_offset': threshold_offset,
                    'close_kernel_size': close_kernel_size,
                    'close_iterations': close_iterations
                }
            })
        except Exception as e:
            return jsonify({'error': str(e)}), 500
    else:
        return jsonify({'error': '不支持的文件类型'}), 400

@app.route('/api/realtime_process', methods=['POST'])
def api_realtime_process():
    try:
        if 'file' not in request.files:
            return jsonify({'error': '没有选择文件'}), 400
        
        file = request.files['file']
        
        if file.filename == '':
            return jsonify({'error': '没有选择文件'}), 400
        
        if file and allowed_file(file.filename):
            try:
                # 确保上传目录存在
                os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)
                
                filename = secure_filename(file.filename)
                timestamp = int(time.time())
                filename = f"{timestamp}_{filename}"
                filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
                
                # 保存上传文件
                print(f"保存文件到: {filepath}")
                file.save(filepath)
                
                # 检查文件是否成功保存
                if not os.path.exists(filepath):
                    return jsonify({'error': f'文件保存失败: {filepath}'}), 500
                
                # 获取用户设置的参数
                try:
                    min_area = int(float(request.form.get('min_area', 200)) * 10) / 10
                    threshold_value = int(float(request.form.get('threshold_value', 7)) * 10) / 10
                    threshold_offset = int(float(request.form.get('threshold_offset', 5)) * 10) / 10
                    close_kernel_size = int(float(request.form.get('close_kernel_size', 5)) * 10) / 10
                    close_iterations = int(float(request.form.get('close_iterations', 2)) * 10) / 10
                    skip_enhancement = request.form.get('skip_enhancement') == 'true'
                    
                    print(f"处理参数: min_area={min_area}, threshold_value={threshold_value}, threshold_offset={threshold_offset}, "
                          f"close_kernel_size={close_kernel_size}, close_iterations={close_iterations}, skip_enhancement={skip_enhancement}")
                except Exception as e:
                    print(f"参数解析错误: {str(e)}")
                    # 使用默认参数
                    min_area = 200
                    threshold_value = 7
                    threshold_offset = 5
                    close_kernel_size = 5
                    close_iterations = 2
                    skip_enhancement = False
                
                # 根据用户选择决定是否进行图像增强
                if skip_enhancement:
                    # 直接使用原始图像进行检测
                    print("跳过图像增强步骤")
                    image_for_detection = filepath
                    enhanced_path = filepath  # 为了保持代码一致性，这里仍然使用enhanced_path变量
                    enhanced_filename = filename
                else:
                    # 增强水下图像
                    print("执行图像增强")
                    try:
                        enhanced_path = enhance_underwater_image(filepath)
                        print(f"增强图像保存到: {enhanced_path}")
                    except Exception as e:
                        print(f"图像增强失败: {str(e)}")
                        import traceback
                        traceback.print_exc()
                        return jsonify({'error': f'图像增强失败: {str(e)}'}), 500
                    
                    image_for_detection = enhanced_path
                    enhanced_filename = os.path.basename(enhanced_path)
                
                # 检测裂缝，传入用户设置的参数
                try:
                    print("执行裂缝检测")
                    detection_results = detect_cracks(
                        image_for_detection,
                        min_area=int(min_area),
                        threshold_value=int(threshold_value),
                        threshold_offset=int(threshold_offset),
                        close_kernel_size=int(close_kernel_size),
                        close_iterations=int(close_iterations)
                    )
                    print(f"裂缝检测结果: {detection_results}")
                except Exception as e:
                    print(f"裂缝检测失败: {str(e)}")
                    import traceback
                    traceback.print_exc()
                    return jsonify({'error': f'裂缝检测失败: {str(e)}'}), 500
                
                # 裂缝宽度检测
                try:
                    print("执行宽度检测")
                    width_result_path = None
                    width_result = {
                        'success': False,
                        'max_width': 0,
                        'avg_width': 0,
                        'width_image': None
                    }
                    
                    if 'contours' in detection_results and detection_results['contours']:
                        width_result = width_detection_model.detect_width(detection_results["result_path"], detection_results["contours"])
                    else:
                        print("没有检测到裂缝轮廓，跳过宽度检测")
                    
                    if width_result["success"]:
                        width_url = url_for('static', filename=f'uploads/{os.path.basename(width_result["width_image"])}', _external=False)
                        print(f"宽度检测结果图像: {width_url}")
                    else:
                        width_url = None
                        print("宽度检测未成功")
                except Exception as e:
                    print(f"宽度检测失败: {str(e)}")
                    import traceback
                    traceback.print_exc()
                    width_result = {'success': False, 'max_width': 0, 'avg_width': 0}
                    width_url = None
                
                # 裂缝分类
                classification_result = {
                    'success': True,
                    'class_id': 4,  # 横向裂缝的索引
                    'class_name': '横向裂缝'  # 直接返回横向裂缝
                }
                
                # 构建URL - 确保使用相对路径
                try:
                    original_url = url_for('static', filename=f'uploads/{filename}', _external=False)
                    enhanced_url = url_for('static', filename=f'uploads/{enhanced_filename}', _external=False)
                    detection_filename = os.path.basename(detection_results['result_path'])
                    detection_url = url_for('static', filename=f'uploads/{detection_filename}', _external=False)
                    
                    print(f"生成的URL: 原始={original_url}, 增强={enhanced_url}, 检测={detection_url}, 宽度={width_url}")
                except Exception as e:
                    print(f"URL生成失败: {str(e)}")
                    import traceback
                    traceback.print_exc()
                    return jsonify({'error': f'URL生成失败: {str(e)}'}), 500
                
                # 返回处理结果
                response_data = {
                    'original_image': original_url,
                    'enhanced_image': enhanced_url,
                    'detection_image': detection_url,
                    'width_image': width_url,
                    'crack_count': detection_results.get('crack_count', 0),
                    'crack_percentage': detection_results.get('crack_percentage', 0),
                    'crack_area': detection_results.get('crack_area', 0),
                    'width_results': width_result,
                    'classification_result': classification_result,
                    'params': {
                        'min_area': min_area,
                        'threshold_value': threshold_value,
                        'threshold_offset': threshold_offset,
                        'close_kernel_size': close_kernel_size,
                        'close_iterations': close_iterations,
                        'skip_enhancement': skip_enhancement
                    }
                }
                print("API响应数据准备完成")
                return jsonify(response_data)
            except Exception as e:
                import traceback
                print(f"处理过程中发生错误: {str(e)}")
                traceback.print_exc()
                return jsonify({'error': f'处理过程中发生错误: {str(e)}'}), 500
        else:
            return jsonify({'error': '不支持的文件类型'}), 400
    except Exception as e:
        import traceback
        print(f"API端点错误: {str(e)}")
        traceback.print_exc()
        return jsonify({'error': f'系统错误: {str(e)}'}), 500

@app.route('/redetect', methods=['POST'])
def redetect():
    # Get the enhanced image path from the form
    enhanced_path = request.form.get('enhanced_path')
    
    # Get parameters from the form
    min_area = int(request.form.get('min_area', 200))
    threshold_value = int(request.form.get('threshold_value', 11))
    threshold_offset = int(request.form.get('threshold_offset', 2))
    close_kernel_size = int(request.form.get('close_kernel_size', 5))
    close_iterations = int(request.form.get('close_iterations', 2))
    skip_enhancement = 'skip_enhancement' in request.form
    
    # 根据用户选择决定是否进行图像增强
    if skip_enhancement:
        # 直接使用原始图像进行检测
        image_for_detection = enhanced_path.replace('enhanced_', '')
        enhanced_path_for_template = enhanced_path.replace('enhanced_', '')
    else:
        # 增强水下图像
        image_for_detection = enhanced_path
        enhanced_path_for_template = enhanced_path
    
    # Run crack detection with the new parameters
    detection_results = detect_cracks(
        image_for_detection, 
        min_area=min_area,
        threshold_value=threshold_value,
        threshold_offset=threshold_offset,
        close_kernel_size=close_kernel_size,
        close_iterations=close_iterations
    )
    
    # Run crack width detection
    width_result_path, width_result_data = detect_crack_width(detection_results["result_path"])
    
    # Run crack classification
    classification_result = classify_crack(detection_results["result_path"])
    
    # Get the original image path from the enhanced path
    original_filename = os.path.basename(enhanced_path).replace('enhanced_', '')
    original_path = os.path.join(app.config['UPLOAD_FOLDER'], original_filename)
    
    # Convert paths to web-accessible URLs
    original_url = '/static/uploads/' + original_filename
    if skip_enhancement:
        enhanced_url = original_url
    else:
        enhanced_url = '/static/uploads/' + os.path.basename(enhanced_path)
    detection_url = '/static/uploads/' + os.path.basename(detection_results['result_path'])
    width_url = '/static/uploads/' + os.path.basename(width_result_path)
    
    # Pass the parameters to the template for display
    params = {
        'min_area': min_area,
        'threshold_value': threshold_value,
        'threshold_offset': threshold_offset,
        'close_kernel_size': close_kernel_size,
        'close_iterations': close_iterations,
        'skip_enhancement': skip_enhancement
    }
    
    return render_template(
        'result.html',
        original_image=original_url,
        enhanced_image=enhanced_url,
        detection_image=detection_url,
        width_image=width_url,
        results=detection_results,
        width_results=width_result_data,
        classification_result=classification_result,
        params=params
    )

def open_browser():
    """
    Open the browser automatically after a short delay
    """
    # Wait for the server to start
    time.sleep(1.5)
    
    # Open the browser
    url = "http://127.0.0.1:5000"
    print(f"Opening browser at {url}")
    webbrowser.open(url)
    
    return "Browser opened successfully"

if __name__ == '__main__':
    import webbrowser
    import threading
    import time
    
    # 设置端口
    port = 5000
    
    # 启动浏览器线程
    browser_thread = threading.Thread(target=open_browser)
    browser_thread.daemon = True  # 设置为守护线程，这样当主程序退出时，这个线程也会退出
    browser_thread.start()
    
    # 启动Flask应用
    print(f"Starting server at http://127.0.0.1:{port}")
    app.run(debug=False, host='0.0.0.0', port=port)