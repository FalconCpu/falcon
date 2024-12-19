`timescale 1ns / 1ns

module blit_drawline(
    input                  clock,
    input                  stall,
    input                  pause,

    input signed [15:0]    x1,
    input signed [15:0]    y1,
    input signed [15:0]    x2,
    input signed [15:0]    y2,
    input                  start,

    output reg [15:0]      x,
    output reg [15:0]      y,
    output reg             write_enable,
    output reg             done
);

// internal values used in calculationg the steps
reg signed [15:0] dx, dy;       // slope

// registers
reg               sx, next_sx;
reg               sy, next_sy;
reg               steep, next_steep;
reg               prev_start;
reg signed [15:0] error, next_error;
reg signed [15:0] num_diagonal, next_num_diagonal;
reg signed [15:0] minus_num_straight, next_minus_num_straight;
reg [15:0]        next_x, next_y;
reg               next_write_enable;

always @(*) begin
    done       = 1'b0;
    next_x     = 16'hx;
    next_y     = 16'hx;
    next_error = 16'hx;
    next_write_enable = 1'b0;

    // The control logic will always set up x1, y1, x2, y2 at least one cycle before the start
    // signal is asserted. This gives us time to calculate the slope before we need to draw.
    // ie this next block of logic will have already been executed by the time the start signal is asserted

    dx = x2 - x1;               // Calculate the distance to move in each direction
    dy = y2 - y1;

    next_sx = dx < 0 ;          // get the principal direction of motion in each direction
    next_sy = dy < 0 ;
    
    dx = next_sx ? -dx : dx;    // Calculate the absolute value of the deltas
    dy = next_sy ? -dy : dy;

    next_steep = dy > dx;       // Check if the line is steep - ie more Y movement than X
                        
    if (next_steep) begin       // Now calculate the number of diagonal steps and the number of straight steps
        next_num_diagonal = dx;
        next_minus_num_straight = dx-dy;
    end else begin
        next_num_diagonal = dy;
        next_minus_num_straight = dy-dx;
    end

    // Now for the main loop
    if (start==0) begin
        next_error = 16'hx;
        next_x     = 16'hx;
        next_y     = 16'hx;
    end else if (start==1 && prev_start==0) begin
        // First time through the loop
        next_error = {minus_num_straight[15],minus_num_straight[15:1]};
        next_x     = x1;
        next_y     = y1;
        next_write_enable = 1'b1;
    end else begin
        // Main body of the loop
        if (pause) begin
            next_error = error;
            next_x     = x;
            next_y     = y;
        end else if ((x==x2) && (y==y2)) begin
            done = 1'b1;
        end else begin
            if (steep || error>=0)
                next_y = y + (sy ? -16'd1 : 16'd1);
            else
                next_y = y;

            if (!steep || error>=0)
                next_x = x + (sx ? -16'd1 : 16'd1);
            else    
                next_x = x;

            if (error<0)
                next_error = error + num_diagonal;
            else
                next_error = error + minus_num_straight;
            next_write_enable = 1'b1;
        end
    end
end 

always @(posedge clock) begin
    if (!stall) begin
        sx           <= next_sx;
        sy           <= next_sy;
        steep        <= next_steep;
        error        <= next_error;
        num_diagonal <= next_num_diagonal;
        minus_num_straight <= next_minus_num_straight;
        x            <= next_x;
        y            <= next_y;
        prev_start   <= start;
        write_enable <= next_write_enable;
    end 
end

endmodule