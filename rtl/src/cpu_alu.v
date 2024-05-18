`include "cpu.vh"
`timescale 1ns/1ps

module cpu_alu(
    input                clock,
    input                reset,   
    output reg           stall,    
    input      [31:0]    p3_data_a,     // data 'a' for alu opertations
    input      [31:0]    p3_data_b,     // data 'b' for alu opertations
    input      [31:0]    p3_addr_a,     // source 'a' for address operations
    input      [31:0]    p3_addr_b,     // source 'b' for address operations
    input      [6:0]     p3_op,         // operation to perform
    input      [1:0]     p3_opx,        // extra bits to specify shift opertations
    input                interupt,      // Interupt handler has a pending interupt
    input      [31:0]    p3_pc,         // address of current instruction

    output reg [31:0]    p3_out,        // result of alu operation
    output reg           p3_jump,       // indicate a jump is to occur
    output reg [31:0]    p3_jump_addr,  // address for a jump
    output reg [31:0]    p4_out,        // Final result of operation

    // cpu memory bus. 
    output reg           cpu_request,   // cpu is making a requst on the bus
    output reg           cpu_write,     // set for a write operation, clear for read
    output reg [31:0]    cpu_address,   // address for transaction
    output reg [31:0]    cpu_wdata,     // output data for transaction
    output reg [3:0]     cpu_byte_en,   // byte enables for write transaction
    input      [31:0]    cpu_rdata,      // received data
    input                cpu_valid
);


reg misaligned_address;
wire compare_eq  = p3_data_a == p3_data_b;
wire compare_lt  = $signed(p3_data_a) < $signed(p3_data_b);
wire compare_ltu = $unsigned(p3_data_a) < $unsigned(p3_data_b);

reg          interrupt_enable, next_interupt_enable;
reg  [31:0]  interrupt_pc, next_interrupt_pc;
reg  [31:0]  p4_in;
reg  [6:0]   p4_op;
reg  [1:0]   p4_addr;


reg  [31:0] p3_numerator, p4_numerator;
reg  [31:0] p3_denominator, p4_denominator;
reg  [31:0] p3_multiply, p4_multiply;
wire [31:0] p4_quotient;
wire [31:0] p4_remainder;
reg         p3_divide_sign, p4_divide_sign;
wire        p4_divide_done;
reg         p3_divide_start, p4_divide_start;

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


wire p3_is_memory_op = p3_op[6:4]==3'h1;
wire p4_is_memory_op = p4_op[6:4]==3'h1;

// Config registers
reg [4:0]     this_ecause, next_ecause;
reg [31:0]    this_epc, next_epc;
reg [31:0]    this_edata, next_edata;
reg [31:0]    this_escratch, next_escratch;
reg [31:0]    this_counter, next_counter;
reg [31:0]    cfg_read, cfg_write;



