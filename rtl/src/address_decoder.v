`timescale 1ns / 1ns

// This module decodes the address bus and generates the appropriate requests
// routes the requests to the appropriate modules
//
// Address space:
// 0x00000000 - 0x03FFFFFF     SDRAM memory
// 0xE0000000 - 0xE000FFFF     Hardware registers
// 0xFFFF0000 - 0xFFFFFFFF    Instruction memory

module address_decoder(
    input        cpu_request,
    input [31:0] cpu_address,

    output reg   dmem_request,
    output reg   hwregs_request,
    output reg   imem_request,
    output reg   patmem_request,
    output reg   error_request    // trigger an invalid address error
);

always @(*) begin
    dmem_request = 0;
    hwregs_request = 0;
    imem_request = 0;
    patmem_request = 0;
    error_request = 0;


    if (cpu_request)
        if (cpu_address >= 32'h00000000 && cpu_address < 32'h04000000)
            dmem_request = 1'b1;
        else if (cpu_address >= 32'hE0000000 && cpu_address < 32'hE0010000)
            hwregs_request = 1;
        else if (cpu_address >= 32'hE1000000 && cpu_address < 32'hE1010000)
            patmem_request = 1;
        else if (cpu_address >= 32'hFFFF0000)
            imem_request = 1;
        else
            error_request = 1;
end

endmodule

