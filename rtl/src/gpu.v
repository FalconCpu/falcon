`timescale 1ns/1ns

module gpu(
    input   clock,
    input   reset,

    // slave interface to bus arbiter (for access to pattern ram)
    input               pram_request,
    input               pram_write,
    input   [15:0]      pram_address,
    input   [31:0]      pram_wdata,
    input   [3:0]       pram_byte_en,
    output              pram_valid,
    output   [31:0]     pram_rdata,

    // master interface to bus arbiter 
    output reg          gpu_request,
    output reg          gpu_write,
    output reg [25:0]   gpu_address,
    output reg [31:0]   gpu_wdata,
    output reg          gpu_burst,
    output reg [3:0]    gpu_byte_en,
    input  [31:0]       gpu_rdata,
    input               gpu_valid,
    input               gpu_complete,

    // connection to the hwregs
    input signed [15:0] gpu_x,               // X location on screen
    input signed [15:0] gpu_y,               // Y location on screen
    input signed [15:0] gpu_width,           // Width of operation (in pixels)
    input signed [15:0] gpu_height,          // Height of operation (in pixels)
    input        [15:0] gpu_pattern_offset,  // offset within pattern for pattern
    input        [7:0]  gpu_pattern_width,   // width of pattern in bytes
    input        [1:0]  gpu_pattern_depth,   // 1,2,4,8 bits per pixel
    input        [8:0]  gpu_color0,          // Background color (256=transparant)
    input        [8:0]  gpu_color1,          // Foreground color

    input        [7:0]  gpu_command,         // Action to perform
    input               gpu_start,           // Start pulse

    input signed [15:0] gpu_clip_x1,         // Clipping region
    input signed [15:0] gpu_clip_y1,
    input signed [15:0] gpu_clip_x2,
    input signed [15:0] gpu_clip_y2,

    output reg          gpu_busy,
    input        [31:0] gpu_screen_address
);

reg stall; 

reg signed [15:0] this_x, next_x;
reg signed [15:0] this_y, next_y;
reg               next_busy, this_busy_dly, this_busy_dly2;
reg signed [15:0] next_draw_x, this_draw_x;
reg signed [15:0] next_draw_y, this_draw_y;
reg               next_draw_enable, this_draw_enable;

reg        [31:0] next_byte_address, this_byte_address;
reg        [7:0]  next_byte_data, this_byte_data;
reg               next_byte_enable, this_byte_enable;

reg        [31:0] next_word_address, this_word_address;
reg        [31:0] next_word_data, this_word_data;
reg        [3:0]  next_word_enable, this_word_enable;

reg        next_gpu_request;
reg        next_gpu_write;
reg [25:0] next_gpu_address;
reg [31:0] next_gpu_wdata;
reg        next_gpu_burst;
reg [3:0]  next_gpu_byte_en;

wire [8:0] texture_color;

wire     same_word = (this_byte_address[31:2]==this_word_address[31:2]);

parameter [7:0]   CMD_PATTERN_RECTANGLE = 8'h2;
parameter [7:0]   CMD_SOLID_RECTANGLE   = 8'h3;

// ===========================================
//            The pattern ram
// ===========================================

pattern_ram  pattern_ram_inst (
    .clock(clock),
    .stall(stall),
    .pram_request(pram_request),
    .pram_write(pram_write),
    .pram_address(pram_address),
    .pram_wdata(pram_wdata),
    .pram_byte_en(pram_byte_en),
    .pram_valid(pram_valid),
    .pram_rdata(pram_rdata),
    .pattern_offset(gpu_pattern_offset),
    .pattern_width(gpu_pattern_width),
    .pattern_depth(gpu_pattern_depth),
    .pattern_color0(gpu_color0),
    .pattern_color1(gpu_color1),
    .pattern_solid(gpu_command[0]),
    .pattern_x(this_x),
    .pattern_y(this_y),
    .texture_color(texture_color)
  );

// ===========================================
//            The gpu operation
// ===========================================

always @(*) begin
    next_busy        = gpu_busy;
    next_x           = this_x;
    next_y           = this_y;
    next_draw_x      = 16'hx;
    next_draw_y      = 16'hx;
    next_draw_enable = 1'b0;
    next_word_address = this_word_address;
    next_word_data    = this_word_data;
    next_word_enable  = this_word_enable;
    next_gpu_address  = gpu_address;
    next_gpu_write    = gpu_write;
    next_gpu_wdata    = gpu_wdata;
    next_gpu_byte_en  = gpu_byte_en;
	 next_gpu_burst    = gpu_burst;
    stall             = 1'b0;
    next_gpu_request  = 1'b0;

    // ========================================================
    //     First stage of pipeline - interpret command and 
    //     identify pixels to draw
    // ========================================================

    if (reset) begin
        next_busy = 1'b0;

    end else if (gpu_start) begin
        // Got a command to start an operation

        if (gpu_busy) 
            $display("Time %t: ERROR gpu command interupted", $abstime);

        case(gpu_command)
            CMD_SOLID_RECTANGLE, CMD_PATTERN_RECTANGLE: begin
                next_x    = 0;
                next_y    = 0;                
                next_busy = 1'b1;  // ensure x1,y1 is top left corner
            end

            default: $display("Time %t: Unrecognized gpu command %x", $abstime, gpu_command);
        endcase

    end else if (gpu_busy) 
        // perform the operation
        case(gpu_command)
        CMD_SOLID_RECTANGLE, CMD_PATTERN_RECTANGLE: begin
            next_draw_x      = gpu_x + this_x;
            next_draw_y      = gpu_y + this_y;
            next_draw_enable = 1'b1;
            next_x           = this_x + 1'b1;
            if (next_x==gpu_width) begin
                next_x = 0;
                next_y = this_y + 1'b1;
                if (next_y==gpu_height)
                    next_busy = 1'b0; 
            end
        end
		  
		  default: begin end
    endcase


    // ===========================================================
    // Second stage of pipeline - check for clipping and calculate
    // address
    // ========================================================

    next_byte_address = gpu_screen_address + 640*this_draw_y + this_draw_x;
    next_byte_data    = texture_color[7:0];
    
    // Don't draw if pixel is outside clipping region, or if pixel is transparent
    next_byte_enable  = this_draw_enable && (this_draw_x>=gpu_clip_x1) && (this_draw_x<gpu_clip_x2) && 
                                            (this_draw_y>=gpu_clip_y1) && (this_draw_y<gpu_clip_y2) && 
                                            (texture_color[8]==1'b0);

    // ===========================================================
    // Stage 3 - combine byte operations into words
    // ===========================================================

    if (reset || gpu_complete) begin
        next_gpu_request = 1'b0;
        next_gpu_address = 26'bx;
        next_gpu_write   = 1'b0;
        next_gpu_burst   = 1'b0;
        next_gpu_wdata   = 32'bx;
    end                                                

    if (reset) begin
        next_word_address= 32'h0;
        next_word_enable = 4'h0;
        next_word_data   = 32'hx;    
    end

    if (this_word_address!=0 && ((this_byte_enable && !same_word) || !this_busy_dly2)) begin
        // pass the word we have built onto the sdram interface
        // If there is a pending request then stall the pipeline until it clears
        if(gpu_request)
            stall            = 1'b1;
        else begin
            next_gpu_request = 1'b1;
            next_gpu_address = this_word_address;
            next_gpu_write   = 1'b1;
            next_gpu_burst   = 1'b0;
            next_gpu_wdata   = this_word_data;
            next_gpu_byte_en = this_word_enable;
            next_word_address= 32'h0;
            next_word_enable = 4'h0;
            next_word_data   = 32'hx;
        end
    end

    if (this_byte_enable && (same_word || next_word_address==0)) begin
        next_word_address = {this_byte_address[31:2],2'b0};
        if (this_byte_address[1:0]==2'h0) begin
            next_word_data[7:0] = this_byte_data;
            next_word_enable[0] = 1'b1;
        end else if (this_byte_address[1:0]==2'h1) begin
            next_word_data[15:8] = this_byte_data;
            next_word_enable[1] = 1'b1;
        end else if (this_byte_address[1:0]==2'h2) begin
            next_word_data[23:16] = this_byte_data;
            next_word_enable[2] = 1'b1;
        end else begin
            next_word_data[31:24] = this_byte_data;
            next_word_enable[3] = 1'b1;
        end 
    end
end


always @(posedge clock) begin
    if (!stall) begin
        gpu_busy          <= next_busy;
        this_busy_dly     <= gpu_busy;
        this_busy_dly2    <= this_busy_dly;
        this_x            <= next_x;
        this_y            <= next_y;
        this_draw_x       <= next_draw_x;
        this_draw_y       <= next_draw_y;
        this_draw_enable  <= next_draw_enable;
        this_byte_address <= next_byte_address;
        this_byte_data    <= next_byte_data;
        this_byte_enable  <= next_byte_enable;
        this_word_address <= next_word_address;
        this_word_data    <= next_word_data;
        this_word_enable  <= next_word_enable;
        gpu_write         <= next_gpu_write;
        gpu_address       <= next_gpu_address;
        gpu_wdata         <= next_gpu_wdata;
        gpu_burst         <= next_gpu_burst;
        gpu_byte_en       <= next_gpu_byte_en;
    end

    gpu_request           <= ((gpu_request && !gpu_complete) || next_gpu_request) && !reset;
end

endmodule