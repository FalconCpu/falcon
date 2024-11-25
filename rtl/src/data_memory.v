module data_memory (
    input               clock,
    input               request,
    input [31:0]        address,
    input               write,
    input [3:0]         wstrb,
    input [31:0]        wdata,
    output [31:0]       rdata,
    output reg          ack
);

reg [7:0] mem0 [0:1023];
reg [7:0] mem1 [0:1023];
reg [7:0] mem2 [0:1023];
reg [7:0] mem3 [0:1023];
reg [31:0] ram_q;

wire [9:0] word_addr = address[11:2];

assign rdata = ack ? ram_q : 32'b0;

always @(posedge clock) begin
    ram_q <= 32'bx;
    
    ack <= request;
    if (request) begin
        if (write) begin
            if (wstrb[0]) mem0[word_addr] <= wdata[7:0];
            if (wstrb[1]) mem1[word_addr] <= wdata[15:8];
            if (wstrb[2]) mem2[word_addr] <= wdata[23:16];
            if (wstrb[3]) mem3[word_addr] <= wdata[31:24];
            ack <= 1;
        end else begin
            ram_q <= {mem3[word_addr],mem2[word_addr],mem1[word_addr],mem0[word_addr]};
        end
    end 
end

endmodule