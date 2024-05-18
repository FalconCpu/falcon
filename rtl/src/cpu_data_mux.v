//                              cpu_data_mux
// 
// Fetches data from the register file, and directs it to the appropriate ports

`include "cpu.vh"
`timescale 1ns/1ps

module cpu_data_mux(
    input           clock,
    input           stall,
    input           reset,
    input [31:0]    p1_pc,

    // inputs from the instruction decoder
    input [4:0]     p2_reg_a,       // register number for 'a' source
    input [4:0]     p2_reg_b,       // register number for 'b' source
    input [1:0]     p2_src_data_a,  // which data source to use for alu 
    input [1:0]     p2_src_data_b,  // which data source to use for alu 
    input [1:0]     p2_src_addr_a,  // which data source to use for address gen
    input [1:0]     p2_src_addr_b,  // which data source to use for address gen
    input [31:0]    p2_literal,

    input [4:0]     p3_dest_reg,
    input [31:0]    p3_out,
    input [4:0]     p4_dest_reg,
    input [31:0]    p4_out,

    output reg [31:0] p3_data_a,
    output reg [31:0] p3_data_b,
    output reg [31:0] p3_addr_a,
    output reg [31:0] p3_addr_b
);

wire [31:0]  reg_a;
wire [31:0]  reg_b;
reg [31:0]   p2_data_a;
reg [31:0]   p2_data_b;
reg [31:0]   p2_addr_a;
reg [31:0]   p2_addr_b;

// synthesis translate_off
integer output_log_file;
initial begin
    output_log_file = $fopen("rtl_regs.log", "w");
end
// synthesis translate_on

wire wren = p4_dest_reg!=0 && !stall && !reset;

cpu_regfile  cpu_regfile_a (
    .clock(clock),
    .data(p4_out),
    .rdaddress(p2_reg_a),
    .wraddress(p4_dest_reg),
    .wren(wren),
    .q(reg_a)
);

cpu_regfile  cpu_regfile_b (
    .clock(clock),
    .data(p4_out),
    .rdaddress(p2_reg_b),
    .wraddress(p4_dest_reg),
    .wren(wren),
    .q(reg_b)
);


wire bypass_a3 = (p3_dest_reg == p2_reg_a) && (p3_dest_reg!=0);
wire bypass_b3 = (p3_dest_reg == p2_reg_b) && (p3_dest_reg!=0);
wire bypass_a4 = (p4_dest_reg == p2_reg_a) && (p4_dest_reg!=0);
wire bypass_b4 = (p4_dest_reg == p2_reg_b) && (p4_dest_reg!=0);

always @(*) begin
    case(p2_src_data_a) 
        `SRC_ZERO:    p2_data_a = 32'b0;
        `SRC_REG:     p2_data_a = bypass_a3 ? p3_out : bypass_a4 ? p4_out : reg_a;
        `SRC_PC:      p2_data_a = p1_pc;
        default:      p2_data_a = 32'bx;
    endcase

    case(p2_src_data_b) 
        `SRC_ZERO:    p2_data_b = 32'b0;
        `SRC_REG:     p2_data_b = bypass_b3 ? p3_out : bypass_b4 ? p4_out : reg_b;
        `SRC_LIT:     p2_data_b = p2_literal;
        default:      p2_data_b = 32'bx;
    endcase

    case(p2_src_addr_a) 
        `SRC_ZERO:    p2_addr_a = 32'b0;
        `SRC_REG:     p2_addr_a = bypass_a3 ? p3_out : bypass_a4 ? p4_out : reg_a;
        `SRC_PC:      p2_addr_a = p1_pc;
        default:      p2_addr_a = 32'bx;
    endcase

    case(p2_src_addr_b) 
        `SRC_ZERO:    p2_addr_b = 32'b0;
        `SRC_REG:     p2_addr_b = bypass_b3 ? p3_out : bypass_b4 ? p4_out : reg_b;
        `SRC_LIT:     p2_addr_b = p2_literal;
        default:      p2_addr_b = 32'bx;
    endcase
end

always @(posedge clock) begin
    if (!stall) begin
        p3_data_a <= p2_data_a;
        p3_data_b <= p2_data_b;
        p3_addr_a <= p2_addr_a;
        p3_addr_b <= p2_addr_b;
    	
        // synthesis translate_off
        if (wren)
            $fdisplay(output_log_file,"$%d = %x", p4_dest_reg, p4_out);
	    // synthesis translate_on
    end
end



endmodule