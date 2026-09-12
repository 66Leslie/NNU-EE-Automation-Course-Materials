module mux2to1(D0, D1, S, Y );
	input D0, D1, S;
	output Y;
	reg Y ; //数据类型说明
	always @(S or D0 or D1)
		if (S == 1) Y = D1; //也可以写成if (S) Y = D1;
		else Y = D0; //过程赋值语句。注意=左边的Y必须是reg型。
endmodule