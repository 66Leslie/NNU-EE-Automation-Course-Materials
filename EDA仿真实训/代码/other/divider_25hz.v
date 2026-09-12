module divider_25hz (
    input clk,         // 50Hz input clock
    input rst_n,       // Asynchronous low-active reset
    output reg clk_25hz, // 25Hz tick (pulsed high for one clk cycle)
    output [5:0] counter
);

    reg [5:0] cnt;     // Counter for 50Hz clock (counts 0 to 1 for 25Hz)

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            cnt <= 6'd0;
            clk_25hz <= 1'b0;
        end else begin
            // For a 50Hz input clock, to get a 25Hz output, we need to divide by 2.
            // So, the counter should count from 0 to 1 (2 cycles total).
            if (cnt == 6'd1) begin  // Counted 2 cycles (0 to 1)
                cnt <= 6'd0;
                clk_25hz <= 1'b1;   // Assert tick for one cycle
            end else begin
                cnt <= cnt + 6'd1;
                clk_25hz <= 1'b0;   // De-assert tick otherwise
            end
        end
    end
    assign counter = cnt;
    
endmodule