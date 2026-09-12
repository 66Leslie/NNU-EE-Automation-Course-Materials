% --- 1. 定义矩阵 A ---
A = [ 0,   1,  -1;
     -6, -11,   6;
     -6, -11,   5];

% --- 2. 求特征多项式 ---
P_coeffs = poly(A);
disp('--- A 的特征多项式系数 ---');
disp(P_coeffs);

% --- 3. 求特征值 ---
eigenvalues = eig(A);
disp('--- A 的特征值 ---');
disp(eigenvalues);

% --- 4. 求对角型 J 和 变换矩阵 T ---
% [T, J] = eig(A) 会找到 T 和 J 
% 使得 A*T = T*J 或 A = T*J*inv(T)
% J 是对角矩阵 (即本题的对角型)
% T 是由特征向量组成的变换矩阵
[T, J] = eig(A);

disp('--- 对角型 J ---');
disp(J);
disp('--- 变换矩阵 T ---');
disp(T);
