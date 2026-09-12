%关节定义
% th d a alpha sigma
L(1) = Link([ 0 5 0 -pi/2 0]);
L(2) = Link([ 0 5 0 pi/2 0]);
L(3) = Link([ 0 0 0 0 1]); % PRISMATIClink,
L(4) = Link([ 0 0 0 -pi/2 0]);
L(5) = Link([ 0 0 0 pi/2 0]);
L(6) = Link([ 0 5 0 0 0]);
%关节参数范围限定
L(1).qlim = [-170 170]*pi/180;
L(2).qlim = [-170 170]*pi/180;
L(3).qlim = [5 15];
L(4).qlim = [-170 170]*pi/180;
L(5).qlim = [-90 90]*pi/180;
L(6).qlim = [-170 170]*pi/180;
robot = SerialLink(L, 'name', 'Nnu');%创建机器人
init = [0 0 0 0 0 0];%初始关节参数
%工作区定义
w=[-20,20,-20,20,-20,20];
figure
robot.plot(init,'workspace',w);%画出图像
title('初始状态');
t = 0:0.5:8;%采样时间
%期望关节参数 第一组
aid0 = [0 -pi/4 10 0 -pi/2 0];
robot.plot(aid0,"workspace",w);
%运动轨迹求解，第一组
[q0,qd0,qdd0]=jtraj(init,aid0,t);%运动指标求解，q为位移，qd为速度，qdd为加速度
aid1 = [-pi/2 pi/4 15 0 pi/2 0];
for i = 1:20
    aid1 = [-pi/i pi/i 10 5 0 5];
    robot.plot(aid1,"workspace",w);
end
%运动轨迹求解，第二组
[q1,qd1,qdd1]=jtraj(init,aid1,t);%q为位移，qd为速度，qdd为加速度
% init_ang=[0 0 0 0 0 0];
% targ_ang=[pi/4,-pi/3,pi/5,pi/2,-pi/4,pi/6];
%step=40;
%[q,qd,qdd] = jtraj(aid0, aid1, step);
%subplot(3,2,[1,3]);
q2=q1(17,:);
robot.plot(q2);
%第一组
Theta0 =q0(17,:)/pi*180;
Theta0(3)=Theta0(3)/180*pi;
Theta0
%第二组
Theta1 =q1(17,:)/pi*180;
Theta1(3)=Theta1(3)/180*pi;
Theta1
%正运动学分析 第一组
T0=robot.fkine(aid0);
T0
%逆运动学分析 第一组
theta0 =robot.ikine(T0);
theta0 = theta0/pi*180;%转换为角度
theta0(3) = theta0(3)/180*pi;%由于第三关节是移动副，所以这里将错乘的转换回来
theta0
%正运动学分析 第二组
T1=robot.fkine(aid1);
T1
%逆运动学分析 第二组
theta1 =robot.ikine(T1);
theta1 = theta1/pi*180;
theta1(3) = theta1(3)/180*pi;
theta1


