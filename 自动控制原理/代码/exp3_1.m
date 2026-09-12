function exp3_1()
    n=[1280 640];
    d=[1 24.2 1604.81 320.24 16];
    figure(1)
    set(gcf, 'Color', 'w');
    nyquist(n,d)

    figure(2)
    margin(n,d)

    [z,p,k] = tf2zp(n,d)
    G = zpk(z,p,k)
    % 计算单位负反馈的闭环系统
    T = feedback(G, 1);

    % 计算闭环系统的极点
    closed_loop_poles = pole(T);

    fprintf('\n闭环系统极点 (Closed-loop poles):\n');
    disp(closed_loop_poles);
    set(gcf, 'Color', 'w');
end




