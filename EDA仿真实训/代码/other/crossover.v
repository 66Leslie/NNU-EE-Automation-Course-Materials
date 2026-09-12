module divider_1hz(
    input clk,          // 50Hz input clock
    input rst_n,        // Asynchronous low active reset
    output reg clk_1hz_tick, // 1Hz single-cycle pulse output (renamed for clarity)
    output wire [5:0] counter_val // Renamed for clarity
);

    // To get a 1Hz tick from a 50Hz clock, we need to count 50 cycles.
    // Count from 0 to 49.
    localparam COUNT_MAX = 6'd49; // 50 cycles (0 to 49)
    reg [5:0] cnt;                // Counter

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            cnt <= 6'd0;
            clk_1hz_tick <= 1'b0;
        end else begin
            if (cnt == COUNT_MAX) begin
                cnt <= 6'd0;
                clk_1hz_tick <= 1'b1;  // Pulse high for ONE cycle when count reaches max
            end else begin
                cnt <= cnt + 6'd1;
                clk_1hz_tick <= 1'b0;  // Low otherwise
            end
        end
    end
    assign counter_val = cnt; // Output the internal counter value if needed for debug
endmodule
