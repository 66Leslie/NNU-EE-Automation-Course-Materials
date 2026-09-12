module mod24(
    input CP,          // 时钟信号
    input nCR,         // 异步复位（低有效）
    input EN,          // 使能
    output reg [3:0] QH, // 十位（小时的十位，0~2）
    output reg [3:0] QL, // 个位（小时的个位，0~9）
    output reg Co         // 进位输出
);

	always @(posedge CP or negedge nCR)
	begin
		if (~nCR)
		begin
			QH <= 4'b0000;
			QL <= 4'b0000;
			Co <= 1'b0;
		end
		else if (~EN)
		begin
			QH <= QH;
			QL <= QL;
			Co <= Co;
		end
		else if ((QH > 2) || (QL > 9))
		begin
			QH <= 4'b0000;
			QL <= 4'b0000;
			Co <= 1'b0;
		end
		else if ((QH == 2) && (QL == 3)) // 计数到23，归零，进位
		begin
			QH <= 4'b0000;
			QL <= 4'b0000;
			Co <= 1'b1;
		end
		else if (QL == 9) // 个位计数到9，归零，十位加一
		begin
			QL <= 4'b0000;
			QH <= QH + 1'b1;
			Co <= 1'b0;
		end
		else
		begin
			QL <= QL + 1'b1;
			Co <= 1'b0;
		end
	end

endmodule
