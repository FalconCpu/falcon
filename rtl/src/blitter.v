`timescale 1ns / 1ns
// ====================================================================================
//                              Blitter commands:
// ====================================================================================
// CODE NAME                    ARGS
// 1:   BLIT_SET_DEST_ADDR      address:Int, bytesPerRow:Short
// 2:   BLIT_SET_SRC_ADDR       address:Int, bytesPerRow:Short
// 3:   BLIT_FILL_RECT          width:Short, height:Short,x1:Short, y1:Short,  color:Short
// 4:   BLIT_COPY_RECT          width:Short, height:Short, destX:Short, destY:Short, srcX:Short, srcY:Short 
// 5:   BLIT_COPY_RECT_REVERSED width:Short, height:Short, destX:Short, destY:Short, srcX:Short, srcY:Short
// 6:   BLIT_SET_CLIP_RECT      x1:Short, y1:Short, x2:Short, y2:Short
// 7:   BLIT_SET_TRANS_COLOR    color:Short
// 8:   BLIT_SET_FONT           fontAddr:Int, fontWidth:Short, fontHeight:Short, fontBpr:Short, fontBpc:Short
// 9:   BLIT_DRAW_CHAR          char:Short, _:Short, x:Short, y:Short, color:Short, bgColor:Short
// 10:  BLIT_DRAW_LINE          x1:Short, y1:Short, x2:Short, y2:Short, color:Short

// ====================================================================================
//                             Pipeline overview
// ====================================================================================
// p0 : Decode command
// p1 : Calculate Coordinates
// p2 : Convert coordinates into address
// p3 : Fetch data from ram (or cache)
// p4 : Determine pixel color
// p5 : Combine byte writes to word
// fifo: Write fifo



module blitter(
    input             clock,
    input             reset,

    // Connections to the hwregs
    input [103:0]     blit_cmd,
    input             blit_start,
    output [7:0]      blit_slots_free,

    // connections to the memory bus - write
    output            blitw_request,
    output [25:0]     blitw_address,
    output [31:0]     blitw_wdata,
    output [3:0]      blitw_byte_en,
    input             blitw_ack,

    // connections to the memory bus - read
    output            blitr_request,
    output [25:0]     blitr_address,
    input  [31:0]     blitr_rdata,
    input             blitr_valid,
    input             blitr_ack,
    input             blitr_complete, 

    // connections to the pattern memory
    input             patmem_request,
    input [15:0]      patmem_address,
    input             patmem_write,
    input [3:0]       patmem_wstrb,
    input [31:0]      patmem_wdata,
    output [31:0]     patmem_rdata,
    output            patmem_ack,

    output [1:0]      debug_led
);

// ==================================================
//                 Signals
// ==================================================

// From command fifo
wire [103:0]     p0_cmd;
wire             p0_cmd_valid;

// From command interpreter
wire             cmd_next;
wire [15:0]      p1_x1;
wire [15:0]      p1_y1;
wire [15:0]      p1_x2;
wire [15:0]      p1_y2;
wire [15:0]      p1_width;
wire [15:0]      p1_height;
wire [7:0]       p1_char;
wire [7:0]       p1_font_bpc;
wire             p1_textmode;
wire             p1_run_line;
wire             p2_run_line;
wire             p1_run_rect;
wire             p2_run_rect;
wire             p1_reversed;
wire             p2_textmode;
wire             p4_textmode;
wire [31:0]      p1_src_addr;
wire [31:0]      p2_src_addr;
wire [15:0]      p2_src_bpr;
wire [25:0]      p2_dest_addr;
wire [15:0]      p2_dest_bpr;
wire             p3_mem_read;
wire             p4_mem_read;
wire [8:0]       p4_fg_color;
wire [8:0]       p4_bg_color;
wire [8:0]       p4_trans_color;
wire [15:0]      p2_clip_x1;
wire [15:0]      p2_clip_y1;
wire [15:0]      p2_clip_x2;
wire [15:0]      p2_clip_y2;
wire             p5_active;


// From rectangle engine
wire [15:0]   p2_rect_dest_x;
wire [15:0]   p2_rect_dest_y;
wire [15:0]   p2_rect_src_x;
wire [15:0]   p2_rect_src_y;
wire          rect_done;

// From line engine
wire [15:0]   p2_line_x;
wire [15:0]   p2_line_y;
wire          line_done;

// from address generator
wire [31:0] p3_src_addr;
wire [25:0] p3_dest_addr;
wire [2:0]  p3_src_bit;
wire        p3_write_en;
reg         p4_write_en;

// from data cache
wire [7:0]  p4_src_data;
wire        read_stall, write_stall;
reg  [25:0] p4_dest_addr;
reg  [25:0] p5_dest_addr;
reg  [2:0]  p4_src_bit;
wire [15:0] pattern_address;
wire [31:0] pattern_data;


// from color unit
wire [7:0]   p5_write_data;
wire         p5_write_enable;

// from the write combine unit
wire [25:0] p6_addr;
wire [31:0] p6_data;
wire [3:0]  p6_byte_en;
wire        p6_write;

assign debug_led[0] = p5_active;
assign debug_led[1] = stall;



// general
wire stall = (read_stall || write_stall) & !reset;

// ==================================================
//                 Command Fifo
// ==================================================

blit_cmd_fifo  blitter_cmd_fifo_inst (
    .clock(clock),
    .reset(reset),
    .blit_cmd(blit_cmd),
    .blit_start(blit_start),
    .blit_slots_free(blit_slots_free),
    .cmd(p0_cmd),
    .cmd_valid(p0_cmd_valid),
    .cmd_next(cmd_next & !stall)
  );

// ==================================================
//                 Command Interpreter
// ==================================================

blit_cmd  blit_cmd_inst (
    .clock(clock),
    .stall(stall),
    .reset(reset),
    .p0_cmd(p0_cmd),
    .p0_cmd_valid(p0_cmd_valid),
    .cmd_next(cmd_next),
    .p1_x1(p1_x1),
    .p1_y1(p1_y1),
    .p1_x2(p1_x2),
    .p1_y2(p1_y2),
    .p1_width(p1_width),
    .p1_height(p1_height),
    .p1_char(p1_char),
    .p1_font_bpc(p1_font_bpc),
    .p1_run_line(p1_run_line),
    .p2_run_line(p2_run_line),
    .p1_run_rect(p1_run_rect),
    .p2_run_rect(p2_run_rect),
    .p1_reversed(p1_reversed),
    .p1_textmode(p1_textmode),
    .p2_textmode(p2_textmode),
    .p4_textmode(p4_textmode),
    .p1_src_addr(p1_src_addr),
    .p2_src_bpr(p2_src_bpr),
    .p2_dest_addr(p2_dest_addr),
    .p2_dest_bpr(p2_dest_bpr),
    .p2_clip_x1(p2_clip_x1),
    .p2_clip_y1(p2_clip_y1),
    .p2_clip_x2(p2_clip_x2),
    .p2_clip_y2(p2_clip_y2),
    .p3_mem_read(p3_mem_read),
    .p4_mem_read(p4_mem_read),
    .p4_fg_color(p4_fg_color),
    .p4_bg_color(p4_bg_color),
    .p4_trans_color(p4_trans_color),
    .p5_active(p5_active),

    .line_done(line_done),
    .rect_done(rect_done)
  );

// ==================================================
//                 Rectangle Engine
// ==================================================

  blit_drawrect  blit_drawrect_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall),
    .start(p1_run_rect),
    .reversed(p1_reversed),
    .width(p1_width),
    .height(p1_height),
    .p1_x1(p1_x1),
    .p1_y1(p1_y1),
    .p1_x2(p1_x2),
    .p1_y2(p1_y2),
    .p2_rect_dest_x(p2_rect_dest_x),
    .p2_rect_dest_y(p2_rect_dest_y),
    .p2_rect_src_x(p2_rect_src_x),
    .p2_rect_src_y(p2_rect_src_y),
    .done(rect_done)
  );

// ==================================================
//                 Line Engine
// ==================================================

blit_drawline  blit_drawline_inst (
    .clock(clock),
    .stall(stall),
    .x1(p1_x1),
    .y1(p1_y1),
    .x2(p1_x2),
    .y2(p1_y2),
    .start(p1_run_line),
    .x(p2_line_x),
    .y(p2_line_y),
    .done(line_done)
  );

// ==================================================
//                Text Address Generator
// ==================================================

  blit_text_addr  blit_text_addr_inst (
    .clock(clock),
    .stall(stall),
    .p1_src_addr(p1_src_addr),
    .p1_char(p1_char),
    .p1_font_bpc(p1_font_bpc),
    .p1_textmode(p1_textmode),
    .p2_src_addr(p2_src_addr)
  );

// ==================================================
//                 Address Generator
// ==================================================

  blit_addrgen  blit_addrgen_inst (
    .clock(clock),
    .stall(stall),
    .clip_x1(p2_clip_x1),
    .clip_y1(p2_clip_y1),
    .clip_x2(p2_clip_x2),
    .clip_y2(p2_clip_y2),
    .p2_rect_dest_x(p2_rect_dest_x),
    .p2_rect_dest_y(p2_rect_dest_y),
    .p2_rect_src_x(p2_rect_src_x),
    .p2_rect_src_y(p2_rect_src_y),
    .p2_line_x(p2_line_x),
    .p2_line_y(p2_line_y),
    .p2_run_line(p2_run_line),
    .p2_run_rect(p2_run_rect),
    .p2_textmode(p2_textmode),
    .p2_src_addr(p2_src_addr),
    .p2_src_bpr(p2_src_bpr),
    .p2_dest_addr(p2_dest_addr),
    .p2_dest_bpr(p2_dest_bpr),
    .p3_src_addr(p3_src_addr),
    .p3_dest_addr(p3_dest_addr),
    .p3_src_bit(p3_src_bit),
    .p3_write_en(p3_write_en)
  );

// ==================================================
//                Data Cache
// ==================================================

blit_cache  blit_cache_inst (
    .clock(clock),
    .reset(reset),
    .read_address(p3_src_addr),
    .read_request(p3_mem_read),
    .read_data(p4_src_data),
    .read_stall(read_stall),
    .mem_address(blitr_address),
    .mem_request(blitr_request),
    .mem_data(blitr_rdata),
    .mem_valid(blitr_valid),
    .mem_ack(blitr_ack),
    .mem_complete(blitr_complete),
    .pattern_address(pattern_address),
    .pattern_data(pattern_data)
  );


// ==================================================
//                Color
// ==================================================

blit_color  blit_color_inst (
    .clock(clock),
    .stall(stall),
    .src_bit(p4_src_bit),
    .src_data(p4_src_data),
    .fg_color(p4_fg_color),
    .bg_color(p4_bg_color),
    .transparent_color(p4_trans_color),
    .write(p4_write_en),
    .textmode(p4_textmode),
    .mem_read(p4_mem_read),
    .wr_data(p5_write_data),
    .wr_enable(p5_write_enable)
  );

// ==================================================
//                Write combine
// ==================================================

blit_combine  blit_combine_inst (
    .clock(clock),
    .stall(stall),
    .in_data(p5_write_data),
    .in_addr(p5_dest_addr),
    .in_en(p5_write_enable),
    .in_active(p5_active),
    .out_addr(p6_addr),
    .out_data(p6_data),
    .out_byte_en(p6_byte_en),
    .out_write(p6_write)
  );


  
// ==================================================
//                 Output Fifo
// ==================================================

  blitter_fifo  blitter_fifo_inst (
    .clock(clock),
    .reset(reset),
    .wr_address(p6_addr),
    .wr_byte_en(p6_byte_en),
    .wr_data(p6_data),
    .wr_valid(p6_write & !stall),
    .wr_full(write_stall),
    .rd_address(blitw_address),
    .rd_byte_en(blitw_byte_en),
    .rd_data(blitw_wdata),
    .rd_valid(blitw_request),
    .rd_ready(blitw_ack)
  );


// ==================================================
//                Pattern memory
// ==================================================

  pattern_memory  pattern_memory_inst (
    .clock(clock),
    .request(patmem_request),
    .address(patmem_address),
    .write(patmem_write),
    .wstrb(patmem_wstrb),
    .wdata(patmem_wdata),
    .rdata(patmem_rdata),
    .ack(patmem_ack),
    .pattern_address(pattern_address),
    .pattern_data(pattern_data)
  );

// ==================================================
//                Pipeline regs
// ==================================================

  always @(posedge clock) begin
    if (!stall) begin
      p4_dest_addr  <= p3_dest_addr;
      p5_dest_addr  <= p4_dest_addr;
      p4_src_bit    <= p3_src_bit;
      p4_write_en   <= p3_write_en;
    end
  end

endmodule