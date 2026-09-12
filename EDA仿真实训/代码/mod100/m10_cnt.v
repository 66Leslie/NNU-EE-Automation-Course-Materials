module m10_cnt(
	input CE,CP,CR,
	output reg[3:0] Q,
	output reg CO
	);
	always @ (posedge CP, negedge CR)
		if(~CR) begin Q <= 4'b0000;CO <= 0;end
		else if(CE)
			if( Q >=4'b1001) 
			begin
				Q <= 4'b0000;
				CO<=1;
			end
			else 
			begin 
			Q <= Q + 4'b0001;
			CO <= 0;  
			end 
endmodule