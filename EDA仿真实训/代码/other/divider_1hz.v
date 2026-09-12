module divider_1hz (
    input clk,          // 50Hz input clock
    input rst_n,        // Asynchronous low-active reset
    output reg clk_1hz, // 1Hz tick (pulsed high for one clk cycle)
    output [5:0] counter
);

    reg [5:0] cnt;      // Counter for 50Hz clock (counts 0 to 49 for 1Hz)

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            cnt <= 6'd0;
            clk_1hz <= 1'b0;
        end else begin
            if (cnt == 6'd49) begin  // Counted 50 cycles (0 to 49)
                cnt <= 6'd0;
                clk_1hz <= 1'b1;   // Assert tick for one cycle
            end else begin
                cnt <= cnt + 6'd1;
                clk_1hz <= 1'b0;   // De-assert tick otherwise
            end
        end
    end
    assign counter = cnt;
    
endmodule