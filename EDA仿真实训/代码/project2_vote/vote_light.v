module vote_light(
	input [6:0]person,
	output wire pass,
	output wire [7:0]seg
);
	wire [2:0]num;
	vote u1 (person,pass,num);
	light u2 (num,seg);
endmodule
