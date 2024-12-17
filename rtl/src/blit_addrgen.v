`timescale 1ns / 1ns

module blit_addrgen(
    input clock,
    input stall,

    input [15:0] p2_rect_dest_x,
    input [15:0] p2_rect_dest_y,
    input [15:0] p2_rect_src_x,
    input [15:0] p2_rect_src_y,
    input [15:0] p2_line_x,
    input [15:0] p2_line_y,
    input        p2_run_line,
    input        p2_run_rect,
    input        p2_textmode,
    input [15:0] clip_x1,
    input [15:0] clip_y1,
    input [15:0] clip_x2,
    input [15:0] clip_y2,

    input [25:0] p2_src_addr,
    input [15:0] p2_src_bpr,
    input [25:0] p2_dest_addr,
    input [15:0] p2_dest_bpr,

    output  reg [25:0] p3_src_addr,
    output  reg [25:0] p3_dest_addr,
    output  reg [2:0]  p3_src_bit,
    output  reg        p3_write_en
);

wire [15:0] dest_x = p2_run_line ? p2_line_x : p2_rect_dest_x;
wire [15:0] dest_y = p2_run_line ? p2_line_y : p2_rect_dest_y;
wire [15:0] src_x  = p2_textmode ? p2_rect_src_x >>3 : p2_rect_src_x;

always @(posedge clock) begin
    if (!stall) begin
        p3_src_addr  <= p2_src_addr  + {10'b0,src_x}  + p2_rect_src_y*p2_src_bpr;
        p3_dest_addr <= p2_dest_addr + {10'b0,dest_x} +   dest_y*p2_dest_bpr;
        p3_src_bit   <= p2_rect_src_x[2:0];

        p3_write_en <= (p2_run_line || p2_run_rect) &&
                    (dest_x>=clip_x1) && (dest_x<clip_x2) &&
                    (dest_y>=clip_y1) && (dest_y<clip_y2);
    end
end

endmodule
