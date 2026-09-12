module door(
	input A,B,C,
	output  Y,Z
);
	wire a,b;
	assign Y = (B ~^ C) ~^ A;
	assign a = ~((~A)&(B~^C));
	assign b = ~(B&C); 
	assign Z = ~(a&b);

endmodule