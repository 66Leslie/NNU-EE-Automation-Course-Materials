A = [0 1 0; 0 0 1; 0 -2 -3];
B = [0; 0; 1];
C_original = [1 0 0];
D = 0;

open_loop_poles = eig(A);
disp('-----------------------------------------');
disp('(1) 系统开环极点 (Open-loop Poles):');
disp(open_loop_poles); 

C_states = eye(3);
D_states = [0; 0; 0];
sys_states_open = ss(A, B, C_states, D_states);
t_final = 100; 
[Y_states, t_open] = step(sys_states_open, t_final);
x1_data = Y_states(:,1);
x2_data = Y_states(:,2);
x3_data = Y_states(:,3);

figure; 
hold on; 
plot(t_open, x1_data, 'b', 'LineWidth', 1.5, 'DisplayName', 'x_1(t)');
plot(t_open, x2_data, 'r--', 'LineWidth', 1.5, 'DisplayName', 'x_2(t)');
plot(t_open, x3_data, 'g:', 'LineWidth', 1.5, 'DisplayName', 'x_3(t)');
title('开环系统状态变量的阶跃响应');
xlabel('时间 (s)');
ylabel('幅值');
legend('show', 'Location', 'best'); 
grid on; 
hold off; 