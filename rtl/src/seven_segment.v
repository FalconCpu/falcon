`timescale 1ns/1ns

module seven_segment(
    input               clock,
    input       [23:0]  seven_segment,
    input       [7:0]   seven_segment_brightness,
	output	reg [6:0]	HEX0,
	output	reg [6:0]	HEX1,
	output	reg [6:0]	HEX2,
	output	reg [6:0]	HEX3,
	output	reg [6:0]	HEX4,
	output	reg [6:0]	HEX5
);

reg [7:0] counter=0;

always @(posedge clock) begin

    counter = counter + 1;

    case (seven_segment[3:0]) 
        4'h0:    HEX0 = 7'b1000000;  // digit 0
        4'h1:    HEX0 = 7'b1111001;  // digit 1
        4'h2:    HEX0 = 7'b0100100;  // digit 2
        4'h3:    HEX0 = 7'b0110000;  // digit 3
        4'h4:    HEX0 = 7'b0011001;  // digit 4
        4'h5:    HEX0 = 7'b0010010;  // digit 5
        4'h6:    HEX0 = 7'b0000010;  // digit 6
        4'h7:    HEX0 = 7'b1111000;  // digit 7
        4'h8:    HEX0 = 7'b0000000;  // digit 8
        4'h9:    HEX0 = 7'b0010000;  // digit 9
        4'ha:    HEX0 = 7'b0001000;  // digit A
        4'hb:    HEX0 = 7'b0000011;  // digit B
        4'hc:    HEX0 = 7'b1000110;  // digit C
        4'hd:    HEX0 = 7'b0100001;  // digit D
        4'he:    HEX0 = 7'b0000110;  // digit E
        default: HEX0 = 7'b0001110;  // digit F
    endcase

    case (seven_segment[7:4]) 
        4'h0:    HEX1 = 7'b1000000;  // digit 0
        4'h1:    HEX1 = 7'b1111001;  // digit 1
        4'h2:    HEX1 = 7'b0100100;  // digit 2
        4'h3:    HEX1 = 7'b0110000;  // digit 3
        4'h4:    HEX1 = 7'b0011001;  // digit 4
        4'h5:    HEX1 = 7'b0010010;  // digit 5
        4'h6:    HEX1 = 7'b0000010;  // digit 6
        4'h7:    HEX1 = 7'b1111000;  // digit 7
        4'h8:    HEX1 = 7'b0000000;  // digit 8
        4'h9:    HEX1 = 7'b0010000;  // digit 9
        4'ha:    HEX1 = 7'b0001000;  // digit A
        4'hb:    HEX1 = 7'b0000011;  // digit B
        4'hc:    HEX1 = 7'b1000110;  // digit C
        4'hd:    HEX1 = 7'b0100001;  // digit D
        4'he:    HEX1 = 7'b0000110;  // digit E
        default: HEX1 = 7'b0001110;  // digit F
    endcase

    case (seven_segment[11:8]) 
        4'h0:    HEX2 = 7'b1000000;  // digit 0
        4'h1:    HEX2 = 7'b1111001;  // digit 1
        4'h2:    HEX2 = 7'b0100100;  // digit 2
        4'h3:    HEX2 = 7'b0110000;  // digit 3
        4'h4:    HEX2 = 7'b0011001;  // digit 4
        4'h5:    HEX2 = 7'b0010010;  // digit 5
        4'h6:    HEX2 = 7'b0000010;  // digit 6
        4'h7:    HEX2 = 7'b1111000;  // digit 7
        4'h8:    HEX2 = 7'b0000000;  // digit 8
        4'h9:    HEX2 = 7'b0010000;  // digit 9
        4'ha:    HEX2 = 7'b0001000;  // digit A
        4'hb:    HEX2 = 7'b0000011;  // digit B
        4'hc:    HEX2 = 7'b1000110;  // digit C
        4'hd:    HEX2 = 7'b0100001;  // digit D
        4'he:    HEX2 = 7'b0000110;  // digit E
        default: HEX2 = 7'b0001110;  // digit F
    endcase

    case (seven_segment[15:12]) 
        4'h0:    HEX3 = 7'b1000000;  // digit 0
        4'h1:    HEX3 = 7'b1111001;  // digit 1
        4'h2:    HEX3 = 7'b0100100;  // digit 2
        4'h3:    HEX3 = 7'b0110000;  // digit 3
        4'h4:    HEX3 = 7'b0011001;  // digit 4
        4'h5:    HEX3 = 7'b0010010;  // digit 5
        4'h6:    HEX3 = 7'b0000010;  // digit 6
        4'h7:    HEX3 = 7'b1111000;  // digit 7
        4'h8:    HEX3 = 7'b0000000;  // digit 8
        4'h9:    HEX3 = 7'b0010000;  // digit 9
        4'ha:    HEX3 = 7'b0001000;  // digit A
        4'hb:    HEX3 = 7'b0000011;  // digit B
        4'hc:    HEX3 = 7'b1000110;  // digit C
        4'hd:    HEX3 = 7'b0100001;  // digit D
        4'he:    HEX3 = 7'b0000110;  // digit E
        default: HEX3 = 7'b0001110;  // digit F
    endcase

    case (seven_segment[19:16]) 
        4'h0:    HEX4 = 7'b1000000;  // digit 0
        4'h1:    HEX4 = 7'b1111001;  // digit 1
        4'h2:    HEX4 = 7'b0100100;  // digit 2
        4'h3:    HEX4 = 7'b0110000;  // digit 3
        4'h4:    HEX4 = 7'b0011001;  // digit 4
        4'h5:    HEX4 = 7'b0010010;  // digit 5
        4'h6:    HEX4 = 7'b0000010;  // digit 6
        4'h7:    HEX4 = 7'b1111000;  // digit 7
        4'h8:    HEX4 = 7'b0000000;  // digit 8
        4'h9:    HEX4 = 7'b0010000;  // digit 9
        4'ha:    HEX4 = 7'b0001000;  // digit A
        4'hb:    HEX4 = 7'b0000011;  // digit B
        4'hc:    HEX4 = 7'b1000110;  // digit C
        4'hd:    HEX4 = 7'b0100001;  // digit D
        4'he:    HEX4 = 7'b0000110;  // digit E
        default: HEX4 = 7'b0001110;  // digit F
    endcase

    case (seven_segment[23:20]) 
        4'h0:    HEX5 = 7'b1000000;  // digit 0
        4'h1:    HEX5 = 7'b1111001;  // digit 1
        4'h2:    HEX5 = 7'b0100100;  // digit 2
        4'h3:    HEX5 = 7'b0110000;  // digit 3
        4'h4:    HEX5 = 7'b0011001;  // digit 4
        4'h5:    HEX5 = 7'b0010010;  // digit 5
        4'h6:    HEX5 = 7'b0000010;  // digit 6
        4'h7:    HEX5 = 7'b1111000;  // digit 7
        4'h8:    HEX5 = 7'b0000000;  // digit 8
        4'h9:    HEX5 = 7'b0010000;  // digit 9
        4'ha:    HEX5 = 7'b0001000;  // digit A
        4'hb:    HEX5 = 7'b0000011;  // digit B
        4'hc:    HEX5 = 7'b1000110;  // digit C
        4'hd:    HEX5 = 7'b0100001;  // digit D
        4'he:    HEX5 = 7'b0000110;  // digit E
        default: HEX5 = 7'b0001110;  // digit F
    endcase

    if (counter>seven_segment_brightness) begin
        HEX0 = 7'b1111111;
        HEX1 = 7'b1111111;
        HEX2 = 7'b1111111;
        HEX3 = 7'b1111111;
        HEX4 = 7'b1111111;
        HEX5 = 7'b1111111;
    end

end

endmodule