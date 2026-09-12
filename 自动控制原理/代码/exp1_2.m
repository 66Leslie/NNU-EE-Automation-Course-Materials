function exp1_2()    %proportion
    num1=[2];
    num2=[8];
    den =[1];
    subplot(3,2,1)
    set(gca,'Color','white');
    step(num1,den);
    hold on;
    step(num2,den);
    hold off;
    title('比例环节');
    legend('G_1=2','G_2=8');
    clear;    %inertia
    den1=[4,1];den2=[10,1];
    num = [1];
    subplot(3,2,2)
    set(gca,'Color','white');
    step(num,den1);
    hold on;
    step(num,den2);
    hold off;
    title('惯性环节');
    legend('T_1=4','T_2=10');
    clear    %integration
    den1=[3,0];den2=[12,0];
    num=[1];
    subplot(3,2,3)
    set(gca,'Color','white');
    step(num,den1);
    hold on;
    step(num,den2);
    hold off;
    title('积分环节');
    legend('T_1=3','T_2=12');
    clear    %deferentiate
    a1=[3,0];b1=[3,1];
    a2=[12,0];b2=[12,1];
    subplot(3,2,4)
    set(gca,'Color','white');
    step(a1,b1);
    hold on;
    step(a2,b2);
    hold off;
    title('微分环节');
    legend('T_1=3','T_2=12');
    clear;    %zhendang
    subplot(3,2,5)
    set(gca,'Color','white');
    kexi = 0.2;w =3;
    num = [w*w];den=[1,2*kexi*w,w*w];
    step(num,den);
    hold on;
    kexi = 0.4;
    num = [w*w];den=[1,2*kexi*w,w*w];
    step(num,den);
    kexi = 0.8;
    num = [w*w];den=[1,2*kexi*w,w*w];
    step(num,den);
    hold off;
    title('振荡环节');
    legend('ζ=0.2','ζ=0.4','ζ=0.8');
    set(gcf, 'Color', 'w');
end