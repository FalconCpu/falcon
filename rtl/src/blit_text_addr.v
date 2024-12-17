`timescale 1ns/1ns

module blit_text_addr(
    input clock,
    input stall,

    input [25:0] p1_src_addr,
    input [7:0]  p1_char,
    input [7:0]  p1_font_bpc,
    input        p1_textmode,

    output reg [25:0]  p2_src_addr
);

always @(posedge clock) begin
    if (!stall) begin
        if (p1_textmode)
            p2_src_addr <= p1_src_addr + p1_char * p1_font_bpc;
        else
            p2_src_addr <= p1_src_addr;     
    end
end



endmodule