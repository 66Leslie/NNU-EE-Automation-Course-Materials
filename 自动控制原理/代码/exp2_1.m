function exp2_1()
    %%EXP2-1:Q1
    n1 = [ 2 3];d1 = [5 2 2];
    n2 = 5*[1 2];d2_1 = [1 0.5];d2_2 = [1 8];
    d2 = conv(d2_1,d2_2);
    [num,den] = series (n1,d1,n2,d2);
    n3 = [2 6];d3 = [1 1 8];%G3
    [num,den] = parallel(num,den,n3,d3);
    G = tf(num,den);
    [num,den] = cloop(num,den,-1);
    tf(num,den)
    %%EXP2-1:Q2
    step(num,den)
    %%EXP2-1:Q3
    [y, x, t] = step(num,den);
    ess = dcgain(G);
    ess = 1/(1+ess)
end

