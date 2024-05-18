`timescale 1ns/1ps

module cpu(
    input           clock,
    input           reset,

    // data bus
    output         cpu_request,
    output         cpu_write,
    output [31:0]  cpu_address,
    output [31:0]  cpu_wdata,
    output [3:0]   cpu_byte_en,
    input  [31:0]  cpu_rdata,
    input          cpu_valid,
    // instruction bus
    // TODO - make this a proper icache
    output [31:0]  instr_address,
    input  [31:0]  instr_data
);

// signals driven by cpu_pc
wire [31:0]  p1_pc;
wire [31:0]  p2_pc;
wire [31:0]  p3_pc;

// signals driven by cpu_alu
wire [31:0]    p3_out;        // result of alu operation for low latency ops
wire           p3_jump;       // indicate a jump is to occur
wire [31:0]    p3_jump_addr;  // address for a jump
wire [31:0]    p4_out;        // Final result of alu operation
wire           stall;

// signals driven by data_mux
wire [31:0] p3_data_a;
wire [31:0] p3_data_b;
wire [31:0] p3_addr_a;
wire [31:0] p3_addr_b;

// signals driven by decoder
wire [4:0]   p2_reg_a;       // register number for 'a' source
wire [4:0]   p2_reg_b;       // register number for 'b' source
wire [1:0]   p2_src_data_a;  // which data source to use for alu 
wire [1:0]   p2_src_data_b;  // which data source to use for alu 
wire [1:0]   p2_src_addr_a;  // which data source to use for address gen
wire [1:0]   p2_src_addr_b;  // which data source to use for address gen
wire [31:0]  p2_literal;
wire         p2_bubble;
wire [6:0]   p3_op;          // operation to perform
wire [1:0]   p3_opx;         // extra bits to specify shift operations
wire [4:0]   p3_dest_reg;    // register to write data to by instr at p3
wire [4:0]   p4_dest_reg;    // register to write data to by instr at p4

cpu_pc  cpu_pc_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall),
    .p2_bubble(p2_bubble),
    .p3_jump(p3_jump),
    .p3_jump_addr(p3_jump_addr),
    .instr_address(instr_address),
    .p1_pc(p1_pc),
    .p2_pc(p2_pc),
    .p3_pc(p3_pc)
);

cpu_decoder  cpu_decoder_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall),
    .instr_data(instr_data),
    .p2_reg_a(p2_reg_a),
    .p2_reg_b(p2_reg_b),
    .p2_src_data_a(p2_src_data_a),
    .p2_src_data_b(p2_src_data_b),
    .p2_src_addr_a(p2_src_addr_a),
    .p2_src_addr_b(p2_src_addr_b),
    .p2_literal(p2_literal),
    .p2_bubble(p2_bubble),
    .p3_jump(p3_jump),
    .p3_op(p3_op),
    .p3_opx(p3_opx),
    .p3_dest_reg(p3_dest_reg),
    .p4_dest_reg(p4_dest_reg)
  );

cpu_data_mux  cpu_data_mux_inst (
    .clock(clock),
    .stall(stall),
    .reset(reset),
    .p1_pc(p1_pc),
    .p2_reg_a(p2_reg_a),
    .p2_reg_b(p2_reg_b),
    .p2_src_data_a(p2_src_data_a),
    .p2_src_data_b(p2_src_data_b),
    .p2_src_addr_a(p2_src_addr_a),
    .p2_src_addr_b(p2_src_addr_b),
    .p2_literal(p2_literal),
    .p3_dest_reg(p3_dest_reg),
    .p4_dest_reg(p4_dest_reg),
    .p3_out(p3_out),
    .p4_out(p4_out),
    .p3_data_a(p3_data_a),
    .p3_data_b(p3_data_b),
    .p3_addr_a(p3_addr_a),
    .p3_addr_b(p3_addr_b)
  );

cpu_alu  cpu_alu_inst (
    .clock(clock),
    .reset(reset),
    .stall(stall),
    .p3_data_a(p3_data_a),
    .p3_data_b(p3_data_b),
    .p3_addr_a(p3_addr_a),
    .p3_addr_b(p3_addr_b),
    .p3_op(p3_op),
    .p3_opx(p3_opx),
    .p3_out(p3_out),
    .p4_out(p4_out),
    .p3_jump(p3_jump),
    .p3_jump_addr(p3_jump_addr),
    .p3_pc(p3_pc),
    .interupt(1'b0),
    .cpu_request(cpu_request),
    .cpu_write(cpu_write),
    .cpu_address(cpu_address),
    .cpu_wdata(cpu_wdata),
    .cpu_byte_en(cpu_byte_en),
    .cpu_rdata(cpu_rdata),
    .cpu_valid(cpu_valid)
);

// synthesis translate_off
always @(posedge clock) begin
  if (p3_pc==0 && reset==0) begin
    @(posedge clock);
    @(posedge clock);
    $finish;
  end
end
// synthesis translate_on

endmodule