clc;
clear;
close all;
den = [1, 2, 3, 4];
num = [0, 0, 1, 2;
       0, 1, 5, 3];
[A, B, C, D] = tf2ss(num, den)