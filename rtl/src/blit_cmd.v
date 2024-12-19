`timescale 1ns / 1ps

module blit_cmd(
    input clock,
    input reset,
    input stall,

    input [103:0] p0_cmd,
    input p0_cmd_valid,
    output reg cmd_next,

    output reg [15:0] p1_x1,
    output reg [15:0] p1_y1,
    output reg [15:0] p1_x2,
    output reg [15:0] p1_y2,
    output reg [15:0] p1_width,
    output reg [15:0] p1_height,
    output reg [7:0]  p1_char,
    output reg        p1_run_line,
    output reg        p1_run_rect,
    output reg        p1_reversed,
    output reg        p1_textmode,
    output reg        p2_textmode,
    output reg        p2_mem_read,
    output reg [7:0]  p1_font_bpc,
    output reg [15:0] p2_clip_x1,
    output reg [15:0] p2_clip_y1,
    output reg [15:0] p2_clip_x2,
    output reg [15:0] p2_clip_y2,
    output reg [31:0] p1_src_addr,
    output reg [15:0] p2_src_bpr,
    output reg [25:0] p2_dest_addr,
    output reg [15:0] p2_dest_bpr,
    output reg [8:0]  p4_fg_color,
    output reg [8:0]  p4_bg_color,
    output reg [8:0]  p4_trans_color,
    output reg        p4_textmode,
    output reg        p5_active,

    input             line_done,
    input             rect_done
);

// ============================================
//               Blitter commands
// ============================================
parameter BLIT_SET_DEST_ADDR      = 8'h1; 
parameter BLIT_SET_SRC_ADDR       = 8'h2; 
parameter BLIT_FILL_RECT          = 8'h3; 
parameter BLIT_COPY_RECT          = 8'h4; 
parameter BLIT_COPY_RECT_REVERSED = 8'h5; 
parameter BLIT_SET_CLIP_RECT      = 8'h6; 
parameter BLIT_SET_TRANS_COLOR    = 8'h7; 
parameter BLIT_SET_FONT           = 8'h8;
parameter BLIT_DRAW_CHAR          = 8'h9;
parameter BLIT_DRAW_LINE          = 8'hA;

// ============================================
//               Registers
// ============================================
reg [25:0]      dest_addr, next_dest_addr;   // start address of the destination buffer
reg [15:0]      dest_bpr, next_dest_bpr;     // bytes per row
reg [31:0]      src_addr, next_src_addr;     // start address of the source
reg [15:0]      src_bpr, next_src_bpr;       // bytes per row
reg [15:0]      clip_x1, next_clip_x1;
reg [15:0]      clip_y1, next_clip_y1;
reg [15:0]      clip_x2, next_clip_x2;
reg [15:0]      clip_y2, next_clip_y2;
reg [8:0]       blit_trans_color, next_blit_trans_color;
reg [31:0]      font_addr, next_font_addr;
reg [4:0]       font_width, next_font_width;
reg [4:0]       font_height, next_font_height;
reg [4:0]       font_bpr, next_font_bpr;     // Font bytes per row
reg [7:0]       font_bpc, next_font_bpc;     // Font bytes per character

// ============================================
//               Pipeline registers
// ============================================

reg        first_cycle;      // set for the first cycle of a command
reg [15:0] p0_x1;
reg [15:0] p0_y1;
reg [15:0] p0_x2;
reg [15:0] p0_y2;
reg [15:0] p0_width;
reg [15:0] p0_height;
reg [7:0]  p0_char;
reg        p0_run_line;
reg        p0_run_rect;
reg        p0_reversed;
reg        p0_textmode;
reg        p3_textmode;
reg        p0_mem_read;
reg        p1_mem_read;
reg [15:0] p1_src_bpr;
reg [25:0] p1_dest_addr;
reg [15:0] p1_dest_bpr;
reg [8:0]  p0_fg_color,p1_fg_color,p2_fg_color,p3_fg_color;
reg [8:0]  p0_bg_color,p1_bg_color,p2_bg_color,p3_bg_color;
reg [8:0]  p1_trans_color;
reg [8:0]  p2_trans_color;
reg [8:0]  p3_trans_color;
reg        p2_active;
reg        p3_active;
reg        p4_active;






reg [7:0]  cmd;




// ============================================
//               Combinatorial logic
// ============================================

always @(*) begin
    next_dest_addr   = dest_addr;   
    next_dest_bpr    = dest_bpr;
    next_src_addr    = src_addr;
    next_src_bpr     = src_bpr;
    next_clip_x1     = clip_x1;
    next_clip_y1     = clip_y1;
    next_clip_x2     = clip_x2;
    next_clip_y2     = clip_y2;
    next_blit_trans_color = blit_trans_color;
    next_font_addr   = font_addr;
    next_font_width  = font_width;
    next_font_height = font_height;
    next_font_bpr    = font_bpr;
    next_font_bpc    = font_bpc;
    cmd_next = 1'b0;

    p0_width      = p0_cmd[15:0];
    p0_height     = p0_cmd[31:16];
    p0_x1         = p0_cmd[47:32];
    p0_y1         = p0_cmd[63:48];
    p0_x2         = p0_cmd[79:64];
    p0_y2         = p0_cmd[95:80];
    cmd           = p0_cmd[103:96];

    p0_fg_color   = 9'hx;
    p0_bg_color   = 9'hx;
    p0_run_line   = 1'b0;
    p0_run_rect   = 1'b0;
    p0_reversed   = 1'b0;
    p0_textmode   = 1'b0;
    p0_mem_read   = 1'b0;
    p0_char       = 8'h0;


    if (! p0_cmd_valid) begin
        cmd_next = 1'b0;
    
    end else begin
        case (cmd)

        BLIT_SET_DEST_ADDR: begin
            next_dest_addr = p0_cmd[25:0];
            next_dest_bpr = p0_x1;
            cmd_next = 1'b1;
        end

        BLIT_SET_SRC_ADDR: begin
            next_src_addr = p0_cmd[31:0];
            next_src_bpr = p0_x1;
            cmd_next = 1'b1;
        end

        BLIT_SET_CLIP_RECT: begin
            next_clip_x1 = p0_width;
            next_clip_y1 = p0_height;
            next_clip_x2 = p0_x1;
            next_clip_y2 = p0_y1;
            cmd_next = 1'b1;
        end

        BLIT_SET_FONT: begin
            next_font_addr = p0_cmd[31:0];
            next_font_width = p0_x1[4:0];
            next_font_height = p0_y1[4:0];
            next_font_bpr = p0_x2[4:0];
            next_font_bpc = p0_y2[4:0];
            cmd_next = 1'b1;
        end 

        BLIT_SET_TRANS_COLOR: begin
            next_blit_trans_color = p0_cmd[8:0];
            cmd_next = 1'b1;
        end 

        BLIT_DRAW_LINE: begin
            p0_run_line = ! first_cycle;
            p0_x2       = p0_cmd[15:0];
            p0_y2       = p0_cmd[31:16];        
            p0_fg_color = p0_cmd[72:64];
            cmd_next = line_done;            
        end

        BLIT_DRAW_CHAR: begin
            p0_run_rect = ! first_cycle;
            p0_textmode = 1'b1;
            p0_fg_color = p0_cmd[72:64];
            p0_bg_color = p0_cmd[88:80];
            p0_mem_read = 1'b1;
            p0_width    = font_width;
            p0_height   = font_height;
            p0_char     = p0_cmd[7:0];
            p0_x2       = 0;
            p0_y2       = 0;
            cmd_next    = rect_done;
        end

        BLIT_COPY_RECT_REVERSED: begin
            p0_run_rect = ! first_cycle;
            p0_mem_read = 1'b1;
            p0_reversed = 1'b1;
            cmd_next = rect_done;            
        end

        BLIT_COPY_RECT : begin
            p0_run_rect = ! first_cycle;
            p0_mem_read = 1'b1;
            cmd_next = rect_done;            
        end

        BLIT_FILL_RECT: begin
            p0_run_rect = ! first_cycle;
            cmd_next = rect_done;
            p0_fg_color = p0_cmd[72:64];
        end

        default: begin
            $display("Unknown blit command %x", p0_cmd);
            cmd_next = 1'b1;
        end
    endcase
    end
end

// ==============================================
//               Register Process
// ==============================================

always @(posedge clock) begin
    if (!stall) begin
        first_cycle <= cmd_next | ! p0_cmd_valid;
        dest_addr   <= next_dest_addr;   
        dest_bpr    <= next_dest_bpr;
        src_addr    <= next_src_addr;
        src_bpr     <= next_src_bpr;
        clip_x1     <= next_clip_x1;
        clip_y1     <= next_clip_y1;
        clip_x2     <= next_clip_x2;
        clip_y2     <= next_clip_y2;
        blit_trans_color <= reset ? -8'h1 : next_blit_trans_color;
        font_addr   <= next_font_addr;
        font_width  <= next_font_width;
        font_height <= next_font_height;
        font_bpr    <= next_font_bpr;
        font_bpc    <= next_font_bpc;

        p1_x1       <= p0_x1;
        p1_y1       <= p0_y1;
        p1_x2       <= p0_x2;
        p1_y2       <= p0_y2;
        p1_width    <= p0_width;
        p1_height   <= p0_height;
        p1_font_bpc <= font_bpc;
        p1_char     <= p0_char;
        p1_run_line <= p0_run_line && !line_done;
        p1_run_rect <= p0_run_rect && !rect_done;
        p2_active   <= p1_run_line | p1_run_rect;
        p3_active   <= p2_active;
        p4_active   <= p3_active;
        p5_active   <= p4_active;
        p1_reversed <= p0_reversed;
        p1_textmode <= p0_textmode;
        p2_textmode <= p1_textmode;
        p3_textmode <= p2_textmode;
        p4_textmode <= p3_textmode;
        p1_src_addr <= p0_textmode ? font_addr : src_addr;
        p1_src_bpr  <= p0_textmode ? font_bpr :src_bpr;
        p1_dest_addr<= dest_addr;
        p1_dest_bpr <= dest_bpr;
        p2_src_bpr  <= p1_src_bpr;
        p2_dest_addr<= p1_dest_addr;
        p2_dest_bpr <= p1_dest_bpr;
        p1_mem_read <= p0_mem_read & !reset;
        p2_mem_read <= p1_mem_read && !reset;
        p1_fg_color <= p0_fg_color;
        p2_fg_color <= p1_fg_color;
        p3_fg_color <= p2_fg_color;
        p4_fg_color <= p3_fg_color;
        p1_bg_color <= p0_bg_color;
        p2_bg_color <= p1_bg_color;
        p3_bg_color <= p2_bg_color;
        p4_bg_color <= p3_bg_color;
        p1_trans_color <= blit_trans_color;
        p2_trans_color <= p1_trans_color;
        p3_trans_color <= p2_trans_color;
        p4_trans_color <= p3_trans_color;
        p2_clip_x1  <= clip_x1;
        p2_clip_y1  <= clip_y1;
        p2_clip_x2  <= clip_x2;
        p2_clip_y2  <= clip_y2;
        p1_font_bpc <= font_bpc;
    
    end
end

endmodule