`timescale 1ns / 1ps

module blit_drawline_tb;

  // Parameters

  //Ports
  reg clock;
  reg reset;
  reg stall;
  reg signed [15:0] x1;
  reg signed [15:0] y1;
  reg signed [15:0] x2;
  reg signed [15:0] y2;
  reg start;
  wire [15:0] x;
  wire [15:0] y;
  wire write;
  wire done;

  blit_drawline  blit_drawline_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall),
    .x1(x1),
    .y1(y1),
    .x2(x2),
    .y2(y2),
    .start(start),
    .x(x),
    .y(y),
    .write(write),
    .done(done)
  );

always begin
    clock = 0;
    #5; 
    clock = 1;
    #5;
end

integer i;

initial begin
    $dumpvars(5);
    reset = 1;
    stall = 0;
    start = 0;
    #10;
    reset = 0;
    x1 = 10;
    y1 = 20;
    x2 = 30;
    y2 = 48;
    #10;
    start = 1;
    #10;
    for (i=0; i<100; i=i+1) begin
        if (done)
            start = 0;
        #10;
    end 
    #10;
    $finish();
end

always @(posedge clock) begin
    if (write)
        $display("x=%d, y=%d", x, y);
end

endmodule