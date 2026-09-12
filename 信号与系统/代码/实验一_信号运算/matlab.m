% 定义时间向量
t = -5:0.01:5;

% 定义单位阶跃函数
u = @(t) double(t >= 0);

% 情况1：f1(t)和f2(t)都在[-1,1]区间
t1 = -1; t2 = 1; t3 = -1; t4 = 1;

% 定义矩形脉冲信号
f1 = @(t) u(t - t1) - u(t - t2);
f2 = @(t) u(t - t3) - u(t - t4);

f1_t = f1(t);
f2_t = f2(t);

% 计算卷积
conv_f1_f2 = conv(f1_t, f2_t) * 0.01;
t_conv = -10:0.01:10;

% 绘制第一种情况
figure;
set(gcf, 'Color', 'white');
subplot(3,1,1);
plot(t, f1_t, 'LineWidth', 1.5);
title('信号 f_1(t)');
xlabel('t');
ylabel('f_1(t)');
grid on;

subplot(3,1,2);
plot(t, f2_t, 'LineWidth', 1.5);
title('信号 f_2(t)');
xlabel('t');
ylabel('f_2(t)');
grid on;

subplot(3,1,3);
plot(t_conv, conv_f1_f2, 'LineWidth', 1.5);
title('卷积 f_1(t) * f_2(t)');
xlabel('t');
ylabel('f_1(t) * f_2(t)');
grid on;

% 情况2：f1(t)在[-1,1]，f2(t)在[0,4]
t1 = -1; t2 = 1; t3 = 0; t4 = 4;

f1 = @(t) u(t - t1) - u(t - t2);
f2 = @(t) u(t - t3) - u(t - t4);

f1_t = f1(t);
f2_t = f2(t);

% 计算卷积
conv_f1_f2 = conv(f1_t, f2_t) * 0.01;

% 绘制第二种情况
figure;
set(gcf, 'Color', 'white');
subplot(3,1,1);
plot(t, f1_t, 'LineWidth', 1.5);
title('信号 f_1(t)');
xlabel('t');
ylabel('f_1(t)');
grid on;

subplot(3,1,2);
plot(t, f2_t, 'LineWidth', 1.5);
title('信号 f_2(t)');
xlabel('t');
ylabel('f_2(t)');
grid on;

subplot(3,1,3);
plot(t_conv, conv_f1_f2, 'LineWidth', 1.5);
title('卷积 f_1(t) * f_2(t)');
xlabel('t');
ylabel('f_1(t) * f_2(t)');
grid on;