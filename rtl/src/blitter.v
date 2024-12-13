`timescale 1ns / 1ns

module blitter(
    input           clock,
    input           reset,

    // Connections to the hwregs
    input [7:0]     blit_cmd,        // Blitter command - see above for list of commands
    input [15:0]    blit_width,      // Width of the blit
    input [15:0]    blit_height,     // Height of the blit
    input           blit_start,      // Strobe to start the blit
    output reg      blit_busy,       // Indicates that the blitter is busy

    input [7:0]     blit_fgcolor,    // Foreground color
    input [7:0]     blit_bgcolor,    // Background color

    input [25:0]    blit_dest_addr,   // start address of the destination buffer
    input [15:0]    blit_dest_bpr,    // bytes per row
    input [15:0]    blit_dest_x,      // x coordinate for destination of blit
    input [15:0]    blit_dest_y,      // y coordinate for destination of blit

    input [25:0]    blit_src_addr,    // start address of the source buffer
    input [15:0]    blit_src_bpr,     // bytes per row
    input [15:0]    blit_src_x,       // x coordinate for source of blit
    input [15:0]    blit_src_y,       // 

    input [15:0]    blit_clip_x1,
    input [15:0]    blit_clip_y1,
    input [15:0]    blit_clip_x2,
    input [15:0]    blit_clip_y2,
    input [8:0]     blit_transparent_color,

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
    input             blitr_complete
);

// Signals in the P1 Stage
reg                next_blit_busy;         
reg [15:0]         p1_x, p1_next_x;        // The X-coordinate of the pixel being processed
reg [15:0]         p1_y, p1_next_y;        // The Y-coordinate of the pixel being processed
reg [15:0]         p1_dest_x, p1_dest_y;
reg [15:0]         p1_src_x,  p1_src_y;
reg [25:0]         p1o_dest_addr;
reg [25:0]         p1o_src_addr;
reg                p1o_write_en;
reg                p1o_read_en;
reg [8:0]          p1o_color;
reg [7:0]          p1o_cmd;

// Signals in the p2 stage
reg [7:0]          p2i_cmd;
reg [8:0]          p2i_color;
reg [25:0]         p2i_dest_addr;
reg                p2i_write_en;
reg [25:0]         p2i_src_addr;
reg                p2_read_en;
reg [25:0]         p2o_dest_addr;
reg [25:0]         p2o_src_addr;

// Signals in the p3 stage
wire [7:0]         p3i_sdram_data;
reg [7:0]          p3i_cmd;
reg [8:0]          p3i_color;
reg [25:0]         p3i_dest_addr;
reg                p3i_write_en;
reg [8:0]          p3_data;
reg [25:0]         p3o_dest_addr;
reg [31:0]         p3o_data;
reg [3:0]          p3o_byte_en;

// Signals in the p4 stage
reg [25:0]         p4i_dest_addr;
reg [31:0]         p4i_data;
reg [3:0]          p4i_byte_en;
reg [25:0]         p4_addr,  next_p4_addr;
reg [31:0]         p4_data,  next_p4_data;
reg [3:0]          p4_byte_en,next_p4_byte_en;
reg                p4_write;


// connections to the write fifo
wire        fifo_wr_ready;
wire        fifo_rd_valid;
wire [25:0] fifo_rd_address;
wire [3:0]  fifo_rd_byte_en;
wire [31:0] fifo_rd_data;

wire read_stall;
wire stall = (read_stall || !fifo_wr_ready) && !reset;

parameter COLOR_TRANSPARENT = 9'h1FF;

parameter CMD_IDLE      = 8'h00;
parameter CMD_FILL_RECT = 8'h01;
parameter CMD_COPY_RECT = 8'h02;
parameter CMD_COPY_RECT_REVERSED = 8'h03;

always @(*) begin
    next_blit_busy = blit_busy;
    p1_next_x = p1_x;
    p1_next_y = p1_y;
    p1o_write_en = 1'b0;

    // ***********************************************************
    //        Pipeline stage 1 : Determine pixels to write
    // ***********************************************************

    if (blit_busy) begin
        p1_next_x = p1_x + 16'h1;
        if (p1_next_x == blit_width) begin
            p1_next_x = 0;
            p1_next_y = p1_y + 16'h1;
            if (p1_next_y == blit_height) begin
                next_blit_busy = 1'b0;
            end
        end
        p1o_write_en = 1'b1;
    end else begin
        next_blit_busy = blit_start;
        p1_next_x = 0;
        p1_next_y = 0;
        p1o_color = COLOR_TRANSPARENT;    
    end

    // Convert coordinates to memory address
    if (blit_cmd == CMD_COPY_RECT_REVERSED) begin
        p1_dest_x = blit_dest_x - p1_x;
        p1_dest_y = blit_dest_y - p1_y;
        p1_src_x  = blit_src_x - p1_x;
        p1_src_y  = blit_src_y - p1_y;            
    end else begin
        p1_dest_x = blit_dest_x + p1_x;
        p1_dest_y = blit_dest_y + p1_y;
        p1_src_x  = blit_src_x + p1_x;
        p1_src_y  = blit_src_y + p1_y;            
    end
    p1o_dest_addr = (p1_dest_y * blit_dest_bpr) + p1_dest_x;
    p1o_src_addr  = (p1_src_y  * blit_src_bpr)  + p1_src_x ;
    if (p1_dest_x<blit_clip_x1 ||  p1_dest_x>=blit_clip_x2 || p1_dest_y<blit_clip_y1 || p1_dest_y>=blit_clip_y2)
        p1o_write_en = 1'b0;
    p1o_color = blit_fgcolor;
    p1o_cmd = blit_cmd;
    p1o_read_en = blit_busy && (blit_cmd == CMD_COPY_RECT);

    // ***********************************************************
    //  Pipeline Stage 2 - read from the cache
    // ***********************************************************

    p2o_dest_addr = blit_dest_addr + p2i_dest_addr;
    p2o_src_addr  = blit_src_addr  + p2i_src_addr;


    // ***********************************************************
    //  Pipeline Stage 3 - convert into a memory write
    // ***********************************************************

    p3o_dest_addr = p3i_dest_addr & 26'h3fffffc;
    if (p3i_cmd==CMD_FILL_RECT)
        p3_data = p3i_color;
    else if (p3i_cmd==CMD_COPY_RECT || blit_cmd==CMD_COPY_RECT_REVERSED)
        p3_data = p3i_sdram_data;
    else
        p3_data = 8'hx;

    if (p3i_write_en==1'b0 || p3_data == blit_transparent_color) begin
        p3o_dest_addr = 26'bx;
        p3o_byte_en = 4'b0000;
        p3o_data = 32'bx;
    end else if (p3i_dest_addr[1:0]==0) begin
        p3o_byte_en = 4'b0001;
        p3o_data = {24'hx,p3_data[7:0]};
    end else if (p3i_dest_addr[1:0]==1) begin
        p3o_byte_en = 4'b0010;
        p3o_data = {16'hx,p3_data[7:0],8'hx};
    end else if (p3i_dest_addr[1:0]==2) begin
        p3o_byte_en = 4'b0100;
        p3o_data = {8'hx,p3_data[7:0],16'hx};
    end else begin
        p3o_byte_en = 4'b1000;
        p3o_data = {p3_data[7:0],24'hx};
    end

    // ***********************************************************
    //     Pipeline Stage 3 - combine byte writes into words
    // ***********************************************************
    p4_write = 1'b0;
    next_p4_addr = p4_addr;
    next_p4_byte_en = p4_byte_en;
    next_p4_data = p4_data;

    if (p4i_byte_en!=4'b0000) begin
        // See if we need to flush the current buffer
        if (p4i_dest_addr != p4_addr) begin
            p4_write = p4_byte_en!=4'h0;
            next_p4_addr    <= p4i_dest_addr;
            next_p4_byte_en <= p4i_byte_en;
            next_p4_data    <= p4i_data;
        end else begin
            p4_write = 1'b0;
            if (p4i_byte_en[0])
                next_p4_data[7:0] <= p4i_data[7:0];
            if (p4i_byte_en[1])
                next_p4_data[15:8] <= p4i_data[15:8];
            if (p4i_byte_en[2])
                next_p4_data[23:16] <= p4i_data[23:16];
            if (p4i_byte_en[3])
                next_p4_data[31:24] <= p4i_data[31:24];
            next_p4_byte_en <= p4_byte_en | p4i_byte_en;
        end
    end

    if (reset) begin
        next_blit_busy = 1'b0;
        next_p4_byte_en <= 4'b0000;
        next_p4_addr <= 26'h0;
    end
end 

always @(posedge clock) begin
    if (!stall) begin
        
        blit_busy <= next_blit_busy;
        p1_x  <= p1_next_x;
        p1_y  <= p1_next_y;

        p2i_cmd       <= p1o_cmd;
        p2i_color     <= p1o_color;
        p2i_write_en  <= p1o_write_en;
        p2i_dest_addr <= p1o_dest_addr;
        p2i_src_addr   <= p1o_src_addr;
        p2_read_en    <= p1o_read_en;

        p3i_cmd       <= p2i_cmd;       
        p3i_color     <= p2i_color;     
        p3i_write_en  <= p2i_write_en;  
        p3i_dest_addr <= p2o_dest_addr; 
        
        p4i_dest_addr <= p3o_dest_addr;
        p4i_data      <= p3o_data;
        p4i_byte_en   <= p3o_byte_en;
        p4_addr       <= next_p4_addr;
        p4_byte_en    <= next_p4_byte_en;
        p4_data       <= next_p4_data;
    end
end

// Instantiate the FIFO
blitter_fifo #(
    .DEPTH(256)
) write_fifo (
    .clock(clock),
    .reset(reset),
    
    // Write side (from blitter pipeline)
    .wr_address(p4_addr),
    .wr_byte_en(p4_byte_en),
    .wr_data(p4_data),
    .wr_valid(p4_write && !stall),
    .wr_ready(fifo_wr_ready),
    
    // Read side (to memory interface)
    .rd_address(blitw_address),
    .rd_byte_en(blitw_byte_en),
    .rd_data(blitw_wdata),
    .rd_valid(blitw_request),
    .rd_ready(blitw_ack)
);

blitter_cache  blitter_cache_inst (
    .clock(clock),
    .reset(reset),
    .read_address(p2o_src_addr),
    .read_request(p2_read_en),
    .read_data(p3i_sdram_data),
    .read_stall(read_stall),
    .mem_address(blitr_address),
    .mem_request(blitr_request),
    .mem_data(blitr_rdata),
    .mem_valid(blitr_valid),
    .mem_ack(blitr_ack),
    .mem_complete(blitr_complete)
  );

endmodule