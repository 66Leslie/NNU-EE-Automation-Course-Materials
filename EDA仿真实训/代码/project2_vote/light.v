module light(
    input [3:0] num,      // 输入数字
    output reg [7:0] seg  // 输出段选
);
    always @(*) begin
        case(num)
            4'd0: seg = 8'b11000000; // 0
            4'd1: seg = 8'b11111001; // 1
            4'd2: seg = 8'b10100100; // 2
            4'd3: seg = 8'b10110000; // 3
            4'd4: seg = 8'b10011001; // 4
            4'd5: seg = 8'b10010010; // 5
            4'd6: seg = 8'b10000010; // 6
            4'd7: seg = 8'b11111000; // 7
            4'd8: seg = 8'b10000000; // 8
            4'd9: seg = 8'b10010000; // 9
            default: seg = 8'b11111111; 
        endcase
    end
endmodule
