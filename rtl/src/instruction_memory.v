`timescale 1ns / 1ns

module instruction_memory (
    input               clock,

    // connection to the data bus
    input               request,
    input [15:0]        address,
    input               write,
    input [3:0]         wstrb,
    input [31:0]        wdata,
    output [31:0]       rdata,
    output reg          ack,

    // Connections to the instruction bus
    input [15:2]        instr_address,
    output reg [31:0]   instr_data
);

reg [31:0] mem [0:16383];
reg [31:0] ram_q;

wire [13:0] word_addr = address[15:2];
assign rdata = ack ? ram_q : 32'b0;

initial begin
    $readmemh("asm.hex", mem);
end

always @(posedge clock) begin
    instr_data <= mem[instr_address];

    ack <= request;
    if (request && write) begin
        if (wstrb!= 4'hf)
            $display("Instruction memory only supports word writes");
        mem[word_addr] = wdata;
    end
    ram_q <= mem[word_addr];
end

endmodule