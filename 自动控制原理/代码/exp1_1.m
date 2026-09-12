function exp1_1()
    num = [3, 6, 4];
    den = [1, 3, 8, 4, 2];
    [a,b,c] = tf2zp(num,den);
    zpk(a,b,c)
    subplot(3,1,1)
    pzmap(num,den)
    subplot(3,1,2)
    step(num,den)
    subplot(3,1,3)
    impulse(num,den)
    set(gcf, 'Color', 'w');
end