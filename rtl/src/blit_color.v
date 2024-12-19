`timescale 1ns/1ns

module blit_color(
    input            clock,
    input            stall,
    input [2:0]      src_bit,
    input [7:0]      src_data,
    input [8:0]      fg_color,
    input [8:0]      bg_color,
    input [8:0]      transparent_color,
    input            write,
    input            textmode,
    input            mem_read,

    output reg [7:0] wr_data,
    output reg       wr_enable
);

reg [8:0] color;
reg       wr;

always @(*) begin
    if (textmode)
        color = src_data[src_bit] ? fg_color : bg_color;
    else if (mem_read)
        color = {1'b0,src_data};
    else
        color = fg_color;

    wr = write && (color!=transparent_color);
end

always @(posedge clock) begin
    if (!stall) begin
        wr_data   <= wr ? color[7:0] : 8'bx;
        wr_enable <= wr;
    end
end

endmodule