// mod12.v
// Synchronous BCD counter, counts from 00 to 11 (for 12-hour format).
// Note: Typical 12-hour clocks display 12, 01, 02, ..., 11.
// This counter goes 00, 01, ..., 09, 10, 11, then back to 00.
// If a 12 o'clock display is needed instead of 00, further logic would be
// required at the display stage or by modifying the counter's reset/target values.
// For standard BCD counting to trigger an alarm at "00:XY:00" as per your original request,
// 00 for hour is appropriate.

// Designed for use with a common system clock for CP.
// EN_normal enables counting on CP edges (typically driven by minutes' carry).
// Co_normal is asserted for one CP cycle when counting from 11 to 00.

module mod12(
    input CP,            // System clock
    input nCR,           // Asynchronous reset (active low)
    input EN_normal,     // Enable for normal counting (e.g., from minutes' carry)
    input adj_mode_sel,  // Level signal: High when this counter is selected for adjustment
    input adj_inc_pulse, // Synchronous pulse to CP to increment when adj_mode_sel is high
    output reg [3:0] QH, // Tens digit (0-1)
    output reg [3:0] QL, // Units digit (0-9 for QH=0, 0-1 for QH=1)
    output reg Co_normal // Carry out for normal counting (asserted when 11 -> 00)
);

always @(posedge CP or negedge nCR) begin
    if (~nCR) begin
        QH <= 4'b0000; // Hours reset to 00
        QL <= 4'b0000;
        Co_normal <= 1'b0;
    end
    else begin
        // Default Co_normal to 0, it will be set high only on normal rollover
        Co_normal <= 1'b0;

        if (adj_mode_sel && adj_inc_pulse) begin
            // Adjustment Mode: Increment current value (00-11 cycle)
            // Co_normal remains 0 during adjustment.
            // Sequence: ..., 08, 09, 10, 11, 00, ...
            if ((QH == 4'd1) && (QL == 4'd1)) begin // Current value is 11, rolls over to 00
                QH <= 4'b0000;
                QL <= 4'b0000;
            end
            else if ((QH == 4'd0) && (QL == 4'd9)) begin // Current value is 09, rolls over to 10
                QH <= 4'd1; // QH becomes 1
                QL <= 4'b0000; // QL becomes 0
            end
            // Handles increments like 00->01, ..., 08->09; and 10->11
            else begin
                QL <= QL + 1'b1;
                // QH remains as is (e.g. if QL increments from 0 to 1 while QH is 1 for state 10->11)
            end
        end
        // Normal Counting Mode:
        // Enabled by EN_normal AND not in adjustment mode.
        else if (EN_normal && !adj_mode_sel) begin
            // Invalid state check for a 00-11 counter:
            // (QH=0, QL>9) OR (QH=1, QL>1) OR (QH>1)
            if ((QH == 4'd0 && QL > 4'd9) || (QH == 4'd1 && QL > 4'd1) || (QH > 4'd1)) begin
                 QH <= 4'b0000; // Reset to a known valid state 00
                 QL <= 4'b0000;
                 // Co_normal remains 0
            end
            // Normal increment logic
            else if ((QH == 4'd1) && (QL == 4'd1)) begin // Counting from 11 to 00
                QH <= 4'b0000;
                QL <= 4'b0000;
                Co_normal <= 1'b1; // Generate carry (e.g., for AM/PM toggle or half-day counter)
            end
            else if ((QH == 4'd0) && (QL == 4'd9)) begin // Counting from 09 to 10
                QH <= 4'd1;    // QH becomes 1
                QL <= 4'b0000; // QL becomes 0
                // Co_normal remains 0
            end
            else begin // Increment units digit (covers 00-08 and state 10)
                QL <= QL + 1'b1;
                // QH remains QH
                // Co_normal remains 0
            end
        end
    end
end
endmodule