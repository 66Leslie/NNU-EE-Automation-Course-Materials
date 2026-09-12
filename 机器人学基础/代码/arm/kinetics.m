clear all;
close;
clc;
startup_rvc;

% 创建 PUMA560 模型
puma_robot = loadrobot("puma560");
show(puma_robot); 

% 初始化 Link 对象数组
L = repmat(Link(), 1, 3);

% SCARA 机器人定义（3自由度）
L(1) = Revolute('d', 0.3, 'a', 0.2, 'alpha', 0);
L(2) = Revolute('d', 0,   'a', 0.3, 'alpha', pi);
L(3) = Prismatic('theta', 0, 'a', 0, 'alpha', 0, 'qlim', [0, 0.5]);

scara_robot = SerialLink(L, 'name', 'SCARA');
scara_robot.plot([0 0 0]); % 初始姿态

% 定义关节角度
q_multi = [0, 0, 0;
           pi/4, pi/2, 0.2];  % 示例：第三关节移动 0.2 米

% 计算正运动学
T_multi = scara_robot.fkine(q_multi);

% 提取位置
position = transl(T_multi); % 使用工具箱函数

% 可视化 SCARA
scara_robot.plot(q_multi, 'workspace', [-1 1 -1 1 -0.5 1], 'view', 'top');
title(sprintf('SCARA 末端位置: [%.2f, %.2f, %.2f]', position(1), position(2), position(3)));