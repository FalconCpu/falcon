`timescale 1ns/1ps

// This module is only for simulation purposes. 
// For synthesys it is a MLAB memory 

module cpu_regfile (
    input           clock,
    input   [31:0]  data,
    input   [4:0]   rdaddress,
    input   [4:0]   wraddress,
    input           wren,
    output  [31:0]  q
);

reg [31:0] regs[0:31];
integer i;

integer file_handle;

initial begin
    for(i=0; i<=31; i=i+1)
        regs[i] = 0;

end

assign q = regs[rdaddress];

always @(posedge clock) 
    if (wren) 
        regs[wraddress] <= data;



endmodule
