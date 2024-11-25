`timescale 1ns/1ns

module seven_seg(
    input [23:0] seven_seg_data,
    output [6:0] HEX0,
    output [6:0] HEX1,
    output [6:0] HEX2,
    output [6:0] HEX3,
    output [6:0] HEX4,
    output [6:0] HEX5
);

// Convert 4-bit hex value to 7-segment display pattern (active low)
function [6:0] hex_to_seg(input [3:0] hex);
    case(hex)
        4'h0: hex_to_seg = 7'b1000000;
        4'h1: hex_to_seg = 7'b1111001;
        4'h2: hex_to_seg = 7'b0100100;
        4'h3: hex_to_seg = 7'b0110000;
        4'h4: hex_to_seg = 7'b0011001;
        4'h5: hex_to_seg = 7'b0010010;
        4'h6: hex_to_seg = 7'b0000010;
        4'h7: hex_to_seg = 7'b1111000;
        4'h8: hex_to_seg = 7'b0000000;
        4'h9: hex_to_seg = 7'b0010000;
        4'hA: hex_to_seg = 7'b0001000;
        4'hB: hex_to_seg = 7'b0000011;
        4'hC: hex_to_seg = 7'b1000110;
        4'hD: hex_to_seg = 7'b0100001;
        4'hE: hex_to_seg = 7'b0000110;
        4'hF: hex_to_seg = 7'b0001110;
    endcase
endfunction

assign HEX0 = hex_to_seg(seven_seg_data[3:0]);
assign HEX1 = hex_to_seg(seven_seg_data[7:4]);
assign HEX2 = hex_to_seg(seven_seg_data[11:8]);
assign HEX3 = hex_to_seg(seven_seg_data[15:12]);
assign HEX4 = hex_to_seg(seven_seg_data[19:16]);
assign HEX5 = hex_to_seg(seven_seg_data[23:20]);

endmodule
