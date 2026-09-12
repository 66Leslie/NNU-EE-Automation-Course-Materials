function [A] = filter_b(f, Nh)
% 该函数用于设计一系列巴特沃斯带通滤波器
% f: 基波频率
% Nh: 需要设计的谐波滤波器数量
% A: 输出的元胞数组，每行包含一个滤波器的分子(num)和分母(den)

    Rp = 1;     % 通带最大衰减 (dB)
    Rs = 20;    % 阻带最小衰减 (dB)
    A = cell(Nh, 2); % 初始化元胞数组

    for k = 1:Nh
        % 第 2k 次谐波的中心频率为 f * 2k
        center_freq = f * 2 * k; 
        
        % 定义通带频率范围，例如中心频率的 ±10%
        wp = [center_freq - f * 0.1, center_freq + f * 0.1] * 2 * pi;
        
        % 定义阻带频率范围，例如中心频率的 ±40%
        ws = [center_freq - f * 0.4, center_freq + f * 0.4] * 2 * pi;
        
        % 计算滤波器阶数 N 和截止频率 Wn
        [N, Wn] = buttord(wp, ws, Rp, Rs, 's');
        
        % 设计巴特沃斯滤波器
        [num, den] = butter(N, Wn, 's');
        
        % 存储滤波器的传递函数系数
        A{k, 1} = num;
        A{k, 2} = den;
    end
end