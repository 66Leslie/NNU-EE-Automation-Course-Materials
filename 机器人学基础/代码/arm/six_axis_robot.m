function six_axis_robot()
    % SIX_AXIS_ROBOT Mycobot六轴机械臂交互式控制演示
    % 清除环境
    close all;
    clc;
    startup_rvc; % 确保工具箱已启动

    %% 1. 定义Mycobot六轴机械臂模型 (使用标准DH参数)
    % DH参数: [theta, d, a, alpha]
    % Link (i), ai (Link Length), αi (Link Twist), di (Link Offset), θi (Joint Angle)
    L(1) = Revolute('d', 0.13156, 'a', 0,       'alpha', pi/2,  'qlim', [-165 165]*pi/180);
    L(2) = Revolute('d', 0,       'a', -0.1104, 'alpha', 0,     'qlim', [-165 165]*pi/180);
    L(3) = Revolute('d', 0,       'a', -0.096,  'alpha', 0,     'qlim', [-158 165]*pi/180);
    L(4) = Revolute('d', 0.06462, 'a', 0,       'alpha', pi/2,  'qlim', [-165 165]*pi/180);
    L(5) = Revolute('d', 0.07318, 'a', 0,       'alpha', -pi/2, 'qlim', [-165 165]*pi/180);
    L(6) = Revolute('d', 0.0456,  'a', 0,       'alpha', 0,     'qlim', [-165 165]*pi/180);
    
    robot = SerialLink(L, 'name', 'Mycobot 六轴机械臂');
    
    % 初始关节角度 (直立姿态)
    % J2, J3 设为 -90° 使机械臂完全直立
    current_q = [0 -pi/2 0 -pi/2 0 0];
    
    % 使用蒙特卡洛法计算可达工作空间
    fprintf('正在计算工作空间...');
    num_samples = 5000; % 采样点数
    workspace_points = compute_workspace(robot, num_samples);
    fprintf(' 完成!\n');
    
    % 计算工作空间统计
    distances = sqrt(sum(workspace_points.^2, 2));
    max_reach = max(distances);
    min_reach = min(distances(distances > 0.05)); % 排除接近零的点
    fprintf('最大可达半径: %.4f m\n', max_reach);
    fprintf('最小可达半径: %.4f m\n', min_reach);

    % 用于在图中显示的工作空间信息（更直观的“信息卡片”排版）
    ws_info_str = sprintf([ ...
        '可达空间（蓝色半透明）\n', ...
        '方法：蒙特卡洛采样\n', ...
        '——————————————\n', ...
        '采样点数：%d\n', ...
        '最大半径：%.3f m\n', ...
        '最小半径：%.3f m' ...
        ], size(workspace_points, 1), max_reach, min_reach);
    
    % 计算工作空间的中心点和半径，用于绘制球体
    workspace_center = mean(workspace_points, 1);
    workspace_radius = max_reach * 0.8; % 使用最大可达半径的80%作为球体半径

    %% 2. 创建图形界面 (GUI)
    % 创建主窗口 (固定大小 800x600)
    fig = figure('Name', 'Mycobot 机械臂控制台', 'NumberTitle', 'off', ...
        'Color', [1 1 1], 'Position', [50, 50, 1024, 700], ...
        'MenuBar', 'none', 'ToolBar', 'figure');

    % ========== 左侧：机器人3D视图 ==========
    ax = axes('Parent', fig, 'Position', [0.04, 0.05, 0.45, 0.90]);
    robot.plot(current_q, 'workspace', [-0.45 0.45 -0.45 0.45 -0.1 0.6], ...
        'view', [45 30], 'noshadow', 'noname', 'scale', 0.8);
    hold(ax, 'on');
    [sx, sy, sz] = sphere(30);
    sx = workspace_center(1) + workspace_radius * sx;
    sy = workspace_center(2) + workspace_radius * sy;
    sz = workspace_center(3) + workspace_radius * sz;
    surf(ax, sx, sy, sz, 'FaceAlpha', 0.05, 'EdgeColor', 'none', 'FaceColor', [0.2 0.6 1]);
        % 可达空间文字标注
        text(ax, 0.02, 0.98, ws_info_str, ...
            'Units', 'normalized', ...
            'VerticalAlignment', 'top', ...
            'HorizontalAlignment', 'left', ...
            'FontSize', 8, ...
            'BackgroundColor', [1 1 1], ...
            'EdgeColor', [0.85 0.85 0.85], ...
            'Margin', 4, ...
            'Interpreter', 'none', ...
            'Tag', 'WorkspaceInfo');
    title(ax, 'Mycobot 280', 'FontSize', 11, 'FontWeight', 'bold');
    xlabel(ax, 'X', 'FontSize', 9);
    ylabel(ax, 'Y', 'FontSize', 9);
    zlabel(ax, 'Z', 'FontSize', 9);
    grid(ax, 'on');
    hold(ax, 'off');

    % ========== 右侧：控制面板 (并列排列) ==========
    color_bg = [0.96 0.96 0.96];
    
    % 正向运动学面板 (右上左侧)
    panel_fk = uipanel('Parent', fig, 'Title', '正向运动学', ...
        'Position', [0.51, 0.75, 0.24, 0.23], 'FontSize', 8, ...
        'BackgroundColor', color_bg);
    
    slider_joints = cell(6, 1);
    lbl_joints = cell(6, 1);
    edit_joints = cell(6, 1);

    % 关节限位（度）
    qlim_deg = robot.qlim * 180 / pi;
    
    for joint_idx = 1:6
        y_pos = 115 - (joint_idx-1)*19;
        uicontrol('Parent', panel_fk, 'Style', 'text', 'Position', [2, y_pos, 16, 12], ...
            'String', sprintf('J%d', joint_idx), 'BackgroundColor', color_bg, 'FontSize', 7);
        slider_joints{joint_idx} = uicontrol('Parent', panel_fk, 'Style', 'slider', ...
            'Position', [18, y_pos, 110, 12], ...
            'Min', qlim_deg(joint_idx, 1), 'Max', qlim_deg(joint_idx, 2), ...
            'Value', current_q(joint_idx) * 180 / pi, ...
            'Callback', @update_fk);
        edit_joints{joint_idx} = uicontrol('Parent', panel_fk, 'Style', 'edit', ...
            'Position', [125, y_pos, 40, 14], 'String', '0', 'FontSize', 7, ...
            'Callback', @update_fk_from_edit);
        uicontrol('Parent', panel_fk, 'Style', 'text', 'Position', [162, y_pos, 8, 12], ...
            'String', '°', 'BackgroundColor', color_bg, 'FontSize', 7);
        lbl_joints{joint_idx} = uicontrol('Parent', panel_fk, 'Style', 'text', ...
            'Position', [1,1,1,1], 'Visible', 'off');
    end

    % 同步初始化输入框显示
    for joint_idx = 1:6
        set(edit_joints{joint_idx}, 'String', sprintf('%.1f', current_q(joint_idx) * 180 / pi));
    end
    uicontrol('Parent', panel_fk, 'Style', 'text', 'Position', [2, 1, 168, 12], ...
        'String', '末端: [0.000, 0.000, 0.000]', 'BackgroundColor', [0.9 0.95 1], ...
        'FontSize', 7, 'Tag', 'EndEffectorPos');

    % 逆向运动学面板 (右上右侧)
    panel_ik = uipanel('Parent', fig, 'Title', '逆向运动学', ...
        'Position', [0.76, 0.75, 0.23, 0.23], 'FontSize', 8, ...
        'BackgroundColor', color_bg);

    % 播放速度倍率（注意：这里只影响动画播放/绘图刷新速度，不影响逆解求解本身）
    anim_speed_levels = [0.5 1.0 2.0 3.0];
    anim_speed_idx = 2; % 默认 1.0x
    anim_speed_multiplier = anim_speed_levels(anim_speed_idx);
    
    uicontrol('Parent', panel_ik, 'Style', 'text', 'Position', [5, 100, 16, 12], ...
        'String', 'X:', 'BackgroundColor', color_bg, 'FontSize', 8, 'FontWeight', 'bold');
    edt_x = uicontrol('Parent', panel_ik, 'Style', 'edit', 'Position', [22, 100, 45, 14], ...
        'String', '0.15', 'FontSize', 8);
    uicontrol('Parent', panel_ik, 'Style', 'text', 'Position', [69, 100, 10, 12], ...
        'String', 'm', 'BackgroundColor', color_bg, 'FontSize', 7);
    
    uicontrol('Parent', panel_ik, 'Style', 'text', 'Position', [5, 80, 16, 12], ...
        'String', 'Y:', 'BackgroundColor', color_bg, 'FontSize', 8, 'FontWeight', 'bold');
    edt_y = uicontrol('Parent', panel_ik, 'Style', 'edit', 'Position', [22, 80, 45, 14], ...
        'String', '0.0', 'FontSize', 8);
    uicontrol('Parent', panel_ik, 'Style', 'text', 'Position', [69, 80, 10, 12], ...
        'String', 'm', 'BackgroundColor', color_bg, 'FontSize', 7);
    
    uicontrol('Parent', panel_ik, 'Style', 'text', 'Position', [5, 60, 16, 12], ...
        'String', 'Z:', 'BackgroundColor', color_bg, 'FontSize', 8, 'FontWeight', 'bold');
    edt_z = uicontrol('Parent', panel_ik, 'Style', 'edit', 'Position', [22, 60, 45, 14], ...
        'String', '0.25', 'FontSize', 8);
    uicontrol('Parent', panel_ik, 'Style', 'text', 'Position', [69, 60, 10, 12], ...
        'String', 'm', 'BackgroundColor', color_bg, 'FontSize', 7);
    
    uicontrol('Parent', panel_ik, 'Style', 'pushbutton', 'String', '执行移动', ...
        'Position', [90, 65, 70, 45], 'FontSize', 9, 'FontWeight', 'bold', ...
        'BackgroundColor', [0.2 0.7 0.3], 'ForegroundColor', 'white', ...
        'Callback', @move_robot_callback);

    % 调速按钮（循环切换倍率）
    btn_speed = uicontrol('Parent', panel_ik, 'Style', 'pushbutton', ...
        'Position', [5, 118, 155, 16], 'FontSize', 8, 'FontWeight', 'bold', ...
        'String', sprintf('播放速度: %.1fx', anim_speed_multiplier), ...
        'BackgroundColor', [0.9 0.95 1], 'ForegroundColor', [0 0 0], ...
        'TooltipString', '点击切换倍率：只影响动画播放/刷新频率', ...
        'Callback', @cycle_anim_speed);

    uicontrol('Parent', panel_ik, 'Style', 'text', 'Position', [162, 118, 45, 16], ...
        'String', '点击切换', 'BackgroundColor', color_bg, 'FontSize', 7, ...
        'HorizontalAlignment', 'left');
    
    uicontrol('Parent', panel_ik, 'Style', 'text', 'Position', [5, 38, 28, 12], ...
        'String', '状态:', 'BackgroundColor', color_bg, 'FontSize', 7, 'FontWeight', 'bold');
    lbl_status = uicontrol('Parent', panel_ik, 'Style', 'text', ...
        'Position', [5, 1, 170, 37], 'String', '系统就绪', ...
        'BackgroundColor', color_bg, 'ForegroundColor', [0 0.5 0], ...
        'FontSize', 7, 'HorizontalAlignment', 'left');

    % ========== 右下：6个运动分析图表 (2行3列) ==========
    motion_axes = cell(2, 3);
    jtitles = {'关节角度(°)', '关节角速度(°/s)', '关节角加速度(°/s²)'};
    etitles = {'末端位置(m)', '末端速度(m/s)', '末端加速度(m/s²)'};
    
    pw = 0.145; ph = 0.26;  % 减小图表高度避免重叠
    for row = 1:2
        for col = 1:3
            px = 0.51 + (col-1)*0.170;
            py = 0.39 - (row-1)*0.32;  % 增加行间距
            motion_axes{row, col} = axes('Parent', fig, 'Position', [px, py, pw, ph]);
            if row == 1
                plot(motion_axes{row, col}, 0, 0);
                title(motion_axes{row, col}, jtitles{col}, 'FontSize', 7);
            else
                plot(motion_axes{row, col}, 0, 0);
                title(motion_axes{row, col}, etitles{col}, 'FontSize', 7);
            end
            grid(motion_axes{row, col}, 'on');
            set(motion_axes{row, col}, 'FontSize', 6);
        end
    end

    %% 3. 回调函数定义
    % 在3D视图中绘制工作空间信息（每次cla后需要重绘）
    function draw_workspace_info()
        delete(findobj(ax, 'Tag', 'WorkspaceInfo'));
        text(ax, 0.02, 0.98, ws_info_str, ...
            'Units', 'normalized', ...
            'VerticalAlignment', 'top', ...
            'HorizontalAlignment', 'left', ...
            'FontSize', 8, ...
            'BackgroundColor', [1 1 1], ...
            'EdgeColor', [0.85 0.85 0.85], ...
            'Margin', 4, ...
            'Interpreter', 'none', ...
            'Tag', 'WorkspaceInfo');
    end

    % 动画速度倍率切换
    function cycle_anim_speed(~, ~)
        anim_speed_idx = anim_speed_idx + 1;
        if anim_speed_idx > numel(anim_speed_levels)
            anim_speed_idx = 1;
        end
        anim_speed_multiplier = anim_speed_levels(anim_speed_idx);
        set(btn_speed, 'String', sprintf('播放速度: %.1fx', anim_speed_multiplier));
    end

    % 在3D视图中标记不可达点（红色× + 文本）
    function mark_unreachable(x_val, y_val, z_val, label_text)
        if nargin < 4 || isempty(label_text)
            label_text = '不可达';
        end
        if isnan(x_val) || isnan(y_val) || isnan(z_val)
            return;
        end

        % 清除旧的不可达标记，避免残留干扰
        delete(findobj(ax, 'Tag', 'UnreachableMark'));

        hold(ax, 'on');
        h1 = plot3(ax, x_val, y_val, z_val, 'rx', 'MarkerSize', 15, 'LineWidth', 3);
        set(h1, 'Tag', 'UnreachableMark');
        h2 = text(ax, x_val, y_val, z_val + 0.02, label_text, 'Color', 'red', 'FontWeight', 'bold');
        set(h2, 'Tag', 'UnreachableMark');
        hold(ax, 'off');
    end
    
    % 正向运动学更新函数
    function update_fk(~, ~)
        % 读取所有滑动条的值
        q_deg = zeros(1, 6);
        for j = 1:6
            q_deg(j) = get(slider_joints{j}, 'Value');
            set(edit_joints{j}, 'String', sprintf('%.1f', q_deg(j)));
        end
        
        % 转换为弧度
        q_new = q_deg * pi / 180;
        
        % 计算正向运动学 (末端位置)
        T_end = robot.fkine(q_new);
        pos_end = transl(T_end);
        
        % 更新末端位置显示
        lbl_pos = findobj(fig, 'Tag', 'EndEffectorPos');
        set(lbl_pos, 'String', sprintf('末端: [%.3f, %.3f, %.3f]', pos_end(1), pos_end(2), pos_end(3)));
        
        % 更新机器人显示 - 确保在正确的axes中
        axes(ax);
        cla(ax);
        robot.plot(q_new, 'workspace', [-0.45 0.45 -0.45 0.45 -0.1 0.6], ...
            'view', [45 30], 'noshadow', 'noname', 'scale', 0.8);
        
        hold(ax, 'on');
        [sx, sy, sz] = sphere(20);
        sx = workspace_center(1) + workspace_radius * sx;
        sy = workspace_center(2) + workspace_radius * sy;
        sz = workspace_center(3) + workspace_radius * sz;
        surf(ax, sx, sy, sz, 'FaceAlpha', 0.03, 'EdgeColor', 'none', 'FaceColor', [0.2 0.6 1]);

        draw_workspace_info();
        
        % 移除了边界线绘制代码
        
        % 标注末端位置 (青色点)
        plot3(ax, pos_end(1), pos_end(2), pos_end(3), 'co', 'MarkerSize', 10, 'MarkerFaceColor', 'c', 'LineWidth', 1.5);
        hold(ax, 'off');
        
        title('Mycobot六轴机械臂', 'FontSize', 14, 'FontWeight', 'bold', 'Color', [0.2 0.2 0.2]);
        grid on; grid minor;
        
        % 更新当前关节角度
        current_q = q_new;
    end
    
    % 通过键盘输入更新正向运动学函数
    function update_fk_from_edit(~, ~)
        % 读取所有输入框的值
        q_deg = zeros(1, 6);
        for j = 1:6
            q_deg(j) = str2double(get(edit_joints{j}, 'String'));
            % 限制角度范围（按各关节真实限位）
            joint_min = get(slider_joints{j}, 'Min');
            joint_max = get(slider_joints{j}, 'Max');
            if q_deg(j) > joint_max
                q_deg(j) = joint_max;
            elseif q_deg(j) < joint_min
                q_deg(j) = joint_min;
            end
            set(edit_joints{j}, 'String', sprintf('%.1f', q_deg(j)));
            % 同步更新滑动条和标签
            set(slider_joints{j}, 'Value', q_deg(j));
            % set(lbl_joints{j}, 'String', sprintf('%.1f°', q_deg(j))); % 已隐藏
        end
        
        % 转换为弧度
        q_new = q_deg * pi / 180;
        
        % 计算正向运动学 (末端位置)
        T_end = robot.fkine(q_new);
        pos_end = transl(T_end); % 提取位置 [x, y, z]
        
        % 更新末端位置显示
        lbl_pos = findobj(fig, 'Tag', 'EndEffectorPos');
        set(lbl_pos, 'String', sprintf('末端: [%.3f, %.3f, %.3f]', pos_end(1), pos_end(2), pos_end(3)));
        
        % 更新机器人显示 - 确保在正确的axes中
        axes(ax);
        cla(ax);
        robot.plot(q_new, 'workspace', [-0.45 0.45 -0.45 0.45 -0.1 0.6], ...
            'view', [45 30], 'noshadow', 'noname', 'scale', 0.8);
            
        % 重新绘制工作空间球体
        hold(ax, 'on');
        [sx, sy, sz] = sphere(20);
        sx = workspace_center(1) + workspace_radius * sx;
        sy = workspace_center(2) + workspace_radius * sy;
        sz = workspace_center(3) + workspace_radius * sz;
        surf(ax, sx, sy, sz, 'FaceAlpha', 0.03, 'EdgeColor', 'none', 'FaceColor', [0.2 0.6 1]);

        draw_workspace_info();
        
        % 标注末端位置 (青色点)
        plot3(ax, pos_end(1), pos_end(2), pos_end(3), 'co', 'MarkerSize', 10, 'MarkerFaceColor', 'c', 'LineWidth', 1.5);
        hold(ax, 'off');
        
        title(ax, 'Mycobot六轴机械臂', 'FontSize', 12, 'FontWeight', 'bold');
        grid(ax, 'on');
        
        % 更新当前关节角度
        current_q = q_new;
    end
    
    % 逆向运动学移动函数
    function move_robot_callback(~, ~)
        move_completed = false;
        try
            % 获取输入值
            x_val = str2double(get(edt_x, 'String'));
            y_val = str2double(get(edt_y, 'String'));
            z_val = str2double(get(edt_z, 'String'));

            % 简单校验
            if isnan(x_val) || isnan(y_val) || isnan(z_val)
                set(lbl_status, 'String', '错误：请输入有效数字', 'ForegroundColor', 'red');
                return;
            end

            % 每次尝试前清除旧的不可达标记
            delete(findobj(ax, 'Tag', 'UnreachableMark'));

            set(lbl_status, 'String', '正在规划路径...', 'ForegroundColor', 'blue');
            drawnow;

            % 构建目标位姿矩阵 (位置 + 默认姿态)
            T_target = transl(x_val, y_val, z_val);

            % 逆运动学求解
            % 对于Mycobot六轴机械臂，我们关心位置和姿态，这里简化只控制位置
            q_target = robot.ikine(T_target, 'mask', [1 1 1 0 0 0], 'q0', current_q);

            % 检查是否有解
            if isempty(q_target) || any(isnan(q_target))
                % 在图中标注不可达点
                mark_unreachable(x_val, y_val, z_val, '不可达');
                
                set(lbl_status, 'String', '不可达：目标超出工作空间或逆解无解', 'ForegroundColor', 'red');
                return;
            end

            % 统一成行向量，避免后续维度问题
            q_target = q_target(:)';

            % 关节限位校验（防止出现超出关节真实限位的解）
            qlim = robot.qlim;
            if any(q_target < qlim(:,1)' - 1e-9) || any(q_target > qlim(:,2)' + 1e-9)
                mark_unreachable(x_val, y_val, z_val, '不可达');
                set(lbl_status, 'String', '不可达：逆解超出关节限位', 'ForegroundColor', 'red');
                return;
            end

            % 获取播放速度倍率（影响动画播放/绘图刷新）
            speed_multiplier = anim_speed_multiplier;

            % 轨迹规划 (用于计算/绘图的数据采样)
            steps = 40; % 保持采样步数固定，避免倍率影响曲线平滑度
            dt = 0.015; % 时间步长 (用于曲线横轴显示)
            time_data = (0:steps-1)' * dt;
            pause_time = max(0.001, 0.02 / speed_multiplier);
            frame_stride = max(1, round(speed_multiplier)); % 倍率越大，刷新越“跳帧”
            frame_idx = unique([1:frame_stride:steps, steps]);
            [q_traj, qd_traj, qdd_traj] = jtraj(current_q, q_target, steps);
            
            % 计算轨迹中每个点的末端位置、速度和加速度 (使用雅可比矩阵)
            trajectory_points = zeros(steps, 3);
            trajectory_vel = zeros(steps, 3);
            trajectory_acc = zeros(steps, 3);
            
            for k = 1:steps
                % 正向运动学 - 末端位置
                T_temp = robot.fkine(q_traj(k,:));
                trajectory_points(k,:) = transl(T_temp);
                
                % 雅可比矩阵 - 末端速度
                J = robot.jacob0(q_traj(k,:));  % 基坐标系雅可比矩阵
                twist = J * qd_traj(k,:)';      % 6x1 速度向量 [v; w]
                trajectory_vel(k,:) = twist(1:3)';  % 取线速度部分 [vx, vy, vz]
                
                % 雅可比矩阵 - 末端加速度
                % a = J*qdd + Jdot*qd
                % 简化：只考虑主要项 J*qdd
                twist_acc = J * qdd_traj(k,:)';
                trajectory_acc(k,:) = twist_acc(1:3)';
            end

            % 动画执行
            set(lbl_status, 'String', '正在移动...', 'ForegroundColor', [0 0.5 0]);
            drawnow;
            
            % 循环播放动画 (使用 plot 方法逐帧更新，并绘制已走过的轨迹)
            for ii = 1:numel(frame_idx)
                step_idx = frame_idx(ii);
                
                % 清除机器人axes并重新绘制
                axes(ax);  % 确保在正确的axes中绘制
                cla(ax);
                robot.plot(q_traj(step_idx,:), 'workspace', [-0.45 0.45 -0.45 0.45 -0.1 0.6], ...
                    'view', [45 30], 'noshadow', 'noname', 'scale', 0.8);
                
                hold(ax, 'on');
                % 重绘工作空间球体
                [sx, sy, sz] = sphere(20);
                sx = workspace_center(1) + workspace_radius * sx;
                sy = workspace_center(2) + workspace_radius * sy;
                sz = workspace_center(3) + workspace_radius * sz;
                surf(ax, sx, sy, sz, 'FaceAlpha', 0.03, 'EdgeColor', 'none', 'FaceColor', [0.2 0.6 1]);

                draw_workspace_info();
                
                % 绘制轨迹
                if step_idx > 1
                    plot3(ax, trajectory_points(1:step_idx, 1), trajectory_points(1:step_idx, 2), ...
                        trajectory_points(1:step_idx, 3), 'y-', 'LineWidth', 2.5);
                end
                plot3(ax, x_val, y_val, z_val, 'go', 'MarkerSize', 10, 'MarkerFaceColor', 'g');
                plot3(ax, trajectory_points(step_idx, 1), trajectory_points(step_idx, 2), ...
                    trajectory_points(step_idx, 3), 'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r');
                title(ax, '运动中...', 'FontSize', 12);
                grid(ax, 'on');
                hold(ax, 'off');
                
                % 更新6个运动分析图表
                joint_labels = {'J1','J2','J3','J4','J5','J6'};
                xyz_labels = {'X','Y','Z'};
                
                % 第1行：关节曲线（第一个图显示图例）
                cla(motion_axes{1, 1});
                plot(motion_axes{1, 1}, time_data(1:step_idx), q_traj(1:step_idx, :) * 180/pi, 'LineWidth', 1);
                title(motion_axes{1, 1}, '关节角度(°)', 'FontSize', 7);
                legend(motion_axes{1, 1}, joint_labels, 'Location', 'northwest', 'FontSize', 5);
                grid(motion_axes{1, 1}, 'on');
                
                cla(motion_axes{1, 2});
                plot(motion_axes{1, 2}, time_data(1:step_idx), qd_traj(1:step_idx, :) * 180/pi, 'LineWidth', 1);
                title(motion_axes{1, 2}, '关节角速度(°/s)', 'FontSize', 7);
                grid(motion_axes{1, 2}, 'on');
                
                cla(motion_axes{1, 3});
                plot(motion_axes{1, 3}, time_data(1:step_idx), qdd_traj(1:step_idx, :) * 180/pi, 'LineWidth', 1);
                title(motion_axes{1, 3}, '关节角加速度(°/s²)', 'FontSize', 7);
                grid(motion_axes{1, 3}, 'on');
                
                % 第2行：末端曲线（第一个图显示图例）
                cla(motion_axes{2, 1});
                plot(motion_axes{2, 1}, time_data(1:step_idx), trajectory_points(1:step_idx, :), 'LineWidth', 1);
                title(motion_axes{2, 1}, '末端位置(m)', 'FontSize', 7);
                legend(motion_axes{2, 1}, xyz_labels, 'Location', 'northeast', 'FontSize', 5);
                grid(motion_axes{2, 1}, 'on');
                
                cla(motion_axes{2, 2});
                plot(motion_axes{2, 2}, time_data(1:step_idx), trajectory_vel(1:step_idx, :), 'LineWidth', 1);
                title(motion_axes{2, 2}, '末端速度(m/s)', 'FontSize', 7);
                grid(motion_axes{2, 2}, 'on');
                
                cla(motion_axes{2, 3});
                plot(motion_axes{2, 3}, time_data(1:step_idx), trajectory_acc(1:step_idx, :), 'LineWidth', 1);
                title(motion_axes{2, 3}, '末端加速度(m/s²)', 'FontSize', 7);
                grid(motion_axes{2, 3}, 'on');
                
                drawnow limitrate;
                pause(pause_time);
            end
            
            % 同步更新滑动条位置
            for j = 1:6
                set(slider_joints{j}, 'Value', q_target(j)*180/pi);
                set(edit_joints{j}, 'String', sprintf('%.1f', q_target(j)*180/pi));
            end
            
            % 更新末端位置显示
            lbl_pos = findobj(fig, 'Tag', 'EndEffectorPos');
            set(lbl_pos, 'String', sprintf('末端: [%.3f, %.3f, %.3f]', x_val, y_val, z_val));
            
            % 更新当前位置
            current_q = q_target;
            set(lbl_status, 'String', sprintf('到达: [%.3f, %.3f, %.3f]', x_val, y_val, z_val), ...
                'ForegroundColor', 'black');

            move_completed = true;
            
            % 恢复标题
            title(ax, '松灵 Mycobot 280', 'FontSize', 11, 'FontWeight', 'bold');

            catch ME
            % 如果已经完成移动，则不要误报“不可达”
            if exist('move_completed', 'var') && move_completed
                % 保持到达状态，仅提示显示刷新异常（不打断结果）
                set(lbl_status, 'String', '到达（显示刷新异常）', 'ForegroundColor', 'black');
                return;
            end

            % 未完成移动：统一按不可达处理
            if exist('x_val', 'var') && exist('y_val', 'var') && exist('z_val', 'var')
                if ~isnan(x_val) && ~isnan(y_val) && ~isnan(z_val)
                    mark_unreachable(x_val, y_val, z_val, '不可达');
                end
            end
            set(lbl_status, 'String', '不可达：逆运动学求解失败', 'ForegroundColor', 'red');
            warning('IK:Failed', 'IK失败: %s', ME.message);
        end
    end

