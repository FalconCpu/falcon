`timescale 1ns / 1ns
// Blitter commands:
// CODE NAME                    ARGS
// 1:   BLIT_SET_DEST_ADDR      address:Int, bytesPerRow:Short
// 2:   BLIT_SET_SRC_ADDR       address:Int, bytesPerRow:Short
// 3:   BLIT_FILL_RECT          x1:Short, y1:Short, width:Short, height:Short, color:Short
// 4:   BLIT_COPY_RECT          destX:Short, destY:Short, width:Short, height:Short, srcX:Short, srcY:Short 
// 5:   BLIT_COPY_RECT_REVERSED destX:Short, destY:Short, width:Short, height:Short, srcX:Short, srcY:Short
// 6:   BLIT_SET_CLIP_RECT      x1:Short, y1:Short, x2:Short, y2:Short
// 7:   BLIT_SET_TRANS_COLOR    color:Short


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
    input             blitr_complete
);

// Blitter commands:
parameter BLIT_SET_DEST_ADDR      = 8'h1; 
parameter BLIT_SET_SRC_ADDR       = 8'h2; 
parameter BLIT_FILL_RECT          = 8'h3; 
parameter BLIT_COPY_RECT          = 8'h4; 
parameter BLIT_COPY_RECT_REVERSED = 8'h5; 
parameter BLIT_SET_CLIP_RECT      = 8'h6; 
parameter BLIT_SET_TRANS_COLOR    = 8'h7; 

// registers
reg [25:0]      dest_addr, next_dest_addr;   // start address of the destination buffer
reg [15:0]      dest_bpr, next_dest_bpr;     // bytes per row
reg [25:0]      src_addr, next_src_addr;     // start address of the source
reg [15:0]      src_bpr, next_src_bpr;       // bytes per row
reg [15:0]      clip_x1, next_clip_x1;
reg [15:0]      clip_y1, next_clip_y1;
reg [15:0]      clip_x2, next_clip_x2;
reg [15:0]      clip_y2, next_clip_y2;
reg [8:0]       blit_trans_color, next_blit_trans_color;

// Signals in the P1 Stage
reg [15:0]         p1_x, p1_next_x;        // The X-coordinate of the pixel being processed
reg [15:0]         p1_y, p1_next_y;        // The Y-coordinate of the pixel being processed
reg [15:0]         p1_dest_x, p1_dest_y;
reg [15:0]         p1_src_x,  p1_src_y;
reg [25:0]         p1o_dest_addr;
reg [25:0]         p1o_src_addr;
reg                p1o_write_en;
reg                p1o_read_en;
reg [8:0]          p1o_color;
reg                run_blit;
reg                cmd_next;
wire               cmd_valid;

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

// Extract the fields from the command
wire [103:0] fifo_cmd;
wire [7:0]  p1_cmd    = fifo_cmd[103:96];
wire [15:0] cmd_width  = fifo_cmd[15:0];
wire [15:0] cmd_height = fifo_cmd[31:16];
wire [15:0] cmd_x1     = fifo_cmd[47:32];
wire [15:0] cmd_y1     = fifo_cmd[63:48];
wire [15:0] cmd_x2     = fifo_cmd[79:64];
wire [15:0] cmd_y2     = fifo_cmd[95:80];



always @(*) begin
    p1_next_x = 16'h0;
    p1_next_y = 16'h0;
    p1o_write_en = 1'b0;
    next_dest_addr = dest_addr;   // start address of the destination buffer
    next_dest_bpr = dest_bpr;     // bytes per row
    next_src_addr = src_addr;     // start address of the source
    next_src_bpr  = src_bpr;       // bytes per row
    next_clip_x1  = clip_x1;
    next_clip_y1  = clip_y1;
    next_clip_x2  = clip_x2;
    next_clip_y2  = clip_y2;
    next_blit_trans_color = blit_trans_color;
    cmd_next = 1'b0;
    run_blit = 1'b0;

    // ***********************************************************
    //     Pipeline stage 1 : Decode the command
    // ***********************************************************

    if (! cmd_valid) begin
        cmd_next = 1'b0;
    end else begin
        case (p1_cmd)
        BLIT_SET_DEST_ADDR: begin
            next_dest_addr = fifo_cmd[25:0];
            next_dest_bpr = cmd_x1;
            cmd_next = 1'b1;
        end

        BLIT_SET_SRC_ADDR: begin
            next_src_addr = fifo_cmd[25:0];
            next_src_bpr = cmd_x1;
            cmd_next = 1'b1;
        end

        BLIT_SET_CLIP_RECT: begin
            next_clip_x1 = cmd_width;
            next_clip_y1 = cmd_height;
            next_clip_x2 = cmd_x1;
            next_clip_y2 = cmd_y1;
            cmd_next = 1'b1;
        end

        BLIT_SET_TRANS_COLOR: begin
            next_blit_trans_color = fifo_cmd[8:0];
            cmd_next = 1'b1;
        end 

        BLIT_COPY_RECT,
        BLIT_COPY_RECT_REVERSED,
        BLIT_FILL_RECT: begin
            run_blit = 1'b1;
        end

        default: begin
            $display("Unknown blit command %x", p1_cmd);
            cmd_next = 1'b1;
        end
    endcase
    end

    // ***********************************************************
    //        Pipeline stage 1 : Determine pixels to write
    // ***********************************************************

    if (run_blit) begin
        p1o_write_en = 1'b1;
        p1_next_x = p1_x + 16'h1;
        p1_next_y = p1_y;
        if (p1_next_x == cmd_width) begin
            p1_next_x = 0;
            p1_next_y = p1_y + 16'h1;
            if (p1_next_y == cmd_height) begin
                p1_next_y = 0;
                cmd_next = 1'b1;
            end
        end
    end 

    // Convert coordinates to memory address
    if (p1_cmd == BLIT_COPY_RECT_REVERSED) begin
        p1_dest_x = cmd_x1 - p1_x;
        p1_dest_y = cmd_y1 - p1_y;
        p1_src_x  = cmd_x2 - p1_x;
        p1_src_y  = cmd_y2 - p1_y;            
    end else begin
        p1_dest_x = cmd_x1 + p1_x;
        p1_dest_y = cmd_y1 + p1_y;
        p1_src_x  = cmd_x2 + p1_x;
        p1_src_y  = cmd_y2 + p1_y;            
    end

    p1o_dest_addr = (p1_dest_y * dest_bpr) + p1_dest_x;
    p1o_src_addr  = (p1_src_y  * src_bpr)  + p1_src_x ;
    
    // Check for clipping
    if (p1_dest_x<clip_x1 ||  p1_dest_x>=clip_x2 || p1_dest_y<clip_y1 || p1_dest_y>=clip_y2)
        p1o_write_en = 1'b0;

    p1o_color = cmd_x2[8:0];
    p1o_read_en = run_blit && (p1_cmd == BLIT_COPY_RECT || p1_cmd == BLIT_COPY_RECT_REVERSED);

    // ***********************************************************
    //  Pipeline Stage 2 - read from the cache
    // ***********************************************************

    p2o_dest_addr = dest_addr + p2i_dest_addr;
    p2o_src_addr  = src_addr  + p2i_src_addr;


    // ***********************************************************
    //  Pipeline Stage 3 - convert into a memory write
    // ***********************************************************

    p3o_dest_addr = p3i_dest_addr & 26'h3fffffc;
    if (p3i_cmd==BLIT_FILL_RECT)
        p3_data = p3i_color;
    else if (p3i_cmd==BLIT_COPY_RECT || p3i_cmd==BLIT_COPY_RECT_REVERSED)
        p3_data = p3i_sdram_data-1'b1;
    else
        p3_data = 8'hx;

    if (p3i_write_en==1'b0 ) begin // || p3_data == blit_trans_color) begin
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
        next_p4_byte_en <= 4'b0000;
        next_p4_addr <= 26'h0;
    end
end 

always @(posedge clock) begin
    if (!stall) begin
        p1_x  <= p1_next_x;
        p1_y  <= p1_next_y;

        p2i_cmd       <= p1_cmd;
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

        dest_addr     <= next_dest_addr;   // start address of the destination buffer
        dest_bpr      <= next_dest_bpr;     // bytes per row
        src_addr      <= next_src_addr;     // start address of the source
        src_bpr       <= next_src_bpr;       // bytes per row
        clip_x1       <= next_clip_x1;
        clip_y1       <= next_clip_y1;
        clip_x2       <= next_clip_x2;
        clip_y2       <= next_clip_y2;
        blit_trans_color <= next_blit_trans_color;
        

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

blitter_cmd_fifo  blitter_cmd_fifo_inst (
    .clock(clock),
    .reset(reset),
    .blit_cmd(blit_cmd),
    .blit_start(blit_start),
    .blit_slots_free(blit_slots_free),
    .cmd_cmd(fifo_cmd),
    .cmd_valid(cmd_valid),
    .cmd_next(cmd_next)
  );

endmodule