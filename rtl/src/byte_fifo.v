
`timescale 1ns/1ns

// A fifo to hold byte values
//
// On the write side pulse write_enable to store write_data into fifo
// attempts to write when the fifo is full will be ignored
//
// On the read side read_data always shows the data at the front of the fifo (lookahead operation)
// assert read_enable to inidicate the data has been consumed and move onto the next

module byte_fifo (
  input            clk,
  input            reset,
  input            write_enable,  // insert data into fifo
  input [7:0]      write_data,    // data to be added to fifo
  input            read_enable,   // read data has been taken - move onto next
  output reg [7:0] read_data,
  output [9:0]     slots_free,
  output           not_empty
);

// FIFO capacity (number of elements)
parameter FIFO_DEPTH = 1024;

// Memory to store FIFO data
reg [7:0] memory [0:FIFO_DEPTH-1];

// Pointers for write and read operations
reg [9:0] wptr, prev_wptr;
reg [9:0] rptr;


// Full flag logic
// Use a one cycle delayed version of the write pointer when signalling not empty
// to ensure the data has actually been written into the ram before anouncing it as 
// being availible to be read
assign slots_free = rptr - wptr - 1'b1;
assign not_empty  = rptr != prev_wptr;

always @(posedge clk) begin
    // Logic for write operation
    if (reset) begin
        wptr <= 0;
    end else if (write_enable && slots_free!=0) begin
        // Write data to memory if not full
        memory[wptr] <= write_data;
        wptr <= wptr + 1'b1;
    end

    // Logic for read operation
    read_data <= memory[rptr];
    if (reset) begin
        rptr      <= 0;
    end else if (read_enable && not_empty) begin
        // Read data from memory if not empty
        rptr      <= rptr + 1'b1;
    end      

    prev_wptr <= wptr;
end
endmodule
