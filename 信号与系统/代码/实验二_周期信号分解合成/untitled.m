f = 100; % 基波频率
Nh = 4;  % 我们需要4个谐波滤波器
harmonics_filters = filter_b(f, Nh);
% 设计一个低通滤波器来提取直流分量
wp_dc = 10 * 2 * pi; % 通带截止频率，例如 10Hz
ws_dc = 40 * 2 * pi; % 阻带起始频率，例如 40Hz
[N_dc, Wn_dc] = buttord(wp_dc, ws_dc, 1, 20, 's');
[num_dc, den_dc] = butter(N_dc, Wn_dc, 'low', 's');