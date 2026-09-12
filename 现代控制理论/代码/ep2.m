% --- 1. 定义系统---
A = [0 1; -6 -5];
B = [0; 1];
C = [1 0]; % y = x1
D = 0;
sys = ss(A,B,C,D);
t = linspace(0,5,1000)';
x0 = [1; 0]; % 初始条件

% --- 2. 定义输入信号 ---
u_zero = zeros(size(t));  % 零输入
u_step = ones(size(t));   % 单位阶跃输入

% (1) 零输入响应 (ZIR): u(t)=0, x(0) = x0
[y_zi, t_zi, x_zi] = initial(sys, x0, t);

% (2) 零状态响应 (ZSR)
[y_zs, t_zs, x_zs] = step(sys, t);

% 子图 1: 零输入响应 - 状态轨迹 (x1, x2)
subplot(1, 2, 1);
plot(t_zi, x_zi(:,1), 'b-', 'LineWidth', 1.5); hold on;
plot(t_zi, x_zi(:,2), 'r--', 'LineWidth', 1.5);
grid on;
title(sprintf('(1) 零输入响应 - 状态轨迹 (x0=[%g; %g])', x0(1), x0(2)));
xlabel('t / s'); ylabel('状态 x(t)');
legend('x_1(t)', 'x_2(t)', 'Location', 'best');

% 子图 2: 零状态响应 - 状态轨迹 (x1, x2)
subplot(1, 2, 2);
plot(t_zs, x_zs(:,1), 'b-', 'LineWidth', 1.5); hold on;
plot(t_zs, x_zs(:,2), 'r--', 'LineWidth', 1.5);
grid on;
title('(2) 零状态响应 (单位阶跃) - 状态轨迹');
xlabel('t / s'); ylabel('状态 x(t)');
legend('x_1(t)', 'x_2(t)', 'Location', 'best');