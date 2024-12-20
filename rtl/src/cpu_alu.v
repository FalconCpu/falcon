`timescale 1ns / 1ns
`include "f32.vh"

module cpu_alu(
    input               clock,
    input               reset,
    output              stall,
    input [31:0]        p2_pc,
    input [31:0]        p3_pc,
    input [31:0]        p4_pc,

    // connections from the decoder
    input [8:0]         p3_op,
    input [8:0]         p4_op,

    // connections from the register file
    input [31:0]        p3_data_a,
    input [31:0]        p3_data_b,
    input [31:0]        p3_data_c,

    // connections to the memory bus
    output              cpu_request,
    output reg [31:0]   cpu_address,
    output reg          cpu_write,
    output reg [3:0]    cpu_wstrb,
    output reg [31:0]   cpu_wdata,
    input  [31:0]       cpu_rdata,
    input               cpu_valid,      // rdata is valid
    input               cpu_mem_busy,   // memory is busy

    // outputs
    output reg [31:0]   p3_out,
    output reg [31:0]   p4_out,
    output reg          p3_jump,
    output reg [31:0]   p3_jump_target
);

// Common ops
wire clt = $signed(p3_data_a) < $signed(p3_data_b);
wire cltu = p3_data_a < p3_data_b;

wire [31:0] jump_target = p2_pc + p3_data_c;
wire [31:0] mem_addr = p3_data_a + p3_data_c;
reg [31:0] p3_mult, p4_mult;
reg [1:0] p3_addr_lsb, p4_addr_lsb;
reg  p3_is_mem;
reg p4_is_mem;
reg [31:0] p4_in;
reg [31:0] p4_cpu_address;

reg [31:0] this_epc ,     next_epc;
reg [3:0]  this_ecause ,  next_ecause;
reg [31:0] this_edata ,   next_edata;
reg [3:0]  this_estatus , next_estatus;
reg [31:0] this_escratch, next_escratch;
reg [3:0]  this_status,   next_status;
reg [31:0] cfg_read;

reg p3_misaligned_store, p4_misaligned_store;
reg p3_misaligned_load, p4_misaligned_load;
reg [31:0] p4_mem_address;

reg  [31:0] p3_numerator, p4_numerator;
reg  [31:0] p3_denominator, p4_denominator;
wire [31:0] p4_quotient;
wire [31:0] p4_remainder;
reg         p3_divide_sign, p4_divide_sign;
wire        p4_divide_done;
reg         p3_divide_start, p4_divide_start;
reg         raise_exception;



assign cpu_request = p3_is_mem && !stall;
assign stall = ((p4_is_mem && !cpu_valid) || (p4_divide_start && !p4_divide_done)) && !raise_exception && !reset;

