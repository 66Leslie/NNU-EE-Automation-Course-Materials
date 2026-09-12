// buzzer.v
module buzzer (
    // Core Inputs
    input clk,
    input rst_n,             // Asynchronous reset, active low

    // Current time from clock module (BCD format)
    input [3:0] hourH_bcd,   // Hour tens digit
    input [3:0] hourL_bcd,   // Hour units digit
    input [3:0] minH_bcd,    // Minute tens digit
    input [3:0] minL_bcd,    // Minute units digit
    input [3:0] secH_bcd,    // Second tens digit
    input [3:0] secL_bcd,    // Second units digit

    // Tick/Signal inputs
    input Hz_1_tick,       // 1Hz tick (pulsed high for one clk cycle at the start of each second)
    input Hz_5_tick,        // 5Hz square wave output from 5Hz audio module
    input Hz_50_tick,       // 50Hz square wave output from 50Hz audio module

    // Output
    output reg buzzer_out    // To physical buzzer driver
);

    // --- Parameters for Hardcoded Alarm Time (09:33:00) ---
    localparam ALARM_HOUR_H_TARGET   = 4'b0000; // 0 (for 09 hours)
    localparam ALARM_HOUR_L_TARGET   = 4'b0001; //  (for 09 hours)
    localparam ALARM_MINUTE_H_TARGET = 4'b0011; // 3 (for 33 minutes)
    localparam ALARM_MINUTE_L_TARGET = 4'b0011; // 3 (for 33 minutes)
    localparam ALARM_SECOND_H_TARGET = 4'b0000; // 0 (for 00 seconds)
    localparam ALARM_SECOND_L_TARGET = 4'b0000; // 0 (for 00 seconds)


    // --- Internal Wires and Registers ---

    // For "整点报时" (Pre-Hour Chime)
    wire is_59th_minute;
    reg  pre_active; // Flag: a pre-chime sound is currently active
    reg  report;  // if 1 整点报时50hz，else 0，pre5hz
    // For "定时与闹钟" (Timed Alarm)
    wire flag_alarm_check; // Renamed for clarity
    reg  alarm_active;                  // Flag: alarm period (1 minute) is active
    reg  [5:0] alarm_counter;  // Counts 0-59 seconds for alarm duration

    // --- Combinational Logic for Time Conditions ---

    // Condition for "整点报时": Checks if it's the 59th minute of any hour
    assign is_59th_minute = (minH_bcd == 4'd5) && (minL_bcd == 4'd9);

    // Condition for "定时与闹钟": Checks if current time matches the HARDCODED alarm time (09:33:00)
    assign flag_alarm_check = (hourH_bcd == ALARM_HOUR_H_TARGET)   &&
                                           (hourL_bcd == ALARM_HOUR_L_TARGET)   &&
                                           (minH_bcd  == ALARM_MINUTE_H_TARGET) &&
                                           (minL_bcd  == ALARM_MINUTE_L_TARGET) &&
                                           (secH_bcd  == ALARM_SECOND_H_TARGET) &&
                                           (secL_bcd  == ALARM_SECOND_L_TARGET);

    // --- Sequential Logic for State Management ---

    // State logic for "整点报时" (Pre-Hour Chime)
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pre_active <= 1'b0;
            report  <= 1'b0;
        end else begin
            if (Hz_1_tick) begin
                pre_active <= 1'b0; // Default off unless triggered

                if (is_59th_minute) begin
                    if ((secH_bcd == 4'd5 && secL_bcd == 4'd1) ||
                        (secH_bcd == 4'd5 && secL_bcd == 4'd3) ||
                        (secH_bcd == 4'd5 && secL_bcd == 4'd5) ||
                        (secH_bcd == 4'd5 && secL_bcd == 4'd7)) begin
                        pre_active <= 1'b1;
                        report  <= 1'b1;
                    end
                    else if (secH_bcd == 4'd5 && secL_bcd == 4'd9) begin
                        pre_active <= 1'b1;
                        report  <= 1'b0;
                    end
                end
            end
        end
    end

    // State logic for "定时与闹钟" (Timed Alarm with hardcoded time)
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            alarm_active <= 1'b0;
            alarm_counter <= 6'd0;
        end else begin
            if (flag_alarm_check && Hz_1_tick && !alarm_active) begin
                alarm_active <= 1'b1;
                alarm_counter <= 6'd0;
            end
            else if (alarm_active && Hz_1_tick) begin
                if (alarm_counter == 6'd59) begin
                    alarm_active <= 1'b0;
                    alarm_counter <= 6'd0;
                end else begin
                    alarm_counter <= alarm_counter + 6'd1;
                end
            end
            else if (!alarm_active) begin
                 alarm_counter <= 6'd0;
            end
        end
    end

    // --- Buzzer Output Logic ---
    // Alarm takes precedence over pre-chime if conditions overlap.
    always @(*) begin
        buzzer_out = 1'b0; // Default: buzzer is off

        if (alarm_active) begin
            // Alarm sound: 5Hz and 50Hz alternate, each for 1 second
            if (alarm_counter[0] == 1'b0) begin // LSB for alternation
                buzzer_out = Hz_5_tick;
            end else begin
                buzzer_out = Hz_50_tick;
            end
        end
        else if (pre_active) begin
            // Pre-Hour Chime sound
            if (report) begin
                buzzer_out = Hz_5_tick;
            end else begin
                buzzer_out = Hz_50_tick;
            end
        end
    end

endmodule