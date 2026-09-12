% ===================================================================
% 二自由度机械臂仿真 - 参数初始化脚本 (最终修正版)
% ===================================================================
clear; clc; close all;

%% 物理参数 (Physical Parameters)
g = 9.8; m1 = 1.0; m2 = 1.0; l1 = 1.0; l2 = 1.0;

%% 控制器参数 (Controller Parameters)
% 保持较低增益以确保启动稳定
Kp = diag([100, 100]);
Kd = diag([40, 40]);

%% 轨迹规划参数 (Trajectory Parameters)
xc = 0.5; yc = 0.5; r = 0.2; omega = pi/2;

%% 仿真参数 (Simulation Parameters)
T_sim = 8;
initial_dtheta = [0; 0]; % 机器人本体初始角速度为0

% -------------------------------------------------------------------
%  核心修正：计算轨迹起点的位置、速度，并设为初始条件
% -------------------------------------------------------------------
% 1. 计算轨迹在 t=0 时的【位置】
xd0 = xc + r*cos(0);
yd0 = yc + r*sin(0);
val = (xd0^2 + yd0^2 - l1^2 - l2^2) / (2*l1*l2);
theta2_0 = -acos(val);
k1 = l1 + l2*cos(theta2_0);
k2 = l2*sin(theta2_0);
theta1_0 = atan2(yd0, xd0) - atan2(k2, k1);
initial_theta = [theta1_0; theta2_0]; % 机器人本体初始角度

% 2. 计算轨迹在 t=0 时的【速度】
% 首先计算笛卡尔空间的速度
dxd0 = -r*omega*sin(0);
dyd0 = r*omega*cos(0);
cartesian_vel0 = [dxd0; dyd0];

% 然后计算雅可比矩阵在初始位置的值 J(theta_0)
s1 = sin(initial_theta(1));
s2 = sin(initial_theta(2));
c1 = cos(initial_theta(1));
c2 = cos(initial_theta(2));
s12 = sin(initial_theta(1) + initial_theta(2));
c12 = cos(initial_theta(1) + initial_theta(2));
J = [-l1*s1 - l2*s12, -l2*s12; 
      l1*c1 + l2*c12,  l2*c12];

% 最后通过 J_inv * dX = dTheta 计算初始期望角速度
initial_dtheta_d = inv(J) * cartesian_vel0;

% 3. 计算轨迹在 t=0 时的【加速度】
% 为简化，我们假设初始期望角加速度为0
initial_d2theta_d = [0; 0];

disp('最终版参数已加载: 位置、速度初始条件均已对齐!');