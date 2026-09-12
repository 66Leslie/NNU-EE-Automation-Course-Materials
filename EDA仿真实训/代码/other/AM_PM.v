module AM_PM
(
    input clk,    // 时钟（这里你可以用Co信号当作时钟输入）
    input nCR,    // 异步复位（低有效）
    output reg Q  // 输出，高低电平切换
);

always @(posedge clk or negedge nCR) begin
    if (~nCR)
        Q <= 1'b0;       // 初始值为0
    else
        Q <= ~Q;         // 每来一个脉冲翻转一次
end

endmodule
