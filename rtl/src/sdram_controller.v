`timescale 1ns/1ps

module sdram_controller (
    input        clock,
    input        reset,

    // connections to the SDRAM
    output	reg  [12:0]		DRAM_ADDR,
	output	reg   [1:0]		DRAM_BA,
	output	        		DRAM_CKE,
	inout       [15:0]		DRAM_DQ,
	output	        		DRAM_CS_N,
	output	         		DRAM_LDQM,
	output	         		DRAM_RAS_N,
	output	         		DRAM_UDQM,
	output	         		DRAM_WE_N,
	output	        		DRAM_CAS_N,

    // cpu port
    input                   sdram_request,
    input                   sdram_write,
    input [25:0]            sdram_address,
    input [31:0]            sdram_wdata,
    input [3:0]             sdram_byte_en,
    output [31:0]           sdram_rdata,
    output reg              sdram_ack
);

reg [6:0]  this_counter, next_counter;
reg [2:0]  this_state, next_state;
reg [12:0] next_addr;
reg [1:0]  next_ba;
reg [2:0]  next_cmd, this_cmd;
reg [15:0] next_dq, this_dq;
reg [1:0]  next_dqm, this_dqm;
reg        next_dqe, this_dqe;
reg        next_ack;
reg [4:0]  this_read_pipeline;
reg [15:0] reg0_dq, reg1_dq;
reg [9:0]  next_refresh_counter, this_refresh_counter;
reg        this_refresh_needed, next_refresh_needed;

// Address bit mapping
// 5432109876543 21 098765432 10
// RRRRRRRRRRRRR BB CCCCCCCCC MM

// from datasheet:-
// Function                             RAS CAS WE BA1 BA0 A10 A9 - A0
// No operation (NOP)	 	  	  	  	 H 	 H 	 H	 × 	 ×	 ×	 ×
// Burst stop (BST) 	 	  	  	  	 H 	 H 	 L 	 × 	 × 	 × 	 ×
// Read 	 	 	          	  	  	 H 	 L 	 H	 V 	 V 	 L 	 V	
// Read with auto precharge	  	  	  	 H 	 L 	 H 	 V 	 V 	 H	 V	
// Write 	 	              	  	 	 H	 L 	 L 	 V 	 V	 L	 V
// Write with auto precharge 	 	  	 H	 L	 L 	 V 	 V	 H 	 V
// Bank activate (ACT) 		  	 	 	 L 	 H	 H 	 V	 V 	 V 	 V
// Precharge select bank 	  	  	 	 L 	 H 	 L 	 V 	 V	 L 	 ×
// Precharge all banks  	  	  	 	 L	 H	 L	 × 	 ×	 H	 ×
// CBR Auto-Refresh (REF)	  	  	 	 L	 L	 H	 × 	 ×	 ×	 ×
// Self-Refresh (SELF)	 	  	  	 	 L	 L	 H	 × 	 ×	 ×	 ×
// Mode register set (MRS) 	 	  	  	 L 	 L 	 L 	 L 	 L 	 L	 V

parameter CMD_NOP        = 3'b111;
parameter CMD_READ       = 3'b101;
parameter CMD_WRITE      = 3'b100;
parameter CMD_ACT        = 3'b011;
parameter CMD_PRECHARGE  = 3'b010;
parameter CMD_REFRESH    = 3'b001;
parameter CMD_MODE       = 3'b000;

assign DRAM_CKE    = 1'b1;
assign DRAM_CS_N   = 1'b0;
assign DRAM_LDQM   = this_dqm[0];
assign DRAM_UDQM   = this_dqm[1];
assign DRAM_RAS_N  = this_cmd[2];
assign DRAM_CAS_N  = this_cmd[1];
assign DRAM_WE_N   = this_cmd[0];
assign DRAM_DQ     = this_dqe ? this_dq : 16'bz;
assign sdram_rdata   = this_read_pipeline[0] ? {reg0_dq, reg1_dq} : 32'b0;

parameter STATE_RESET   = 0;
parameter STATE_IDLE    = 1;
parameter STATE_READ    = 2;
parameter STATE_WRITE   = 3;
parameter STATE_REFRESH = 4;
parameter STATE_VGA     = 5;

always @(*) begin
    next_counter = this_counter + 1'b1;
    next_state = this_state;
    next_addr = 13'bx;
    next_ba = 2'bx;
    next_cmd = CMD_NOP;
    next_dq = 16'bx;
    next_dqm = 2'b11;
    next_dqe = 1'b0;
    next_ack = 1'b0;
    next_refresh_counter = this_refresh_counter + 1'b1;
    next_refresh_needed = this_refresh_needed;

    if (reset) begin
        next_counter = 7'd0;
        next_state = STATE_RESET;
        next_addr  = 13'b0;
        next_ba    = 2'b0;
        next_refresh_counter = 0;
        next_refresh_needed = 0;

    end else if (this_state==STATE_RESET) begin
        if (this_counter==1) begin
            next_addr = 13'h400;
            next_ba   = 2'h0;
            next_cmd  = CMD_PRECHARGE;
        end
        if (this_counter==8 || this_counter==16 || this_counter==24 || this_counter==32 || this_counter==40 || this_counter==48 || this_counter==56) begin
            next_cmd  = CMD_REFRESH;            
        end
        if (this_counter==64) begin
            next_addr = 13'h31;
            next_ba   = 2'b0;
            next_cmd  = CMD_MODE;
        end 
        if (this_counter==66)
            next_state = STATE_IDLE;

    end else if (this_state==STATE_IDLE) begin
        next_counter = 0;

        if (this_refresh_needed) begin
            next_addr = 13'h400;
            next_ba   = 2'b0;
            next_cmd  = CMD_REFRESH;
            next_state = STATE_REFRESH;
            next_refresh_needed = 1'b0;

        end else if (sdram_request) begin
            next_addr = sdram_address[25:13];
            next_ba   = sdram_address[12:11];
            next_cmd  = CMD_ACT;
            if (sdram_write) begin
                next_state = STATE_WRITE;
                $display("WRITE [%x]=%x %x", sdram_address, sdram_wdata, sdram_byte_en);
            end else
                next_state = STATE_READ;
        end

    end else if (this_state==STATE_READ) begin

        if (this_counter==1) begin
            next_addr = {3'b001, sdram_address[10:2], 1'b0};
            next_ba   = sdram_address[12:11];
            next_cmd  = CMD_READ;

        end else if (this_counter==2 || this_counter==3) begin
            next_dqm  = 2'b0;
    
        end else if (this_counter==7'd6) begin
            next_state = STATE_IDLE;
        end

        next_ack = (this_counter==6);

    end else if (this_state==STATE_WRITE) begin
        // need have one NOP after ACTIVATE, and 2 after WRITE

        if (this_counter==1) begin
            next_addr = {3'b001, sdram_address[10:2], 1'b0};
            next_ba   = sdram_address[12:11];
            next_cmd  = CMD_WRITE;
            next_dqm  = ~sdram_byte_en[1:0];
            next_dq   = sdram_wdata[15:0];
            next_dqe  = 1'b1;

        end else if (this_counter==7'd2) begin
            next_dqm  = ~sdram_byte_en[3:2];
            next_dq   = sdram_wdata[31:16];
            next_dqe  = 1'b1;
            next_ack  = 1'b1;

        end else if (this_counter==6) begin
            next_state = STATE_IDLE;
        end
    end else if (this_state==STATE_REFRESH) begin
        if (this_counter==6) begin
            next_state = STATE_IDLE;
        end
    end

    if (this_refresh_counter==700) begin
        next_refresh_needed = 1'b1;
        next_refresh_counter = 0;
    end

end


always @(posedge clock) begin
    this_counter <= next_counter;
    this_state   <= next_state;
    DRAM_ADDR    <= next_addr;
    DRAM_BA      <= next_ba;
    this_cmd     <= next_cmd;
    this_dq      <= next_dq;
    this_dqm     <= next_dqm;
    this_dqe     <= next_dqe;
    sdram_ack    <= next_ack;
    reg0_dq      <= DRAM_DQ;
    reg1_dq      <= reg0_dq;
    this_read_pipeline <= {this_cmd==CMD_READ && this_state==STATE_READ, this_read_pipeline[4:1]};
    this_refresh_counter <= next_refresh_counter;
    this_refresh_needed  <= next_refresh_needed;
end



endmodule