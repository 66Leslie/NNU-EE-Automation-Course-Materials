clc; clear; close all;

%% 1. 数据输入
F = [1.28, 2, 3, 4, 5, 6, 7, 8, 9];   % 频率 (Khz)
V = [220, 280, 344, 480, 680, 420, 250, 160, 140]; % V_pp

%% 2. 样条插值 (Spline Interpolation) calculation
% 生成更密集的 X 轴坐标，用于画出平滑曲线
% linspace(起始, 结束, 点数) -> 这里生成 500 个点让曲线看起来平滑
F_smooth = linspace(min(F), max(F), 500);

% 使用 'spline' 方法进行计算
% spline(已知X, 已知Y, 待求X)
V_smooth = interp1(F, V, F_smooth, 'spline');

%% 3. 绘图展示
figure('Color', 'w'); % 设置背景为白色
hold on; grid on; box on;

% 画原始散点 (蓝色圆圈)
plot(F, V, 'b.', 'MarkerSize', 20, 'DisplayName', '数据点');

% 画拟合曲线 (红色实线)
plot(F_smooth, V_smooth, 'r-', 'LineWidth', 2, 'DisplayName', '幅频特性曲线');

% 添加图表标签
xlabel('F (KHz)', 'FontSize', 12, 'FontWeight', 'bold');
ylabel('V_{pp}(mV)', 'FontSize', 12, 'FontWeight', 'bold');
title('样条插值拟合效果', 'FontSize', 14);
legend('show', 'Location', 'NorthEast');

% 标注最高点 (可选)
[maxV, idx] = max(V);

hold off;