always @(*) begin
    next_interupt_enable = interrupt_enable;
    next_interrupt_pc    = interrupt_pc;
    p3_denominator       = 32'bx;
    p3_numerator         = 32'bx;
    p3_multiply          = 32'bx;
    p3_divide_start      = 1'b0;
    p3_divide_sign       = 1'bx;
    p3_out               = 32'bx;

    // -------------------------------------------------------------------
    //                    Memory operations
    // -------------------------------------------------------------------
    stall        = (p4_is_memory_op && !cpu_valid) || 
                   (p4_divide_start && !p4_divide_done);
    if(reset)
        stall = 1'b0;
    cpu_request  = p3_is_memory_op && !stall;
    cpu_address  = p3_addr_a + p3_addr_b;

    case (p3_op) 
        `OP_LDB: begin
            cpu_write          = 1'b0;
            cpu_wdata          = 32'bx;
            cpu_byte_en        = 4'bx;
            misaligned_address = 1'b0;
            end

        `OP_LDH: begin
            cpu_write          = 1'b0;
            cpu_wdata          = 32'bx;
            cpu_byte_en        = 4'bx;
            misaligned_address = cpu_address[0];
            end

        `OP_LDW: begin
            cpu_write          = 1'b0;
            cpu_wdata          = 32'bx;
            cpu_byte_en        = 4'bx;
            misaligned_address = | cpu_address[1:0];
            end

        `OP_STB: begin
            cpu_write          = 1'b1;
            misaligned_address = 1'b0;
            case(cpu_address[1:0]) 
                2'b00: begin
                    cpu_wdata = {24'bx, p3_data_b[7:0]}; 
                    cpu_byte_en = 4'b001;
                end

                2'b01: begin
                    cpu_wdata = {16'bx, p3_data_b[7:0], 8'bx}; 
                    cpu_byte_en = 4'b0010;
                end

                2'b10: begin
                    cpu_wdata = {8'bx, p3_data_b[7:0], 16'bx}; 
                    cpu_byte_en = 4'b0100;
                end

                2'b11: begin
                    cpu_wdata = {p3_data_b[7:0], 24'bx}; 
                    cpu_byte_en = 4'b1000;
                end
            endcase
        end

        `OP_STH: begin
            cpu_write          = 1'b1;
            case(cpu_address[1:0]) 
                2'b00: begin
                    cpu_wdata = {16'bx, p3_data_b[15:0]}; 
                    cpu_byte_en = 4'b0011;
                    misaligned_address = 1'b0;
                end

                2'b10: begin
                    cpu_wdata = {p3_data_b[15:0], 16'bx}; 
                    cpu_byte_en = 4'b1100;
                    misaligned_address = 1'b0;
                end

                default: begin
                    cpu_wdata = 32'bx;
                    cpu_byte_en = 4'bxxxx;
                    misaligned_address = 1'b1;
                end
            endcase
        end


        `OP_STW: begin
            cpu_write          = 1'b1;
            case(cpu_address[1:0]) 
                2'b00: begin
                    cpu_wdata = p3_data_b;
                    cpu_byte_en = 4'b1111;
                    misaligned_address = 1'b0;
                end

                default: begin
                    cpu_wdata = 32'bx;
                    cpu_byte_en = 4'bxxxx;
                    misaligned_address = 1'b1;
                end
            endcase
        end

        default: begin
            cpu_write = 1'b0;
            cpu_wdata = 32'bx;
            cpu_byte_en = 4'bxxxx;
            misaligned_address = 1'b0;    
        end
    endcase

    // -------------------------------------------------------------------
    //                    Stall logic
    // -------------------------------------------------------------------
    stall        = (p4_is_memory_op && !cpu_valid) || 
                   (p4_divide_start && !p4_divide_done);
    if(reset)
        stall = 1'b0;

    // block CPU memory accesses during reset, or if there is a misaligned address
    if (misaligned_address || reset)
        cpu_request = 1'b0;


    // -------------------------------------------------------------------
    //                    Config register ops
    // -------------------------------------------------------------------
    next_ecause    = this_ecause;
    next_epc       = this_epc;
    next_edata     = this_edata;
    next_escratch  = this_escratch;
    next_counter   = this_counter;
    cfg_read = 32'hx;
    if (p3_op==`OP_CFGLOAD || p3_op==`OP_CFGSET || p3_op==`OP_CFGCLR || p3_op==`OP_CFGJMP) 
    case (p3_data_b) 
        `CFG_ECAUSE:   cfg_read = this_ecause;
        `CFG_EPC:      cfg_read = this_epc;
        `CFG_EDATA:    cfg_read = this_edata;
        `CFG_ESCRATCH: cfg_read = this_escratch;
        `CFG_COUNTER:  cfg_read = this_counter;
        default:       cfg_read = 32'bx;
    endcase

    if (p3_op==`OP_CFGLOAD)
        cfg_write = p3_data_a;
    else if (p3_op==`OP_CFGSET)
        cfg_write = cfg_read | p3_data_a;
    else if (p3_op==`OP_CFGCLR)
        cfg_write = cfg_read & ~p3_data_a;
    else
        cfg_write = cfg_read;

    if (p3_op==`OP_CFGLOAD || p3_op==`OP_CFGSET || p3_op==`OP_CFGCLR) 
        case (p3_data_b) 
            `CFG_ECAUSE:   next_ecause    = cfg_write;
            `CFG_EPC:      next_epc       = cfg_write;
            `CFG_EDATA:    next_edata     = cfg_write;
            `CFG_ESCRATCH: next_escratch  = cfg_write;
            `CFG_COUNTER:  next_counter   = cfg_write;
        endcase
    
    // -------------------------------------------------------------------
    //                    ALU operations
    // -------------------------------------------------------------------

    case(p3_op) 
        `OP_AND    :  p3_out = p3_data_a & p3_data_b;
        `OP_OR     :  p3_out = p3_data_a | p3_data_b;
        `OP_XOR    :  p3_out = p3_data_a ^ p3_data_b;
        `OP_SHIFT  :  p3_out = (p3_opx==2'h0) ? p3_data_a << p3_data_b[4:0]
                             : (p3_opx==2'h3) ? $unsigned(p3_data_a) >> p3_data_b[4:0]
                             : (p3_opx==2'h2) ? $signed(p3_data_a) >> p3_data_b[4:0]
                             : 32'bx;
        `OP_ADD    :  p3_out = p3_data_a + p3_data_b;
        `OP_SUB    :  p3_out = p3_data_a - p3_data_b;
        `OP_CLT    :  p3_out = {31'b0, compare_lt};
        `OP_CLTU   :  p3_out = {31'b0, compare_ltu};
        `OP_JMP    :  p3_out = p3_data_a;
        `OP_LDU    :  p3_out = p3_data_b;
        `OP_LDPC   :  p3_out = p3_data_a + p3_data_b;
        `OP_MUL    :  p3_multiply = p3_data_a * p3_data_b;   // goes through pipeline register before result
        `OP_DIVU,
        `OP_MODU   :  begin
                         p3_numerator = p3_data_a;
                         p3_denominator = p3_data_b;
                         p3_divide_sign = 1'b0;
                         p3_divide_start = 1'b1;
                      end
        `OP_DIVS,
        `OP_MODS   :  begin
                        p3_numerator   = p3_data_a[31] ? -p3_data_a : p3_data_a;
                        p3_denominator = p3_data_b[31] ? -p3_data_b : p3_data_b;;
                        p3_divide_sign = p3_data_a[31] ^ p3_data_b[31];
                        p3_divide_start = 1'b1;
                    end
        `OP_CFGLOAD,
        `OP_CFGSET,
        `OP_CFGCLR:   p3_out = cfg_read;

        `OP_RETE   :  begin                         // Return from interupt
                        p3_out=32'bx;     
        next_interupt_enable=1'b1;  // enable interupts
        end  
        default    :  p3_out = 32'bx;
    endcase

    // -------------------------------------------------------------------
    //                  Fourth stage of pipeline
    // -------------------------------------------------------------------

    case(p4_op) 
        `OP_LDB:  p4_out = (p4_addr==2'h0) ? {{24{cpu_rdata[7]}},cpu_rdata[7:0]}
                         : (p4_addr==2'h1) ? {{24{cpu_rdata[15]}},cpu_rdata[15:8]}
                         : (p4_addr==2'h2) ? {{24{cpu_rdata[23]}},cpu_rdata[23:16]}
                         :                   {{24{cpu_rdata[31]}},cpu_rdata[31:24]};
        `OP_LDH:  p4_out = (p4_addr==2'b0) ? {{16{cpu_rdata[15]}},cpu_rdata[15:0]}
                        :                    {{16{cpu_rdata[31]}},cpu_rdata[31:16]};
        `OP_LDW:  p4_out = cpu_rdata;
        `OP_MUL:  p4_out = p4_multiply;
        `OP_DIVU,
        `OP_DIVS: p4_out = p4_quotient;
        `OP_MODU,
        `OP_MODS: p4_out = p4_remainder;
                
        default: p4_out = p4_in;
    endcase



    // -------------------------------------------------------------------
    //                    BRANCH operations
    // -------------------------------------------------------------------

    case (p3_op) 
        `OP_BEQ    : p3_jump = compare_eq;    
        `OP_BNE    : p3_jump = !compare_eq;    
        `OP_BLT    : p3_jump =  compare_lt;    
        `OP_BGE    : p3_jump = !compare_lt;    
        `OP_BLTU   : p3_jump =  compare_ltu;    
        `OP_BGEU   : p3_jump = !compare_ltu;    
        `OP_JMP    : p3_jump =  1'b1;
        `OP_RETE   : p3_jump = 1'b1;        // Return from interupt
        default    : p3_jump =  1'b0;    
    endcase

    if (interupt && interrupt_enable) begin
        next_interrupt_pc = p3_pc;              // Save address of current instruction
        p3_jump_addr = 32'hFFFF0004;       // Address of interupt service routine
        p3_jump      = 1'b1;
    end else if (p3_jump)
        p3_jump_addr = p3_addr_a + p3_addr_b;
    else
        p3_jump_addr = 32'bx;

end

// -------------------------------------------------------------------
//                    Registers
// -------------------------------------------------------------------

always @(posedge clock) begin
    if (!stall) begin
        p4_op <= p3_op;
        p4_in <= p3_out;
        p4_addr <= cpu_address[1:0];
        p4_multiply <= p3_multiply;
        p4_numerator <= p3_numerator;
        p4_denominator <= p3_denominator;
        p4_divide_sign <= p3_divide_sign;
        p4_divide_start <= p3_divide_start;

        this_ecause    <= next_ecause;
        this_epc       <= next_epc;
        this_edata     <= next_edata;
        this_escratch  <= next_escratch;
        this_counter   <= next_counter;
    

        if (reset) begin
            interrupt_enable <= 1'b1;
            interrupt_pc     <= 32'b0;
        end else begin
            interrupt_enable <= next_interupt_enable;
            interrupt_pc     <= next_interrupt_pc;
        end
    end
end


endmodule