% --- 第1部分：设置机器人（PUMA 560 改进DH参数） ---
% 这个脚本需要 Robotics System Toolbox

clear;
clc;

% --- 修正点在这里 ---
% 1. 定义符号变量，用于之后的计算
syms theta1 theta2 theta3 theta4 theta5 theta6 real
q_sym = [theta1, theta2, theta3, theta4, theta5, theta6].';

% 2. 定义机器人的【数字】参数，用于“建造”模型
a2 = 0.4318;
a3 = 0.02032;
d3 = 0.14009; % (来自左侧表格的d2，或右侧表格的d3)
d4 = 0.43307;

% 改进DH参数 [a_i-1, alpha_i-1, d_i, theta_i_offset]
% 注意：第四列是关节“偏移量”，对于PUMA 560所有关节都是0。
% 【错误的代码】在这里使用了符号变量(theta1...theta6)
% 【正确的代码】在这里使用数字0
dh_params_numeric = [ 0,   0,      0,   0;
                      0,   -pi/2,  0,   0;
                      a2,  0,      d3,  0;
                      a3,  -pi/2,  d4,  0;
                      0,   pi/2,   0,   0;
                      0,   -pi/2,  0,   0 ];
% ---------------------

% 创建刚体树 (Rigid Body Tree)
robot = rigidBodyTree('DataFormat', 'column');

% 循环添加连杆
bodies = cell(6,1);
joints = cell(6,1);
lastBodyName = 'base';

for i = 1:6
    bodies{i} = rigidBody(['body' num2str(i)]);
    joints{i} = rigidBodyJoint(['jnt' num2str(i)], 'revolute');
    
    % --- 修正点在这里 ---
    % 使用【数字】参数来设置DH
    setFixedTransform(joints{i}, dh_params_numeric(i,:), 'mdh');
    % ---------------------
    
    bodies{i}.Joint = joints{i};
    addBody(robot, bodies{i}, lastBodyName);
    lastBodyName = bodies{i}.Name;
end

% 添加末端执行器
eeName = 'endeffector';
ee = rigidBody(eeName);
ee.Joint = rigidBodyJoint('ee_fix', 'fixed');
setFixedTransform(ee.Joint, trvec2tform([0 0 0])); % 假设EE与body6重合
addBody(robot, ee, lastBodyName);

fprintf('机器人模型 PUMA 560 (改进DH) 已创建。\n');
fprintf('----------------------------------------\n\n');


% --- 第2部分：计算数值解（这部分没有改动） ---
% 这会计算一个特定姿态下的 6x6 雅可比矩阵

% 示例姿态（例如，所有关节角为0）
q_zero = [0, 0, 0, 0, 0, 0].'; 

% 1. 计算基坐标系{0}下的雅可比矩阵 J0
J0_numeric = geometricJacobian(robot, q_zero, eeName);

% 2. 计算从{0}到{6}的变换
T0_6_numeric = getTransform(robot, q_zero, eeName);
R0_6_numeric = T0_6_numeric(1:3, 1:3);

% 3. 构建变换矩阵
T_mat_numeric = [R0_6_numeric.', zeros(3); zeros(3), R0_6_numeric.'];

% 4. 计算 J6
J6_numeric = T_mat_numeric * J0_numeric;

fprintf('【数值解】在 q = [0,0,0,0,0,0] 姿态下：\n');
disp('R0_6 (旋转矩阵):');
disp(R0_6_numeric);
disp('J6 (在{6}中的雅可比矩阵):');
disp(J6_numeric);
fprintf('----------------------------------------\n\n');


% --- 第3部分：计算符号解（!!! 警告 !!!）（这部分没有改动） ---
% 这部分的计算量非常大，在手机上可能需要几分钟，甚至失败。
% 输出的 J6_sym 将会非常非常长！

fprintf('正在计算符号雅可比矩阵... 这可能需要很长时间...\n');

try
    % 1. 计算基坐标系{0}下的符号雅可比矩阵 J0_sym
    %    【注意】这里我们把符号变量 q_sym 传递给了函数
    J0_sym = geometricJacobian(robot, q_sym, eeName);
    
    % 2. 计算符号变换矩阵 T0_6_sym
    T0_6_sym = getTransform(robot, q_sym, eeName);
    R0_6_sym = T0_6_sym(1:3, 1:3);
    
    % 3. 构建符号变换矩阵
    T_mat_sym = [R0_6_sym.', zeros(3); zeros(3), R0_6_sym.'];
    
    % 4. 计算 J6_sym
    J6_sym = T_mat_sym * J0_sym;
    
    fprintf('符号计算 J6_sym 完成。\n');
    
    % (为防止MATLAB Mobile崩溃，我们先显示未化简的版本)
    fprintf('【符号解】(未化JSON) J6_sym 的 (1,1) 元素：\n');
    disp(J6_sym(1,1));
    
    fprintf('\n!!! 警告：完整的 J6_sym 矩阵非常庞大。\n');
    fprintf('!!! 它与你书上的简化答案在形式上完全不同。\n');
    fprintf('!!! 不推荐将其抄录到作业本上。\n');

catch ME
    fprintf('\n!!! 计算符号解时出错 !!!\n');
    fprintf('错误信息: %s\n', ME.message);
    fprintf('这可能是因为MATLAB Mobile内存不足，或者符号计算过于复杂。\n');
end