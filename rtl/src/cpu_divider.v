`timescale 1ns/1ns

// This block performs an unsigned division. 
// For signed division the inputs need to be wrapped in abs()
// and logic to invert sign of output

module cpu_divider(
    input             clock,
    input             start,            // pulsed to start division
    input [31:0]      numerator,
    input [31:0]      denominator,
    input             sign,             // Sign of result
    output reg [31:0] quotient,
    output reg [31:0] remainder,
    output reg        done
);

reg [4:0]  this_count=31, next_count=31;
reg [31:0] this_quotient, next_quotient;
reg [31:0] this_remainder, next_remainder;
reg signed [31:0] n,s;

always @(*) begin
    n = {this_remainder[30:0], numerator[this_count]};
    s = n - denominator;

    if (!done && (start || this_count!=31)) begin
        next_count = this_count - 1'b1;
        if (s>=0) begin
            next_remainder = s;
            next_quotient = {this_quotient[30:0], 1'b1};
        end else begin
            next_remainder = n;
            next_quotient = {this_quotient[30:0], 1'b0};
        end
    end else begin
        next_count = 31;
        next_quotient = 0;
        next_remainder = 0;
    end
end

always @(posedge clock) begin
    this_count     <= next_count;
    this_quotient  <= next_quotient;
    this_remainder <= next_remainder;
    done           <= (this_count==0);

    quotient       <= (denominator==0) ? 32'hffffffff 
                    : sign             ? -next_quotient
                    :                    next_quotient;
    remainder      <= (denominator==0) ? numerator
                    : sign             ? -next_remainder
                    :                    next_remainder;
end

endmodule