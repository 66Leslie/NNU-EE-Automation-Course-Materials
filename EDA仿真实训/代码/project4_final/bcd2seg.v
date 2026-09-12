// bcd_to_7seg.v
// BCD to 7-segment decoder (common cathode)
// seg_out[6:0] = {g, f, e, d, c, b, a}
// Or, a more standard mapping: seg_out[6:0] = {a, b, c, d, e, f, g}
// Let's use the latter for clarity: seg_out[0]=a, seg_out[1]=b, ..., seg_out[6]=g

module bcd2seg (
    input  [3:0] bcd_in,    // 4-bit BCD input (0-9)
    output reg [6:0] seg_out    // 7-segment display output {a,b,c,d,e,f,g}
);

    always @(*) begin
        case (bcd_in)
            4'b0000: seg_out = 7'b0111111; // 0
            4'b0001: seg_out = 7'b0000110; // 1
            4'b0010: seg_out = 7'b1011011; // 2
            4'b0011: seg_out = 7'b1001111; // 3
            4'b0100: seg_out = 7'b1100110; // 4
            4'b0101: seg_out = 7'b1101101; // 5
            4'b0110: seg_out = 7'b1111101; // 6
            4'b0111: seg_out = 7'b0000111; // 7
            4'b1000: seg_out = 7'b1111111; // 8
            4'b1001: seg_out = 7'b1101111; // 9
            default: seg_out = 7'b0000000; // Off for invalid BCD inputs (10-15)
        endcase
    end

endmodule
