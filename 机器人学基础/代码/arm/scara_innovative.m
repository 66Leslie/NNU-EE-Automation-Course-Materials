function scara_innovative()
    % SCARA_INNOVATIVE 交互式机器人控制演示
    % 清除环境
    close all;
    clc;
    startup_rvc; % 确保工具箱已启动

    %% 1. 定义机器人模型
    % 使用独立变量定义 Link
    L1 = Revolute('d', 0.3, 'a', 0.2, 'alpha', 0, 'm', 2, 'r', [0.1, 0, 0], 'I', [0.1 0.1 0.1]);
    L2 = Revolute('d', 0,   'a', 0.3, 'alpha', pi, 'm', 1.5, 'r', [0.15, 0, 0], 'I', [0.1 0.1 0.1]);
    L3 = Prismatic('theta', 0, 'a', 0, 'alpha', 0, 'qlim', [0, 0.5], 'm', 1, 'r', [0, 0, 0.1], 'I', [0.1 0.1 0.1]);

    scara = SerialLink([L1 L2 L3], 'name', 'SCARA_Pro');
    
    % 初始关节角度
    current_q = [0 0 0];

    %% 2. 创建图形界面 (GUI)
    % 创建主窗口
    fig = figure('Name', 'SCARA 交互控制台', 'NumberTitle', 'off', ...
        'Color', [0.95 0.95 0.95], 'Position', [100, 100, 1000, 600]);

    % --- 左侧：机器人显示区域 ---
    ax = axes('Parent', fig, 'Position', [0.05, 0.1, 0.6, 0.8]);
    % 初始绘制机器人 (使用 'noshadow' 和 'noname' 选项以提高性能)
    % 工作空间范围：X[-0.8, 0.8], Y[-0.8, 0.8], Z[0, 1.0] (更大范围)
    scara.plot(current_q, 'workspace', [-0.8 0.8 -0.8 0.8 0 1.0], ...
        'view', [45 30], 'noshadow', 'noname');
    hold(ax, 'on'); % 允许在同一坐标轴上绘制多个对象
    
    % 绘制可达工作空间边界 (简化显示为圆柱体)
    % SCARA 的可达范围: 平面上半径 = a1 + a2 = 0.2 + 0.3 = 0.5
    % Z 轴范围: [d1 - qlim(3,max), d1] = [0.3-0.5, 0.3] = [-0.2, 0.3] (实际取正值)
    theta_workspace = linspace(0, 2*pi, 50);
    r_max = 0.5; % 最大半径 (两连杆伸直)
    r_min = abs(0.3 - 0.2); % 最小半径 (两连杆折叠)
    
    % 外圆 (最大可达范围)
    x_outer = r_max * cos(theta_workspace);
    y_outer = r_max * sin(theta_workspace);
    plot3(ax, x_outer, y_outer, zeros(size(x_outer)), 'b--', 'LineWidth', 1.5);
    plot3(ax, x_outer, y_outer, 0.3*ones(size(x_outer)), 'b--', 'LineWidth', 1.5);
    
    % 内圆 (最小不可达范围)
    x_inner = r_min * cos(theta_workspace);
    y_inner = r_min * sin(theta_workspace);
    plot3(ax, x_inner, y_inner, zeros(size(x_inner)), 'r--', 'LineWidth', 1.5);
    plot3(ax, x_inner, y_inner, 0.3*ones(size(x_inner)), 'r--', 'LineWidth', 1.5);
    
    % 添加图例说明
    legend(ax, {'', '最大可达边界 (外)', '', '最小不可达边界 (内)'}, 'Location', 'northeast', 'FontSize', 8);
    
    % 将标题添加到当前坐标轴
    title('机器人实时视图 (蓝色虚线=可达边界, 红色虚线=不可达区)', 'FontSize', 11, 'FontWeight', 'bold');
    hold(ax, 'off');

    % --- 右侧上方：正向运动学控制面板 ---
    panel_fk = uipanel('Parent', fig, 'Title', '正向运动学 (关节参数)', ...
        'Position', [0.7, 0.52, 0.25, 0.38], ...
        'FontSize', 10, 'BackgroundColor', 'white', ...
        'FontWeight', 'bold', 'ForegroundColor', [0.8 0.4 0]);
    
    % 关节 1 (旋转, 单位: 度)
    uicontrol('Parent', panel_fk, 'Style', 'text', 'Position', [10, 180, 80, 20], ...
        'String', '关节1 (deg):', 'BackgroundColor', 'white', 'HorizontalAlignment', 'left', 'FontSize', 9);
    slider_q1 = uicontrol('Parent', panel_fk, 'Style', 'slider', 'Position', [10, 160, 150, 20], ...
        'Min', -170, 'Max', 170, 'Value', 0, 'Callback', @update_fk);
    lbl_q1 = uicontrol('Parent', panel_fk, 'Style', 'text', 'Position', [165, 160, 50, 20], ...
        'String', '0°', 'BackgroundColor', [0.95 0.95 1], 'FontSize', 9);
    
    % 关节 2 (旋转, 单位: 度)
    uicontrol('Parent', panel_fk, 'Style', 'text', 'Position', [10, 130, 80, 20], ...
        'String', '关节2 (deg):', 'BackgroundColor', 'white', 'HorizontalAlignment', 'left', 'FontSize', 9);
    slider_q2 = uicontrol('Parent', panel_fk, 'Style', 'slider', 'Position', [10, 110, 150, 20], ...
        'Min', -170, 'Max', 170, 'Value', 0, 'Callback', @update_fk);
    lbl_q2 = uicontrol('Parent', panel_fk, 'Style', 'text', 'Position', [165, 110, 50, 20], ...
        'String', '0°', 'BackgroundColor', [0.95 0.95 1], 'FontSize', 9);
    
    % 关节 3 (移动, 单位: m)
    uicontrol('Parent', panel_fk, 'Style', 'text', 'Position', [10, 80, 80, 20], ...
        'String', '关节3 (m):', 'BackgroundColor', 'white', 'HorizontalAlignment', 'left', 'FontSize', 9);
    slider_q3 = uicontrol('Parent', panel_fk, 'Style', 'slider', 'Position', [10, 60, 150, 20], ...
        'Min', 0, 'Max', 0.5, 'Value', 0, 'Callback', @update_fk);
    lbl_q3 = uicontrol('Parent', panel_fk, 'Style', 'text', 'Position', [165, 60, 50, 20], ...
        'String', '0.00m', 'BackgroundColor', [0.95 0.95 1], 'FontSize', 9);
    
    % 末端位置显示
    uicontrol('Parent', panel_fk, 'Style', 'text', 'Position', [10, 30, 210, 20], ...
        'String', '末端位置: [0.50, 0.00, 0.30]', 'BackgroundColor', [1 1 0.9], ...
        'FontSize', 9, 'FontWeight', 'bold', 'HorizontalAlignment', 'center', 'Tag', 'EndEffectorPos');
    
    % --- 右侧下方：逆向运动学控制面板 ---
    panel = uipanel('Parent', fig, 'Title', '逆向运动学 (目标坐标)', ...
        'Position', [0.7, 0.12, 0.25, 0.38], ...
        'FontSize', 10, 'BackgroundColor', 'white', ...
        'FontWeight', 'bold', 'ForegroundColor', [0 0.4 0.8]);

    % 通用样式
    lblStyle = {'Parent', panel, 'Style', 'text', 'BackgroundColor', 'white', ...
        'HorizontalAlignment', 'right', 'FontSize', 10};
    editStyle = {'Parent', panel, 'Style', 'edit', 'BackgroundColor', [0.9 1 0.9], ...
        'HorizontalAlignment', 'center', 'FontSize', 10};

    % X 坐标输入
    uicontrol(lblStyle{:}, 'Position', [20, 180, 40, 25], 'String', 'X (m):');
    edt_x = uicontrol(editStyle{:}, 'Position', [70, 180, 100, 25], 'String', '0.4');

    % Y 坐标输入
    uicontrol(lblStyle{:}, 'Position', [20, 130, 40, 25], 'String', 'Y (m):');
    edt_y = uicontrol(editStyle{:}, 'Position', [70, 130, 100, 25], 'String', '0.1');

    % Z 坐标输入
    uicontrol(lblStyle{:}, 'Position', [20, 80, 40, 25], 'String', 'Z (m):');
    edt_z = uicontrol(editStyle{:}, 'Position', [70, 80, 100, 25], 'String', '0.2');

    % 移动按钮
    btn_move = uicontrol('Parent', panel, 'Style', 'pushbutton', 'String', '移动机械臂', ...
        'Position', [50, 20, 140, 40], ...
        'FontSize', 11, 'FontWeight', 'bold', ...
        'BackgroundColor', [0 0.5 0.8], 'ForegroundColor', 'white', ...
        'Callback', @move_robot_callback);

    % 状态显示
    lbl_status = uicontrol('Parent', fig, 'Style', 'text', ...
        'Position', [0.7, 0.2, 0.25, 0.05], ...
        'String', '就绪', 'BackgroundColor', [0.95 0.95 0.95], ...
        'ForegroundColor', [0.3 0.3 0.3], 'FontSize', 10);

    %% 3. 回调函数定义
    % 正向运动学更新函数
    function update_fk(~, ~)
        % 读取滑动条的值
        q1_deg = get(slider_q1, 'Value');
        q2_deg = get(slider_q2, 'Value');
        q3_m = get(slider_q3, 'Value');
        
        % 更新显示标签
        set(lbl_q1, 'String', sprintf('%.1f°', q1_deg));
        set(lbl_q2, 'String', sprintf('%.1f°', q2_deg));
        set(lbl_q3, 'String', sprintf('%.2fm', q3_m));
        
        % 转换为弧度 (关节1和2)
        q_new = [q1_deg*pi/180, q2_deg*pi/180, q3_m];
        
        % 计算正向运动学 (末端位置)
        T_end = scara.fkine(q_new);
        pos_end = transl(T_end); % 提取位置 [x, y, z]
        
        % 更新末端位置显示
        lbl_pos = findobj(fig, 'Tag', 'EndEffectorPos');
        set(lbl_pos, 'String', sprintf('末端位置: [%.2f, %.2f, %.2f]', pos_end(1), pos_end(2), pos_end(3)));
        
        % 更新机器人显示
        cla(ax);
        scara.plot(q_new, 'workspace', [-0.8 0.8 -0.8 0.8 0 1.0], ...
            'view', [45 30], 'noshadow', 'noname', 'noa', 'nowrist');
        
        % 重新绘制工作空间边界
        hold(ax, 'on');
        theta_ws = linspace(0, 2*pi, 50);
        plot3(ax, 0.5*cos(theta_ws), 0.5*sin(theta_ws), zeros(size(theta_ws)), 'b--', 'LineWidth', 1.5);
        plot3(ax, 0.5*cos(theta_ws), 0.5*sin(theta_ws), 0.3*ones(size(theta_ws)), 'b--', 'LineWidth', 1.5);
        plot3(ax, 0.1*cos(theta_ws), 0.1*sin(theta_ws), zeros(size(theta_ws)), 'r--', 'LineWidth', 1.5);
        plot3(ax, 0.1*cos(theta_ws), 0.1*sin(theta_ws), 0.3*ones(size(theta_ws)), 'r--', 'LineWidth', 1.5);
        
        % 标注末端位置 (青色点)
        plot3(ax, pos_end(1), pos_end(2), pos_end(3), 'co', 'MarkerSize', 12, 'MarkerFaceColor', 'c', 'LineWidth', 2);
        hold(ax, 'off');
        
        title('机器人实时视图 (蓝色=可达, 红色=不可达)', 'FontSize', 11, 'FontWeight', 'bold');
        
        % 更新当前关节角度
        current_q = q_new;
    end
    
    function move_robot_callback(~, ~)
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

            set(lbl_status, 'String', '正在规划路径...', 'ForegroundColor', 'blue');
            drawnow;

            % 构建目标位姿矩阵 (只关心位置，姿态设为默认)
            % SCARA 主要是平面定位+Z轴，末端姿态通常受限
            T_target = transl(x_val, y_val, z_val);

            % 检查目标点是否在可达工作空间内 (几何判断)
            r_target = sqrt(x_val^2 + y_val^2); % 目标点在 XY 平面的距离
            r_max = 0.5; % 最大可达半径
            r_min = 0.1; % 最小可达半径
            z_min = 0;
            z_max = 0.3; % Z 轴范围 (基座高度 d1=0.3)
            
            % 判断是否在可达空间内
            if r_target > r_max || r_target < r_min || z_val < z_min || z_val > z_max
                % 在图中标注不可达点 (红色 X)
                hold(ax, 'on');
                plot3(ax, x_val, y_val, z_val, 'rx', 'MarkerSize', 15, 'LineWidth', 3);
                text(ax, x_val, y_val, z_val + 0.05, '不可达!', 'Color', 'red', 'FontWeight', 'bold', 'FontSize', 10);
                hold(ax, 'off');
                
                set(lbl_status, 'String', sprintf('错误：超出工作空间 (r=%.2f, z=%.2f)', r_target, z_val), 'ForegroundColor', 'red');
                return;
            end
            
            % 逆运动学求解
            % mask=[1 1 1 0 0 0] 表示只匹配 x,y,z
            q_target = scara.ikine(T_target, 'mask', [1 1 1 0 0 0], 'q0', current_q);

            % 检查是否有解 (ikine 如果解算失败可能会返回空或警告)
            if isempty(q_target) || any(isnan(q_target))
                % 在图中标注无解点 (橙色 X)
                hold(ax, 'on');
                plot3(ax, x_val, y_val, z_val, 'mo', 'MarkerSize', 12, 'LineWidth', 2);
                text(ax, x_val, y_val, z_val + 0.05, '无解', 'Color', 'magenta', 'FontWeight', 'bold');
                hold(ax, 'off');
                
                set(lbl_status, 'String', '错误：逆运动学无解', 'ForegroundColor', 'red');
                return;
            end

            % 轨迹规划 (从当前位置平滑移动到目标位置)
            steps = 30;
            q_traj = jtraj(current_q, q_target, steps);
            
            % 计算轨迹中每个点的末端位置 (用于绘制轨迹)
            trajectory_points = zeros(steps, 3);
            for k = 1:steps
                T_temp = scara.fkine(q_traj(k,:));
                trajectory_points(k,:) = transl(T_temp);
            end

            % 动画执行
            set(lbl_status, 'String', '正在移动...', 'ForegroundColor', [0 0.5 0]);
            drawnow;
            
            % 循环播放动画 (使用 plot 方法逐帧更新，并绘制已走过的轨迹)
            for i = 1:steps
                % 清除当前轴并重新绘制
                cla(ax);
                scara.plot(q_traj(i,:), 'workspace', [-0.8 0.8 -0.8 0.8 0 1.0], ...
                    'view', [45 30], 'noshadow', 'noname', 'noa', 'nowrist');
                
                % 重新绘制工作空间边界
                hold(ax, 'on');
                theta_ws = linspace(0, 2*pi, 50);
                plot3(ax, 0.5*cos(theta_ws), 0.5*sin(theta_ws), zeros(size(theta_ws)), 'b--', 'LineWidth', 1.5);
                plot3(ax, 0.5*cos(theta_ws), 0.5*sin(theta_ws), 0.3*ones(size(theta_ws)), 'b--', 'LineWidth', 1.5);
                plot3(ax, 0.1*cos(theta_ws), 0.1*sin(theta_ws), zeros(size(theta_ws)), 'r--', 'LineWidth', 1.5);
                plot3(ax, 0.1*cos(theta_ws), 0.1*sin(theta_ws), 0.3*ones(size(theta_ws)), 'r--', 'LineWidth', 1.5);
                
                % 绘制已走过的轨迹 (黄色实线)
                if i > 1
                    plot3(ax, trajectory_points(1:i, 1), trajectory_points(1:i, 2), trajectory_points(1:i, 3), ...
                        'y-', 'LineWidth', 2.5);
                end
                
                % 标注目标点 (绿色点)
                plot3(ax, x_val, y_val, z_val, 'go', 'MarkerSize', 10, 'MarkerFaceColor', 'g');
                
                % 标注当前末端位置 (红色点)
                plot3(ax, trajectory_points(i, 1), trajectory_points(i, 2), trajectory_points(i, 3), ...
                    'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r');
                hold(ax, 'off');
                
                title('机器人实时视图 (黄色=运动轨迹)', 'FontSize', 11, 'FontWeight', 'bold');
                drawnow;
                pause(0.02); % 短暂暂停以便观察动画
            end
            
            % 同步更新滑动条位置
            set(slider_q1, 'Value', q_target(1)*180/pi);
            set(slider_q2, 'Value', q_target(2)*180/pi);
            set(slider_q3, 'Value', q_target(3));
            set(lbl_q1, 'String', sprintf('%.1f°', q_target(1)*180/pi));
            set(lbl_q2, 'String', sprintf('%.1f°', q_target(2)*180/pi));
            set(lbl_q3, 'String', sprintf('%.2fm', q_target(3)));
            
            % 更新当前位置
            current_q = q_target;
            set(lbl_status, 'String', sprintf('到达: [%.2f, %.2f, %.2f]', x_val, y_val, z_val), ...
                'ForegroundColor', 'black');

        catch ME
            set(lbl_status, 'String', ['错误: ' ME.message], 'ForegroundColor', 'red');
        end
    end

end