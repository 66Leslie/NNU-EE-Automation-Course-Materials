// mod60.v
// Synchronous BCD counter, counts from 00 to 59.
// Designed for use with a common system clock for CP.
// EN_normal enables counting on CP edges.
// Co_normal is asserted for one CP cycle when counting from 59 to 00.

module mod60(
    input CP,            // System clock
    input nCR,           // Asynchronous reset (active low)
    input EN_normal,     // Enable for normal counting (e.g., 1Hz tick or previous stage's carry)
    input adj_mode_sel,  // Level signal: High when this counter is selected for adjustment
    input adj_inc_pulse, // Synchronous pulse to CP to increment when adj_mode_sel is high
    output reg [3:0] QH, // Tens digit (0-5)
    output reg [3:0] QL, // Units digit (0-9)
    output reg Co_normal // Carry out for normal counting (asserted when 59 -> 00)
);

always @(posedge CP or negedge nCR) begin
    if (~nCR) begin
        QH <= 4'b0000;
        QL <= 4'b0000;
        Co_normal <= 1'b0;
    end
    else begin
        // Default Co_normal to 0, it will be set high only on normal rollover
        Co_normal <= 1'b0;

        if (adj_mode_sel && adj_inc_pulse) begin
            // Adjustment Mode: Increment current value (00-59 cycle)
            // Co_normal remains 0 during adjustment.
            if ((QH == 4'd5) && (QL == 4'd9)) begin // Current value is 59, rolls over to 00
                QL <= 4'b0000;
                QH <= 4'b0000;
            end
            else if (QL == 4'd9) begin // Current QL is 9 (e.g., 09, 19, ..., 49), increment QH
                QL <= 4'b0000;
                QH <= QH + 1'b1;
            end
            else begin // Increment QL, QH remains
                QL <= QL + 1'b1;
            end
        end
        // Normal Counting Mode:
        // Enabled by EN_normal AND not in adjustment mode.
        else if (EN_normal && !adj_mode_sel) begin
            // Invalid state check (e.g., if counter somehow gets > 59)
            // This is a safeguard; in normal operation with BCD logic, it shouldn't be strictly necessary
            // if inputs are always valid BCD, but good for robustness.
            if ((QH > 4'd5) || (QL > 4'd9)) begin
                QH <= 4'b0000;
                QL <= 4'b0000;
                // Co_normal remains 0 as it's an invalid state reset
            end
            // Normal increment logic
            else if ((QH == 4'd5) && (QL == 4'd9)) begin // Counting from 59 to 00
                QH <= 4'b0000;
                QL <= 4'b0000;
                Co_normal <= 1'b1; // Generate carry for next stage
            end
            else if (QL == 4'd9) begin // Units digit rolls over (e.g. 09 -> 10)
                QL <= 4'b0000;
                QH <= QH + 1'b1;
                // Co_normal remains 0
            end
            else begin // Increment units digit
                QL <= QL + 1'b1;
                // QH remains QH;
                // Co_normal remains 0
            end
        end
        // If not reset, not adjusting, and not (normal counting enabled OR adjusting this unit),
        // values of QH, QL hold due to registered nature. Co_normal holds its last assigned value (typically 0).
    end
end
endmodule