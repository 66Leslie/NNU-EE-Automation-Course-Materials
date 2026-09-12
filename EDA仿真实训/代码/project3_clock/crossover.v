module crossover(
    input clk,          // 50Hz输入时钟
    input rst_n,        // 异步低有效复位
    output reg clk_1hz,  // 1Hz输出时钟
    output wire [5:0]counter
);

    reg [5:0] cnt;      // 6位计数器，能计到49

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            cnt <= 6'd0;
            clk_1hz <= 1'b0;
        end else if (cnt == 6'd24) begin
            cnt <= 6'd0;
            clk_1hz <= ~clk_1hz;  // 翻转输出
        end else begin
            cnt <= cnt + 6'd1;
        end
    end
    assign counter = cnt;
endmodule
