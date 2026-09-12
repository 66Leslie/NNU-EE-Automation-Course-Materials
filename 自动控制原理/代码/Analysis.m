function Analysis()
    % 1. 定义原始系统的传递函数
    s = tf('s');
    G_s = 8 / (s * (0.5*s + 1) * (0.25*s + 1));

    % 打印原始系统传递函数
    disp('原始系统开环传递函数 G(s):');
    G_s

    % 2. 分析原始系统的伯德图和性能指标
    figure;
    margin(G_s);
    title('原始系统 G(s) 的伯德图');
    grid on;

    [Gm, Pm, Wcg, Wcp] = margin(G_s);
    fprintf('原始系统性能指标:\n');
    fprintf('  幅值裕度 (Gm_dB): %.2f dB (在频率 Wcg = %.2f rad/s)\n', 20*log10(Gm), Wcg);
    fprintf('  相角裕度 (Pm): %.2f deg (在频率 Wcp = %.2f rad/s)\n', Pm, Wcp);
    fprintf('  剪切频率 (Wcp): %.2f rad/s\n', Wcp);

    % 设计目标
    Pm_desired = 40; % 期望的相角裕度 (degrees)
    Gm_desired_dB = 10; % 期望的幅值裕度 (dB)
    Wcp_desired_min = 1; % 期望的最小剪切频率 (rad/s)

    % 3. 设计超前校正器 Gc(s) = K_c * (T*s + 1) / (alpha*T*s + 1),  alpha < 1
    % 或者 Gc(s) = (s + z) / (s + p), p > z

    % 通常需要一些迭代或经验来选择参数
    % 让我们尝试一个设计策略：
    % 目标：提高相角裕度，并可能需要调整增益以满足幅值裕度。

    % 步骤 3.1: 确定在某个期望的剪切频率 wc_new 下，系统需要的附加相位
    % 假设我们选择一个新的剪切频率，例如 wc_new = 2.5 rad/s (需要大于1 rad/s)
    wc_new = 2.5; % rad/s

    % 计算在该频率下原始系统的相位
    [mag_G_wc_new, phase_G_wc_new] = bode(G_s, wc_new);
    phase_G_wc_new_deg = phase_G_wc_new; % bode返回的是度

    % 需要的附加相位 phi_m
    % 我们通常会多加几度 (例如 5-12度) 作为安全余量
    phi_needed = Pm_desired - (180 + phase_G_wc_new_deg);
    phi_m = phi_needed + 10; % 增加10度安全余量

    fprintf('\n在 wc_new = %.2f rad/s 时:\n', wc_new);
    fprintf('  原始系统相位: %.2f deg\n', phase_G_wc_new_deg);
    fprintf('  需要的相位补偿 (phi_needed): %.2f deg\n', phi_needed);
    fprintf('  设计的最大超前角 (phi_m): %.2f deg\n', phi_m);

    if phi_m <= 0
        error('计算得到的所需超前角过小或为负，请重新评估 wc_new 或设计策略。');
    end
    if phi_m > 60 % 通常单个超前校正器提供的最大超前角不宜超过60-70度
        warning('需要的最大超前角较大 (%.2f deg)，可能需要双重超前校正或重新选择 wc_new。', phi_m);
    end

    % 步骤 3.2: 计算 alpha
    alpha = (1 - sind(phi_m)) / (1 + sind(phi_m));
    fprintf('  计算得到的 alpha: %.4f\n', alpha);

    % 步骤 3.3: 计算校正器增益在 wc_new 处应为 -10*log10(alpha) dB
    % 或者说，校正器在 wc_new 处的幅值为 1/sqrt(alpha)
    % 为了使 wc_new 成为校正后系统的剪切频率，
    % |Gc(j*wc_new) * G(j*wc_new)| = 1
    % |Gc(j*wc_new)| * mag_G_wc_new = 1
    % 1/sqrt(alpha) * mag_G_wc_new = 1  <-- 这是如果校正器不带增益 Kc 的情况
    % 我们需要调整 Kc 使得 |Kc * (1/sqrt(alpha)) * G(j*wc_new)| = 1
    % Kc * (1/sqrt(alpha)) * mag_G_wc_new = 1
    % Kc = sqrt(alpha) / mag_G_wc_new

    % 步骤 3.4: 计算 T
    % 最大超前角发生在 omega_m = 1 / (T * sqrt(alpha))
    % 我们希望 omega_m = wc_new
    % 所以 T = 1 / (wc_new * sqrt(alpha))
    T = 1 / (wc_new * sqrt(alpha));
    fprintf('  计算得到的 T: %.4f\n', T);

    % 校正器的零点和极点
    z_c = 1/T;
    p_c = 1/(alpha*T);
    fprintf('  校正器零点 z_c: %.4f\n', z_c);
    fprintf('  校正器极点 p_c: %.4f\n', p_c);

    % 构造校正器传递函数 Gc(s) = (T*s + 1) / (alpha*T*s + 1)
    % 为了满足幅值条件，我们需要在校正器中加入一个增益 Kc
    % 在 wc_new 处，|G(j*wc_new)| = mag_G_wc_new
    % 在 wc_new 处，超前校正器提供的幅值为 1/sqrt(alpha)
    % 为了使 wc_new 成为新的剪切频率，K_total * mag_G_wc_new * (1/sqrt(alpha)) = 1
    % 这里的 K_total 是指整个开环传递函数的总增益。
    % 原系统增益是8。
    % 如果我们用 Gc_s_no_K = (T*s+1)/(alpha*T*s+1)，那么在 wc_new 处，|Gc_s_no_K(j*wc_new)| = 1/sqrt(alpha)
    % 校正后系统 G_comp = Kc * Gc_s_no_K * G_s
    % 我们需要 |Kc * Gc_s_no_K(j*wc_new) * G_s(j*wc_new)| = 1
    % Kc * (1/sqrt(alpha)) * mag_G_wc_new = 1
    % Kc = sqrt(alpha) / mag_G_wc_new;
    % 这里的 Kc 是指附加到校正器上的增益。
    % Gc_s = Kc * (T*s + 1) / (alpha*T*s + 1);

    % 让我们先不加 Kc，看看 G_s * (T*s+1)/(alpha*T*s+1) 的情况
    Gc_lead_no_K = (T*s + 1) / (alpha*T*s + 1);
    G_compensated_no_K = G_s * Gc_lead_no_K;

    % 查看此时在 wc_new 处的幅值
    [mag_comp_no_K_at_wc_new, ~] = bode(G_compensated_no_K, wc_new);

    % 计算需要的增益调整 K_adj
    % 我们希望 mag_comp_no_K_at_wc_new * K_adj = 1
    K_adj = 1 / mag_comp_no_K_at_wc_new;
    fprintf('  需要的增益调整 K_adj: %.4f (%.2f dB)\n', K_adj, 20*log10(K_adj));

    % 校正器传递函数 (包含增益调整)
    Gc_s = K_adj * (T*s + 1) / (alpha*T*s + 1);
    disp('设计的串联超前校正器 Gc(s):');
    Gc_s

    % 4. 校正后系统的开环传递函数
    G_compensated = G_s * Gc_s;
    % 或者 G_compensated = K_adj * G_compensated_no_K;
    disp('校正后系统的开环传递函数 G_comp(s):');
    G_compensated

    % 5. 分析校正后系统的伯德图和性能指标
    figure;
    margin(G_compensated);
    title('校正后系统 G_{comp}(s) 的伯德图');
    grid on;

    [Gm_comp, Pm_comp, Wcg_comp, Wcp_comp] = margin(G_compensated);
    fprintf('\n校正后系统性能指标:\n');
    fprintf('  幅值裕度 (Gm_comp_dB): %.2f dB (在频率 Wcg_comp = %.2f rad/s)\n', 20*log10(Gm_comp), Wcg_comp);
    fprintf('  相角裕度 (Pm_comp): %.2f deg (在频率 Wcp_comp = %.2f rad/s)\n', Pm_comp, Wcp_comp);
    fprintf('  剪切频率 (Wcp_comp): %.2f rad/s\n', Wcp_comp);

    % 验证设计结果
    fprintf('\n设计目标与结果对比:\n');
    fprintf('相角裕度: 目标 >= %.1f deg, 实际 = %.2f deg\n', Pm_desired, Pm_comp);
    fprintf('幅值裕度: 目标 >= %.1f dB,  实际 = %.2f dB\n', Gm_desired_dB, 20*log10(Gm_comp));
    fprintf('剪切频率: 目标 > %.1f rad/s, 实际 = %.2f rad/s\n', Wcp_desired_min, Wcp_comp);

    if Pm_comp >= Pm_desired && 20*log10(Gm_comp) >= Gm_desired_dB && Wcp_comp > Wcp_desired_min
        fprintf('\n设计成功，所有指标均满足要求。\n');
    else
        fprintf('\n设计未完全满足所有指标，可能需要进一步调整校正器参数或设计策略。\n');
        if Pm_comp < Pm_desired
            fprintf('  相角裕度不足 (实际 %.2f deg vs 目标 %.1f deg)。可尝试增大 phi_m (减小 alpha) 或调整 wc_new。\n', Pm_comp, Pm_desired);
        end
        if 20*log10(Gm_comp) < Gm_desired_dB
            fprintf('  幅值裕度不足 (实际 %.2f dB vs 目标 %.1f dB)。可尝试调整增益 Kc 或重新设计。\n', 20*log10(Gm_comp), Gm_desired_dB);
        end
        if Wcp_comp <= Wcp_desired_min
            fprintf('  剪切频率不足 (实际 %.2f rad/s vs 目标 > %.1f rad/s)。可尝试选择更大的 wc_new。\n', Wcp_comp, Wcp_desired_min);
        end
    end
end
