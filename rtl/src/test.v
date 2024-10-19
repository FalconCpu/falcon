
module test(
    input      [7:0] a,
    input      [7:0] b,
    output     [7:0] y
);

genvar i;

generate
    for (i=0; i<8; i=i+1) begin
       assign y[i] = a[i] ^ b[i] ;
    end
endgenerate

endmodule
