`timescale 1ns/1ns

module blit_combine(
    input             clock,
    input             stall,
    
    input [7:0]       in_data,
    input [25:0]      in_addr,
    input             in_en,
    input             in_active,

    output reg [25:0] out_addr,
    output reg [31:0] out_data,
    output reg [3:0]  out_byte_en,
    output reg        out_write
);

reg [31:0] next_data;
reg [25:0] next_addr;
reg [3:0]  next_byte_en;



always @(*) begin
    out_write    = 1'b0;
    next_addr    = out_addr;
    next_byte_en = out_byte_en;
    next_data    = out_data;

    if (in_active==1'b0) begin
        // Flush the buffer at the end of a blit
        out_write    = (out_byte_en!=4'h0);
        next_byte_en = 4'h0;
        next_addr    = 26'hx;
        next_data    = 32'hx;

    end else if (in_en) begin
        if (in_addr[25:2]!=out_addr[25:2]) begin
            out_write    = (out_byte_en!=4'h0);
            next_addr    = {in_addr[25:2],2'b0};
            next_byte_en = 4'h0;
            next_data    = 32'bx;
        end

        if (in_addr[1:0]==2'h0)
            next_data[7:0] = in_data;
        if (in_addr[1:0]==2'h1)
            next_data[15:8] = in_data;
        if (in_addr[1:0]==2'h2)
            next_data[23:16] = in_data;
        if (in_addr[1:0]==2'h3)
            next_data[31:24] = in_data;
        next_byte_en[ in_addr[1:0] ] = 1'b1;
    end
end

always @(posedge clock) begin
    if (!stall) begin
        out_addr <= next_addr;
        out_data <= next_data;
        out_byte_en <= next_byte_en;
    end
end

endmodule
