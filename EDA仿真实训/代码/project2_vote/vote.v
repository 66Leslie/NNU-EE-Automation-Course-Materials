module vote(
	input [6:0] person,
	output reg pass, 
	output reg [2:0]num
);
	integer i;
	always @(*)begin
	num = 3'b000;
	for(i=0;i<7;i=i+1)
		begin
			if(person[i])num = num+3'd1;  
		end
	if(num>=4)pass = 1;
	else pass = 0;
	end
endmodule