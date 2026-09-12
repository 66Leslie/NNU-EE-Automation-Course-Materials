% --- 1. 定义原始矩阵 ---
A = [0, 1; -2, -3];
B = [0; 1];
C = [1, 0];
D = 0;

% --- 2. 定义变换矩阵 T ---
T = [2, 0; 0, -3];

% --- 3. 计算 T 的逆矩阵 ---
% 对于对角矩阵，也可以手动写 inv_T = [1/2, 0; 0, -1/3];
inv_T = inv(T);

% --- 4. 根据公式计算变换后的模型 ---
A_bar = inv_T * A * T;
B_bar = inv_T * B;
C_bar = C * T;
D_bar = D; % D 矩阵在相似变换下不变

% --- 5. 显示运行结果 ---
disp('--- 变换后的状态空间模型 ---');
disp('A_bar 矩阵:');
disp(A_bar);
disp('B_bar 矩阵:');
disp(B_bar);
disp('C_bar 矩阵:');
disp(C_bar);
disp('D_bar 矩阵:');
disp(D_bar);