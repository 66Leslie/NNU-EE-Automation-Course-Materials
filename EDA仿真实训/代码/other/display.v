module display(
    // Inputs from your clock counters (BCD format)
    input [3:0] hourH_bcd,  // Hours - tens digit
    input [3:0] hourL_bcd,  // Hours - units digit
    input [3:0] minH_bcd,   // Minutes - tens digit
    input [3:0] minL_bcd,   // Minutes - units digit
    input [3:0] secH_bcd,   // Seconds - tens digit
    input [3:0] secL_bcd,   // Seconds - units digit

    // Outputs to the 6 seven-segment displays
    output [6:0] seg_hourH,
    output [6:0] seg_hourL,
    output [6:0] seg_minH,
    output [6:0] seg_minL,
    output [6:0] seg_secH,
    output [6:0] seg_secL,
    output [41:0]seg
);

    // Instantiate 6 decoders

    // Hours display
    bcd2seg decoder_hourH (
        .bcd_in  (hourH_bcd),
        .seg_out (seg_hourH)
    );

    bcd2seg decoder_hourL (
        .bcd_in  (hourL_bcd),
        .seg_out (seg_hourL)
    );

    // Minutes display
    bcd2seg decoder_minH (
        .bcd_in  (minH_bcd),
        .seg_out (seg_minH)
    );

    bcd2seg decoder_minL (
        .bcd_in  (minL_bcd),
        .seg_out (seg_minL)
    );

    // Seconds display
    bcd2seg decoder_secH (
        .bcd_in  (secH_bcd),
        .seg_out (seg_secH)
    );

    bcd2seg decoder_secL (
        .bcd_in  (secL_bcd),
        .seg_out (seg_secL)
    );
	
	assign seg = {
		seg_hourH,
		seg_hourL,
		seg_minH,
		seg_minL,
		seg_secH,
		seg_secL
	};
endmodule
