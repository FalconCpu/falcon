`include "cpu.vh"
`timescale 1ns/1ps

module cpu_alu(
    input                clock,
    input                reset,       
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

    // cpu memory bus. 
    // The CPU stalls once cpu_req gets asserted, until cpu_ack is received
    output reg           cpu_request,   // cpu is making a requst on the bus
    output reg           cpu_write,     // set for a write operation, clear for read
    output reg [31:0]    cpu_address,   // address for transaction
    output reg [31:0]    cpu_wdata,     // output data for transaction
    output reg [3:0]     cpu_byte_en,   // byte enables for write transaction
    input      [31:0]    cpu_rdata,      // received data
    input                cpu_ack
);


reg misaligned_address;
wire compare_eq  = p3_data_a == p3_data_b;
wire compare_lt  = $signed(p3_data_a) < $signed(p3_data_b);
wire compare_ltu = $unsigned(p3_data_a) < $unsigned(p3_data_b);

reg          interrupt_enable, next_interupt_enable;
reg  [31:0]  interrupt_pc, next_interrupt_pc;


always @(*) begin
    next_interupt_enable = interrupt_enable;
    next_interrupt_pc    = interrupt_pc;

    // -------------------------------------------------------------------
    //                    Memory operations
    // -------------------------------------------------------------------
    cpu_request  = p3_op[6:4]==3'h1 && !cpu_ack;
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
                    cpu_wdata = {24'bx, p3_data_b[7:0]}; 
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

    // keep cpu_req low during reset to prevent stalls
    // don't assert the request signal if there is a misaligned address
    if (misaligned_address || reset)
        cpu_request = 1'b0;

    // -------------------------------------------------------------------
    //                    ALU operations
    // -------------------------------------------------------------------

    case(p3_op) 
        `OP_AND    :  p3_out = p3_data_a & p3_data_b;
        `OP_OR     :  p3_out = p3_data_a | p3_data_b;
        `OP_XOR    :  p3_out = p3_data_a ^ p3_data_b;
        `OP_SHIFT  :  p3_out = (p3_opx==2'h0) ? p3_data_a << p3_data_b[4:0]
                             : (p3_opx==2'h1) ? $unsigned(p3_data_a) >> p3_data_b[4:0]
                             : (p3_opx==2'h2) ? $signed(p3_data_a) >> p3_data_b[4:0]
                             : 32'bx;
        `OP_ADD    :  p3_out = p3_data_a + p3_data_b;
        `OP_SUB    :  p3_out = p3_data_a - p3_data_b;
        `OP_CLT    :  p3_out = {31'b0, compare_lt};
        `OP_CLTU   :  p3_out = {31'b0, compare_ltu};
        `OP_JMP    :  p3_out = p3_data_a;
        `OP_LDU    :  p3_out = p3_data_b;
        `OP_LDPC   :  p3_out = p3_data_a + p3_data_b;
        `OP_LDB    :  p3_out = (cpu_address[1:0]==2'h0) ? {{24{cpu_rdata[7]}},cpu_rdata[7:0]}
                             : (cpu_address[1:0]==2'h1) ? {{24{cpu_rdata[15]}},cpu_rdata[15:8]}
                             : (cpu_address[1:0]==2'h2) ? {{24{cpu_rdata[23]}},cpu_rdata[23:16]}
                             :                            {{24{cpu_rdata[31]}},cpu_rdata[31:24]};
        `OP_LDH    :  p3_out = (cpu_address[1]==1'b0)   ? {{16{cpu_rdata[15]}},cpu_rdata[15:0]}
                             :                         {{16{cpu_rdata[31]}},cpu_rdata[31:16]};
        `OP_LDW    :  p3_out = cpu_rdata;
        `OP_RETE   :  begin                         // Return from interupt
                        p3_out=32'bx;     
                        next_interupt_enable=1'b1;  // enable interupts
                      end  
        default    :  p3_out = 32'bx;
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
    if (reset) begin
        interrupt_enable <= 1'b1;
        interrupt_pc     <= 32'b0;
    end else begin
        interrupt_enable <= next_interupt_enable;
        interrupt_pc     <= next_interrupt_pc;
    end
end


endmodule