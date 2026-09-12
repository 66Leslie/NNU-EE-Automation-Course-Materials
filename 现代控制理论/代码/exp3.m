num = [1, -1];      % 分子系数: s - 1
den = [1, 1, -2];   % 分母系数: s^2 + s - 2
sys_tf = tf(num, den);
sys_ss = ss(sys_tf);
A = sys_ss.A;
B = sys_ss.B;
C = sys_ss.C;
D = sys_ss.D;
n = size(A, 1); 
Co = ctrb(A, B);
rank_Co = rank(Co); 
disp('能控性矩阵M = ');
disp(Co);
fprintf('能控性矩阵的秩 rank(M) = %d\n', rank_Co);
if rank_Co == n
    disp('结论：系统是【状态完全能控】的。');
    disp(' ');
else
    disp('结论：系统是【不可控】的。');
end
Ob = obsv(A, C); 
rank_Ob = rank(Ob); 
disp('能观测性矩阵 N = ');
disp(Ob);
fprintf('能观测性矩阵的秩 rank(N) = %d\n', rank_Ob);
if rank_Ob == n
    disp('结论：系统是【状态完全能观测】的。');
else
    disp('结论：系统是【不可观测】的。');
end