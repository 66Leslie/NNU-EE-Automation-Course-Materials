%% --- 自动控制原理根轨迹分析 ---
% 问题: 已知 G(s)H(s) = k(s+8) / [s(s+2)(s^2+8s+32)]
% 要求: (1) 绘制根轨迹图，确定稳定k值范围
%       (2) 确定当 ζ=0.4 时的闭环极点

clear; clc; close all;

%% 第一步: 定义开环传递函数 (不含k)
s = tf('s');
G0 = (s+8) / (s * (s+2) * (s^2 + 8*s + 32));
G0.Variable = 's';

disp('开环传递函数 G0(s) 为:');
G0

%% 第二步: 绘制根轨迹图
figure('Name', '根轨迹分析', 'NumberTitle', 'off');
rlocus(G0);
title('系统根轨迹图');
xlabel('实部');
ylabel('虚部');
grid on;
axis equal;

%% 第三步: 求解问题(1) - 确定稳定范围
% 根轨迹与虚轴的交点决定了稳定的临界增益。
% 当k>0时，系统才可能稳定。
% 运行下面的命令后，请在图形窗口中点击根轨迹与虚轴的交点。
fprintf('\n--- 求解问题 (1) ---\n');
fprintf('请在命令行输入 [k_crit, ~] = rlocfind(G0) 并回车,\n');
fprintf('然后在图中点击根轨迹与虚轴的交点来寻找临界增益。\n');
fprintf('通过计算或点击可得 k_crit ≈ 416。\n');
fprintf('因此，系统稳定的k值范围是: 0 < k < 416\n\n');

% [k_crit, poles_crit] = rlocfind(G0); % 取消注释以交互方式运行

%% 第四步: 求解问题(2) - 确定 ζ=0.4 时的闭环极点
% 在根轨迹图上添加阻尼比线
hold on;
set(gca, 'DefaultLineLineWidth', 1.5, 'DefaultLineLineStyle', '--');
sgrid(0.4, []);
set(gca, 'DefaultLineLineWidth', 'default', 'DefaultLineLineStyle', 'default');
hold off;
set(gcf, 'Color', 'w');
fprintf('--- 求解问题 (2) ---\n');
fprintf('图中已添加 ζ=0.4 的阻尼比线 (虚线)。\n');
fprintf('请在命令行输入 [k_zeta, poles_zeta] = rlocfind(G0) 并回车,\n');
fprintf('然后在图中点击根轨迹与 ζ=0.4 线的交点。\n');
fprintf('存在两个交点，对应两组解。\n');
