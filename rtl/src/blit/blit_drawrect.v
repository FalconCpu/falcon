`timescale 1ns / 1ns

module blit_drawrect (
    input               clock,
    input               reset,
    input               stall,
   
    input               start,
    input               reversed,
    input [15:0]        width,
    input [15:0]        height,
    input [15:0]        p1_x1,
    input [15:0]        p1_y1,
    input [15:0]        p1_x2,
    input [15:0]        p1_y2,

    output reg [15:0]   p2_rect_dest_x,
    output reg [15:0]   p2_rect_dest_y,
    output reg [15:0]   p2_rect_src_x,
    output reg [15:0]   p2_rect_src_y,
    output reg          done
);

// local reegisters
reg [15:0]      x, next_x;
reg [15:0]      y, next_y;
reg [15:0]      next_p2_rect_dest_x;
reg [15:0]      next_p2_rect_dest_y;
reg [15:0]      next_p2_rect_src_x;
reg [15:0]      next_p2_rect_src_y;

always @(*) begin
    if (start==0) begin
        next_x = 16'h0;
        next_y = 16'h0;
    end else begin
        next_x = next_x + 16'h1;
        if (next_x == width) begin
            next_x = 16'h0;
            next_y = next_y + 16'h1;
            if (next_y == height) begin
                next_y = 16'h0;
                done = 1'b1;
            end
        end
    end

    if (!reversed) begin
        next_p2_rect_dest_x = p1_x1 + x;
        next_p2_rect_dest_y = p1_y1 + y;
        next_p2_rect_src_x  = p1_x2 + x;
        next_p2_rect_src_y  = p1_y2 + y;
    end else begin
        next_p2_rect_dest_x = p1_x1 - x;
        next_p2_rect_dest_y = p1_y1 - y;
        next_p2_rect_src_x  = p1_x2 - x;
        next_p2_rect_src_y  = p1_y2 - y;
    end
end

always @(posedge clock) begin
    if (!stall) begin
        x <= next_x;
        y <= next_y;
        p2_rect_dest_x <= next_p2_rect_dest_x;
        p2_rect_dest_y <= next_p2_rect_dest_y;
        p2_rect_src_x  <= next_p2_rect_src_x;
        p2_rect_src_y  <= next_p2_rect_src_y;
    end
end
endmodule