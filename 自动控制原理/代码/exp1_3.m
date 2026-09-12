function exp1_3()
    num1 = [2,3];den1 = [5,2,2];
    g1 = tf(num1,den1);
    z =[-2];p=[-0.5,-8];k=5;
    g2 = zpk(z,p,k);
    [num2,den2] = zp2tf(z,p,k);
    num3 = [2,6];den3=[1,1,8];
    g3 = tf(num3,den3);
    num4 = [1,4];
    p1=[1,1];p2=[1,4,1];
    den4 = conv(p1,p2);
    [num,den] = series(num1,den1,num2,den2);
    [num,den] = parallel(num,den,num3,den3);
    [num,den] = series(num,den,num4,den4);
    [num,den] = feedback(num,den,1,1);
    g = tf(num,den)
    display(g)
    set(gcf, 'Color', 'w');
end