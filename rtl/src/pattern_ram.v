`timescale 1ns/1ns

module pattern_ram(
    input      clock,
    input      stall,

    // connections to the cpu
    input            pram_request,
    input            pram_write,
    input   [15:0]   pram_address,
    input   [31:0]   pram_wdata,
    input   [3:0]    pram_byte_en,
    output reg       pram_valid,
    output   [31:0]  pram_rdata,

    // connections to the hwregs
    input [15:0]     pattern_offset,      // offset within pattern ram that pattern starts
    input [7:0]      pattern_width,       // width of pattern in bytes
    input [1:0]      pattern_depth,       // 1,2,4,8 bits per pixel
    input [8:0]      pattern_color0,      // color to use for off pixels, 0-255 for colors or 256 for transparant
    input [8:0]      pattern_color1,      // color to use for on pixels, 0-255 for colors or 256 for transparant
    input            pattern_solid,       // ignore the pattern, and give a solid fill

    // connectrions to the gpu
    input signed [15:0] pattern_x,
    input signed [15:0] pattern_y,
    output [8:0]        texture_color     // result color, 0-255 for colors or 256 for transparant
);

//-----------------------------------------
//         the actual memory
//-----------------------------------------
reg [7:0] ram0[0:16383];
reg [7:0] ram1[0:16383];
reg [7:0] ram2[0:16383];
reg [7:0] ram3[0:16383];
reg [31:0] rdata;

//-----------------------------------------
// Interface to cpu
//-----------------------------------------
reg    pram_read;
assign pram_rdata = pram_read ? rdata : 32'h0;
wire [13:0] pram_addr = pram_address[15:2];
always @(posedge clock) begin
    if (pram_request && pram_write) begin
        if (pram_byte_en[0])   
            ram0[pram_addr] = pram_wdata[7:0];
        if (pram_byte_en[1])   
            ram1[pram_addr] = pram_wdata[15:8];
        if (pram_byte_en[2])   
            ram2[pram_addr] = pram_wdata[23:16];
        if (pram_byte_en[3])   
            ram3[pram_addr] = pram_wdata[31:24];
    end
    rdata <= {ram3[pram_addr], ram2[pram_addr], ram1[pram_addr], ram0[pram_addr]};
    pram_valid <= pram_request;
    pram_read  <= pram_request && !pram_write;
end

//-----------------------------------------
//   Address calculation
//-----------------------------------------

wire [15:0] x_in_bytes = (pattern_depth==2'h0) ? {pattern_x[15], pattern_x[15], pattern_x[15], pattern_x[15:3]}
                       : (pattern_depth==2'h1) ? {pattern_x[15], pattern_x[15], pattern_x[15:2]}
                       : (pattern_depth==2'h2) ? {pattern_x[15], pattern_x[15:1]}
                       :                         pattern_x[15:0];
wire [2:0] bit_offset  = (pattern_depth==2'h0) ? pattern_x[2:0]
                       : (pattern_depth==2'h1) ? {pattern_x[1:0], 1'b0}
                       : (pattern_depth==2'h2) ? {pattern_x[0], 2'b0}
                       :                         3'b0;
wire [15:0] pixel_address = pattern_offset + pattern_y*pattern_width + x_in_bytes;
wire [13:0] word_address = pixel_address[15:2];

// pipeline stage here as we fetch data from ram
reg [31:0]  p2_data_from_ram;   // data fetched from ram
reg [1:0]   p2_byte_offset;     // Byte offset into data
reg [2:0]   p2_bit_offset;      // Bit offset into word
always @(posedge clock) begin
    if (!stall) begin
        p2_data_from_ram <= {ram3[word_address], ram2[word_address], ram1[word_address], ram0[word_address]};
        p2_byte_offset   <= pixel_address[1:0];
        p2_bit_offset    <= bit_offset;
    end
end

// Get the byte we want
wire [7:0] p2_byte_from_ram = (p2_byte_offset==2'h0) ? p2_data_from_ram[7:0]
                            : (p2_byte_offset==2'h1) ? p2_data_from_ram[15:8]
                            : (p2_byte_offset==2'h2) ? p2_data_from_ram[23:16]
                            :                          p2_data_from_ram[31:24];

wire [7:0] p2_bits          = (pattern_depth==2'h0)  ? {7'b0, p2_byte_from_ram[p2_bit_offset]}
                            : (pattern_depth==2'h1)  ? {6'b0, p2_byte_from_ram[{p2_bit_offset[2:1],1'b1}],
                                                              p2_byte_from_ram[{p2_bit_offset[2:1],1'b0}]}
                            : (pattern_depth==2'h1)  ? {4'b0, p2_byte_from_ram[{p2_bit_offset[2],2'd3}],
                                                              p2_byte_from_ram[{p2_bit_offset[2],2'd2}],
                                                              p2_byte_from_ram[{p2_bit_offset[2],2'd1}],
                                                              p2_byte_from_ram[{p2_bit_offset[2],2'd0}]}
                            :                           p2_byte_from_ram;

assign texture_color        = pattern_solid           ? pattern_color1
                            : (p2_bits==8'h0)         ? pattern_color0
                            : (pattern_depth==2'h0)   ? pattern_color1
                            : (pattern_depth==2'h1)   ? {pattern_color1[8:2], p2_bits[1:0]}
                            : (pattern_depth==2'h2)   ? {pattern_color1[8:4], p2_bits[3:0]}
                            :                           {1'b0,p2_bits};

endmodule