end

%% 辅助函数：使用蒙特卡洛法计算工作空间
function workspace_points = compute_workspace(robot, num_samples)
    % 输入:
    %   robot - SerialLink 机器人对象
    %   num_samples - 采样点数
    % 输出:
    %   workspace_points - N x 3 矩阵 (每行为 [x, y, z])
    
    % 获取关节限位
    qlim = robot.qlim;
    n_joints = robot.n;
    
    % 预分配空间
    workspace_points = zeros(num_samples, 3);
    
    % 蒙特卡洛采样
    for i = 1:num_samples
        % 在关节限位范围内随机生成关节角度
        q_random = zeros(1, n_joints);
        for j = 1:n_joints
            q_random(j) = qlim(j, 1) + (qlim(j, 2) - qlim(j, 1)) * rand();
        end
        
        % 计算正向运动学
        T = robot.fkine(q_random);
        pos = transl(T);
        
        % 存储位置 (过滤 Z < 0 的点，模拟桌面安装)
        if pos(3) >= 0
            workspace_points(i, :) = pos;
        end
    end
    
    % 移除全零行 (被过滤的点)
    workspace_points = workspace_points(any(workspace_points, 2), :);
end

%% 辅助函数：计算工作空间边界
function boundary_points = compute_workspace_boundary(workspace_points)
    % 输入:
    %   workspace_points - N x 3 点云矩阵
    % 输出:
    %   boundary_points - M x 3 边界点矩阵
    
    % 方法：在不同高度切片，计算每层的2D凸包，然后连接
    z_min = min(workspace_points(:,3));
    z_max = max(workspace_points(:,3));
    
    % 分层数量
    num_layers = 20;
    z_layers = linspace(z_min, z_max, num_layers);
    
    boundary_points = [];
    
    for i = 1:num_layers
        z_current = z_layers(i);
        tolerance = (z_max - z_min) / (2 * num_layers); % 层厚度
        
        % 提取当前高度附近的点
        layer_mask = abs(workspace_points(:,3) - z_current) < tolerance;
        layer_points = workspace_points(layer_mask, :);
        
        if size(layer_points, 1) >= 3
            % 计算XY平面的2D凸包
            try
                k = convhull(layer_points(:,1), layer_points(:,2));
                % 添加到边界点集
                hull_points = [layer_points(k, 1:2), repmat(z_current, length(k), 1)];
                boundary_points = [boundary_points; hull_points]; %#ok<AGROW>
            catch
                % 如果凸包计算失败（点太少），跳过
                continue;
            end
        end
    end
    
    % 如果没有计算出边界，返回空矩阵
    if isempty(boundary_points)
        boundary_points = zeros(0, 3);
    end
end
