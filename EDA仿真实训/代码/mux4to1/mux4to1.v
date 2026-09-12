module mux4to1(D, S, Y);
	input [3:0] D; //输入端口
	input [1:0] S; //输入端口
	output reg Y; //输出端口及变量数据类型
	
	always @(D, S) //电路功能描述
		if (S==2'b00) Y = D[0];
		else if (S==2'b01) Y = D[1];
		else if (S==2'b10) Y = D[2];
		else Y = D[3];
endmodule