// a 64kB Ram
`timescale 1ns/1ps

module instruction_ram(
    input          clock,

    // connection to the data bus
    input            iram_request,
    input            iram_write,
    input   [15:0]   iram_address,
    input   [31:0]   iram_wdata,
    input   [3:0]    iram_byte_en,
    output reg       iram_ack,
    output   [31:0]  iram_rdata,

    // connection to the instruction bus
    input      [31:0] instr_address,
    output     [31:0] instr_data
);

reg [31:0] rdata;
reg [31:0] instr_data_reg;
reg        instr_valid;

reg [31:0]  ram[0:16383];
initial begin
    $readmemh("rom.hex",ram);
end

assign iram_rdata = iram_ack ? rdata : 32'b0;
assign instr_data = instr_valid ? instr_data_reg : 32'bx;

always @(posedge clock) begin
    instr_data_reg <= {ram[instr_address[15:2]]};
    instr_valid <= instr_address[31:16]==16'hffff;

    iram_ack <= iram_request;

    // Have to use blocking assignment here - otherwise Quartus infers three port ram
    // verilator lint_off BLKSEQ  
    if (iram_request && iram_write) begin
        ram[iram_address[15:2]] = iram_wdata;
        $display("[%x]=%x",iram_address[15:2],iram_wdata);
    end
    rdata = ram[iram_address[15:2]];
    // verilator lint_on BLKSEQ

end

endmodule