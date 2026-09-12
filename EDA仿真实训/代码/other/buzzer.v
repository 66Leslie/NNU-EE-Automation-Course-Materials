// buzzer.v
module buzzer (
// Core Inputs
input clk,
input rst_n,              // Asynchronous reset, active low

// Current time from clock module (BCD format)
input [3:0] hourH_bcd,    // Hour tens digit
input [3:0] hourL_bcd,    // Hour units digit
input [3:0] minH_bcd,     // Minute tens digit
input [3:0] minL_bcd,     // Minute units digit
input [3:0] secH_bcd,     // Second tens digit
input [3:0] secL_bcd,     // Second units digit

// Tick/Signal inputs
input Hz_1_tick,          // 1Hz tick (pulsed high for one clk cycle at the start of each second)
input Hz_5_tick,          // 5Hz square wave output from 5Hz audio module
input Hz_25_tick,         // 25Hz square wave output for the alarm

// Output
output reg buzzer_out      // To physical buzzer driver
);

// --- Parameters for Alarm Time (00:XY:00 where XY are the last two digits of student ID) ---
localparam ALARM_HOUR_H_TARGET   = 4'b0000; // Hour tens digit (0 for 00 hours)
localparam ALARM_HOUR_L_TARGET   = 4'b0000; // Hour units digit (0 for 00 hours)

// 您需要根据您学号的最后两位(XY)修改下面这两个参数
// 例如: 学号末两位是 "34", 则 ALARM_MINUTE_H_TARGET = 4'b0011 (3), ALARM_MINUTE_L_TARGET = 4'b0100 (4).
localparam ALARM_MINUTE_H_TARGET = 4'b0011; // << 修改这里 (分钟的十位数, 例如学号末两位XY, 这里是X的BCD码)
localparam ALARM_MINUTE_L_TARGET = 4'b0011; // << 修改这里 (分钟的个位数, 例如学号末两位XY, 这里是Y的BCD码)

localparam ALARM_SECOND_H_TARGET = 4'b0000; // Second tens digit (0 for 00 seconds)
localparam ALARM_SECOND_L_TARGET = 4'b0000; // Second units digit (0 for 00 seconds)


// --- Internal Wires and Registers for Timed Alarm ---
wire flag_alarm_time_match;     // 当当前时间与设定的闹钟时间匹配时为高
reg  alarm_active;              // 标志位: 指示闹钟是否处于激活状态 (1分钟)
reg  [5:0] alarm_seconds_counter; // 闹钟激活期间, 用于秒计数 (0-59秒)

// --- Combinational Logic for Time Condition ---
// 检查当前时间是否与配置的闹钟时间匹配
assign flag_alarm_time_match = (hourH_bcd == ALARM_HOUR_H_TARGET)   &&
                               (hourL_bcd == ALARM_HOUR_L_TARGET)   &&
                               (minH_bcd  == ALARM_MINUTE_H_TARGET) &&
                               (minL_bcd  == ALARM_MINUTE_L_TARGET) &&
                               (secH_bcd  == ALARM_SECOND_H_TARGET) &&
                               (secL_bcd  == ALARM_SECOND_L_TARGET);

// --- Sequential Logic for Alarm State Management ---
// 此逻辑块管理闹钟的激活和1分钟的持续时间
always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        alarm_active          <= 1'b0;
        alarm_seconds_counter <= 6'd0;
    end else begin
        if (flag_alarm_time_match && Hz_1_tick && !alarm_active) begin
            // 如果时间匹配, 发生1Hz跳变, 且闹钟当前未激活, 则触发闹钟
            alarm_active          <= 1'b1;
            alarm_seconds_counter <= 6'd0; // 为闹钟持续时间重置秒计数器
        end
        else if (alarm_active && Hz_1_tick) begin
            // 如果闹钟已激活, 每秒增加秒计数器
            if (alarm_seconds_counter == 6'd59) begin // 60秒 (0-59) 已经过去
                alarm_active          <= 1'b0;        // 停止闹钟
                alarm_seconds_counter <= 6'd0;        // 重置计数器 (良好习惯)
            end else begin
                alarm_seconds_counter <= alarm_seconds_counter + 6'd1;
            end
        end
        // 无需 'else' 分支: 如果闹钟未激活且未触发, 状态保持不变.
        //计数器状态由上述条件管理。
    end
end

// --- Buzzer Output Logic ---
// 此逻辑块确定蜂鸣器的声音输出.
// 当闹钟激活时, 它会在5Hz和25Hz音调之间交替, 每种音调持续1秒.
always @(*) begin //蜂鸣器输出的组合逻辑
    buzzer_out = 1'b0; // 默认: 蜂鸣器关闭

    if (alarm_active) begin
        // alarm_seconds_counter 的最低有效位 (alarm_seconds_counter[0]) 每秒翻转一次.
        // 用此来交替音调:
        // 闹钟持续的偶数秒 (0, 2, ..., 58秒): alarm_seconds_counter[0] 为 0 -> 输出 5Hz
        // 闹钟持续的奇数秒 (1, 3, ..., 59秒): alarm_seconds_counter[0] 为 1 -> 输出 25Hz
        if (alarm_seconds_counter[0] == 1'b0) begin // 对应闹钟声音间隔的第1、3...秒
            buzzer_out = Hz_5_tick; // 输出 5Hz 音调
        end else begin                                // 对应闹钟声音间隔的第2、4...秒
            buzzer_out = Hz_25_tick; // 输出 25Hz 音调
        end
    end
    // 如果 alarm_active 为 false, buzzer_out 保持为 1'b0 (关闭), 根据默认赋值.
end

endmodule