always @(*) begin
    // default values
    p3_out = 32'bx;
    p3_jump = 1'b0;
    p3_jump_target = 32'bx;
    cpu_address = 32'bx;
    cpu_write = 1'bx;
    cpu_wstrb = 4'bx;
    cpu_wdata = 32'bx;
    p3_is_mem = 1'b0;
    p3_mult = 32'bx;
    p3_divide_start = 1'b0;
    p3_divide_sign = 1'bx;
    p3_numerator = 32'bx;
    p3_denominator = 32'bx;
    next_epc = this_epc;
    next_ecause = this_ecause;
    next_edata = this_edata;
    next_estatus = this_estatus;
    next_escratch = this_escratch;
    next_status = this_status;
    p3_misaligned_store = 1'b0;
    p3_misaligned_load = 1'b0;
    raise_exception = 1'b0;

    // config registers
    case(p3_data_b[12:0])
        13'd0: cfg_read = 32'h00000001;
        13'd1: cfg_read = this_epc;
        13'd2: cfg_read = this_ecause;
        13'd3: cfg_read = this_edata;
        13'd4: cfg_read = this_estatus;
        13'd5: cfg_read = this_escratch;
        13'd6: cfg_read = this_status;
        default: cfg_read = 32'bx;
    endcase

    if (p3_op == `INST_CFGW) begin
        case(p3_data_b[12:0]) 
            13'd1: next_epc = p3_data_a;
            13'd2: next_ecause = p3_data_a[3:0];
            13'd3: next_edata = p3_data_a;
            13'd4: next_estatus = p3_data_a[3:0];
            13'd5: next_escratch = p3_data_a;
            13'd6: next_status = p3_data_a[3:0];
				default: begin end 
        endcase
    end 

    // Stage 3 of the pipeline
    casex (p3_op)
        `INST_AND: 
            p3_out = p3_data_a & p3_data_b;

        `INST_OR:  
            p3_out = p3_data_a | p3_data_b;  

        `INST_XOR:
            p3_out = p3_data_a ^ p3_data_b; 

        `INST_ADD:
            p3_out = p3_data_a + p3_data_b;

        `INST_SUB: 
            p3_out = p3_data_a - p3_data_b;

        `INST_CLT: 
            p3_out = {31'b0, clt};

        `INST_CLTU:
            p3_out = {31'b0, cltu};

        `INST_SHIFT:
            case (p3_data_c[6:5])
                2'b00: p3_out = p3_data_a << p3_data_b[4:0];   // LSL
                2'b01: p3_out = 32'bx;                         // UNDEFINED
                2'b10: p3_out = p3_data_a >> p3_data_b[4:0];   // LSR
                2'b11: p3_out = $signed(p3_data_a) >>> p3_data_b[4:0]; // ASR
            endcase

        `INST_BEQ: begin 
            p3_jump = (p3_data_a == p3_data_b); 
            p3_jump_target = jump_target; 
        end

        `INST_BNE: begin 
            p3_jump = (p3_data_a != p3_data_b); 
            p3_jump_target = jump_target; 
        end

        `INST_BLT: begin 
            p3_jump = ($signed(p3_data_a) < $signed(p3_data_b));
            p3_jump_target = jump_target; 
        end

        `INST_BGE: begin 
            p3_jump = ($signed(p3_data_a) >= $signed(p3_data_b)); 
            p3_jump_target = jump_target; 
        end

        `INST_BLTU:begin 
            p3_jump = ($unsigned(p3_data_a) < $unsigned(p3_data_b));
            p3_jump_target = jump_target;
        end

        `INST_BGEU:begin
            p3_jump = ($unsigned(p3_data_a) >= $unsigned(p3_data_b));
            p3_jump_target = jump_target;
        end

        `INST_LDB: begin 
            p3_is_mem = 1'b1;
            cpu_address = mem_addr;
            cpu_write = 1'b0;
        end  

        `INST_LDH: begin
            p3_is_mem = 1'b1;
            cpu_address = mem_addr;
            cpu_write = 1'b0;
            p3_misaligned_load = mem_addr[0];
    
        end

        `INST_LDW: begin
            p3_is_mem = 1'b1;
            cpu_address = mem_addr;
            cpu_write = 1'b0;
            p3_misaligned_load = mem_addr[1:0] != 2'b00;
        end

        `INST_STB: begin 
            p3_is_mem = 1'b1;
            cpu_address = mem_addr;
            cpu_write = 1'b1; 
            if (mem_addr[1:0] == 2'b00) begin 
                cpu_wstrb = 4'b0001; 
                cpu_wdata = {24'bx,p3_data_b[7:0]}; 
            end else if (mem_addr[1:0] == 2'b01) begin
                cpu_wstrb = 4'b0010;
                cpu_wdata = {16'bx,p3_data_b[7:0],8'bx};
            end else if (mem_addr[1:0] == 2'b10) begin
                cpu_wstrb = 4'b0100;
                cpu_wdata = {8'bx,p3_data_b[7:0],16'bx};
            end else begin
                cpu_wstrb = 4'b1000;
                cpu_wdata = {p3_data_b[7:0],24'bx};
                end
            end
        
        `INST_STH: begin
            p3_misaligned_store = mem_addr[0];
            p3_is_mem = ! p3_misaligned_store;
            cpu_address = mem_addr;
            cpu_write = 1'b1;
            if (mem_addr[1:0] == 2'b00) begin
                cpu_wstrb = 4'b0011;
                cpu_wdata = {16'bx,p3_data_b[15:0]};
            end else if (mem_addr[1:0]==2'b10) begin
                cpu_wstrb = 4'b1100;
                cpu_wdata = {p3_data_b[15:0],16'bx};
            end else begin
                cpu_wstrb = 4'b0000; // TODO: Raise exception
                cpu_wdata = 32'bx;
            end
        end

        `INST_STW: begin
            p3_misaligned_store = mem_addr[1:0] != 2'b00;
            p3_is_mem = ! p3_misaligned_store;
            cpu_address = mem_addr;
            if (mem_addr[1:0] == 2'b00) begin
                cpu_write = 1'b1;
                cpu_wstrb = 4'b1111;
                cpu_wdata = p3_data_b;
            end else begin
                // TODO: Raise Exception
                cpu_write = 1'b0;
                cpu_wstrb = 4'b0000;
                cpu_wdata = 32'bx;
            end
        end

        `INST_LDU: begin
            p3_out = p3_data_c;
        end

        `INST_JMP: begin
            p3_jump = 1'b1;
            p3_jump_target = jump_target;   // PC relative target
            p3_out = p2_pc;
        end

        `INST_JMPR: begin
            p3_jump = 1'b1;
            p3_jump_target = p3_data_a + p3_data_c;
            p3_out = p2_pc;
        end

        `INST_LDPC:
            p3_out = jump_target;

        `INST_MUL:
            // Multiplication needs to be pipelined by one stage to prevent it becomming the
            // critical path. So rather than assigning to p3_out, we assign to a pipeline
            // register p3_mult - which then gets forwarded to p4_out in the next stage.
            p3_mult = p3_data_a * p3_data_b;

        `INST_DIVU,
        `INST_MODU   :  begin
            // Divide operations are handled by a separate module - code here just needs to route
            // the data into the divide unit and set the divide_start signal.
            p3_numerator = p3_data_a;
            p3_denominator = p3_data_b;
            p3_divide_sign = 1'b0;
            p3_divide_start = 1'b1;
        end

        `INST_DIVS,
        `INST_MODS   :  begin
            // The hardware divide unit only supports unsigned division. So we need to take the 
            // absolute value of the operands and then determine the sign of the result.
            p3_numerator   = p3_data_a[31] ? -p3_data_a : p3_data_a;
            p3_denominator = p3_data_b[31] ? -p3_data_b : p3_data_b;;
            p3_divide_sign = p3_data_a[31] ^ p3_data_b[31];
            p3_divide_start = 1'b1;
        end

        `INST_CFGR,
        `INST_CFGW:
            p3_out = cfg_read;

        `INST_RTE: begin
            p3_jump = 1'b1;
            p3_jump_target = this_epc;
            next_status = this_estatus;
        end
        default: 
            p3_out = 32'bx;

    endcase

    // Stage 4 of the pipeline
    case (p4_op)
        `INST_LDB:
            case(p4_addr_lsb)
                2'b00: p4_out = {{24{cpu_rdata[7]}},cpu_rdata[7:0]};
                2'b01: p4_out = {{24{cpu_rdata[15]}},cpu_rdata[15:8]};
                2'b10: p4_out = {{24{cpu_rdata[23]}},cpu_rdata[23:16]};
                2'b11: p4_out = {{24{cpu_rdata[31]}},cpu_rdata[31:24]};
            endcase

        `INST_LDH:
            case(p4_addr_lsb)
                2'b00: p4_out = {{16{cpu_rdata[15]}},cpu_rdata[15:0]};
                2'b10: p4_out = {{16{cpu_rdata[31]}},cpu_rdata[31:16]};
                default: p4_out = 32'bx; // Should never happen - exception would have been raised
            endcase

        `INST_LDW:
            p4_out = cpu_rdata;

        `INST_DIVU,
        `INST_DIVS:  p4_out = p4_quotient;

        `INST_MODU,
        `INST_MODS: p4_out = p4_remainder;

        `INST_MUL:
            p4_out = p4_mult;
            
        default:
            p4_out = p4_in;
    endcase

    // Exception handling
    if (p4_misaligned_load) begin
        next_epc = p4_pc;        
        next_edata = p4_cpu_address;
        next_ecause = `CAUSE_MISALIGNED_LOAD;
        raise_exception = 1'b1;
    end else if (p4_misaligned_store) begin
        next_epc = p4_pc;
        next_edata = p4_cpu_address;
        next_ecause = `CAUSE_MISALIGNED_STORE;
        raise_exception = 1'b1;
    end

    if (raise_exception) begin
        p3_jump = 1'b1;
        p3_jump_target = 32'hFFFF0004;
        next_estatus = this_status;
    end
end

always @(posedge clock) begin
    if(!stall) begin
        p4_in = p3_out;
        p4_addr_lsb = mem_addr[1:0];
        p4_mult = p3_mult;
        p4_numerator <= p3_numerator;
        p4_denominator <= p3_denominator;
        p4_divide_sign <= p3_divide_sign;
        p4_divide_start <= p3_divide_start;
        p4_is_mem <= p3_is_mem;
        p4_misaligned_load <= p3_misaligned_load;
        p4_misaligned_store <= p3_misaligned_store;
        p4_cpu_address <= cpu_address;
        this_epc <= next_epc;
        this_ecause <= next_ecause;
        this_edata <= next_edata;
        this_estatus <= next_estatus;
        this_escratch <= next_escratch;
        this_status <= next_status;
    end
end

cpu_divider  cpu_divider_inst (
    .clock(clock),
    .start(p4_divide_start),
    .numerator(p4_numerator),
    .denominator(p4_denominator),
    .sign(p4_divide_sign),
    .quotient(p4_quotient),
    .remainder(p4_remainder),
    .done(p4_divide_done)
  );

endmodule