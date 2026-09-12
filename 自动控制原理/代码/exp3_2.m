function exp3_2()
    n=[1280 640];
    d=[1 24.2 1604.81 320.24 16];
    figure(1)
    nyquist(n,d)

    figure(2)
    margin(n,d)

    [z,p,k] = tf2zp(n,d)
    G = zpk(z,p,k)
end




