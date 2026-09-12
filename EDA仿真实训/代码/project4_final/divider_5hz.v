module divider_5hz(
    input clk,              // 50Hz输入时钟
    input rst_n,            // 异步低有效复位
    output reg clk_5hz,     // 5Hz输出时钟
    output wire [3:0]counter // 4位计数器，能计到9
);

    reg [3:0] cnt;          // 4位计数器，能计到9

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            cnt <= 4'd0;
            clk_5hz <= 1'b0;
        end else if (cnt == 4'd4) begin   // 计数到4，5个时钟翻转一次
            cnt <= 4'd0;
            clk_5hz <= ~clk_5hz;          // 翻转输出
        end else begin
            cnt <= cnt + 4'd1;
        end
    end

    assign counter = cnt;
endmodule
