`timescale 1ns/1ns

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

    // connections to the arbiter
    input                   sdram_request,      // Held high while a transaction is requested  
    input [3:0]             sdram_master,       // Which master was requesting this transaction
    input                   sdram_write,        // Set if the transaction is a write
    input [25:0]            sdram_address,      // Address of transaction (lowest 2 lsb's not used)
    input [31:0]            sdram_wdata,        // Data for a write
    input [3:0]             sdram_byte_en,      // Byte enables for a write
    input                   sdram_burst,        // Set for a 32 byte burst, Clear for single 4 byte transaction
    output [31:0]           sdram_rdata,        // Data for a read
    output reg [3:0]        sdram_valid,        // Pulse with the value of the master to indicate the read data is valid
    output reg [3:0]        sdram_complete,     // pulsed to value of master to indicate a burst trnsaction has been completed
    output reg              sdram_ready         // Set to indicate the SDRAM is ready to accept a new transaction
);

reg [6:0]  this_counter, next_counter;
reg [2:0]  this_state, next_state;
reg [3:0]  next_master, this_master;
reg [12:0] next_addr;
reg [1:0]  next_ba;
reg [2:0]  next_cmd, this_cmd;
reg [15:0] next_dq, this_dq;
reg [1:0]  next_dqm, this_dqm;
reg        next_dqe, this_dqe;
reg [2:0]  next_col, this_col;
reg [3:0]  next_valid;
reg [3:0]  next_complete;
reg [15:0] reg0_dq, reg1_dq;
reg [9:0]  next_refresh_counter, this_refresh_counter;
reg        this_refresh_needed, next_refresh_needed;
reg [12:0] this_latched_address, next_latched_address;
reg [15:0] wdata_msb_delay;   // MSBs of wdata delayed by one clock cycle
reg [1:0]  byte_en_delay;     // MSB of byte enable delayed by one clock cycle
reg [1:0]  prev_writes;       // Shift reg to remember when the last write cycle was

reg [12:0] bank_addr[0:3];
reg [3:0]  bank_open;

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
assign sdram_rdata   = sdram_valid ? {reg0_dq, reg1_dq} : 32'b0;

parameter STATE_RESET      = 0;
parameter STATE_IDLE       = 1;
parameter STATE_READ       = 2;
parameter STATE_WRITE      = 3;
parameter STATE_REFRESH    = 4;
parameter STATE_READ_BURST = 5;
parameter STATE_PRECHARGE  = 6;
parameter STATE_ACTIVATE   = 7;

wire        selected_bank_open = bank_open[ sdram_address[12:11]];
wire [12:0] selected_bank_addr = bank_addr[ sdram_address[12:11]];


always @(*) begin
    next_counter = this_counter + 1'b1;
    next_state = this_state;
    next_addr = 13'bx;
    next_ba = DRAM_BA;
    next_cmd = CMD_NOP;
    next_dq = 16'bx;
    next_dqm = 2'b11;
    next_dqe = 1'b0;
    next_valid = 4'b0;
    next_complete = 4'b0;
    next_refresh_counter = this_refresh_counter + 1'b1;
    next_refresh_needed = this_refresh_needed;
    next_latched_address = this_latched_address;
    next_col = this_col;
    next_master = this_master;
    sdram_ready = 1'b0;


    if (reset) begin
        next_counter = 7'd0;
        next_state = STATE_RESET;
        next_addr  = 13'b0;
        next_ba    = 2'b0;
        next_refresh_counter = 0;
        next_refresh_needed = 0;
        next_col = this_col;

    end else if (this_state==STATE_RESET) begin
        // The SDRAM requires at least 8 precharge/refresh cycles after reset
        if (this_counter==1) begin
            next_addr = 13'h400;
            next_ba   = 2'h0;
            next_cmd  = CMD_PRECHARGE;
        end
        if (this_counter==8 || this_counter==16 || this_counter==24 || this_counter==32 || this_counter==40 || this_counter==48 || this_counter==56) begin
            next_cmd  = CMD_REFRESH;            
        end
        // Set the mode register
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
            next_state = STATE_REFRESH;
            next_refresh_needed = 1'b0;

        end else if (sdram_request) begin
            if (selected_bank_open && selected_bank_addr!=sdram_address[25:13]) begin
                // Bank is open, but at the wrong page. Need to precharge it.
                // Need to wait for the previous write to complete before we can precharge
                if (prev_writes==2'b0) begin
                    next_cmd = CMD_PRECHARGE;
                    next_ba = sdram_address[12:11];
                    next_addr = sdram_address[25:13];
                    next_state = STATE_PRECHARGE;
                end
            end else if (!selected_bank_open) begin
                next_cmd = CMD_ACT;
                next_ba = sdram_address[12:11];
                next_addr = sdram_address[25:13];
                next_state = STATE_ACTIVATE;
            end else if (sdram_write) begin
                $display("WRITE MASTER=%x ADDR=%x DATA=%x %x", sdram_master, sdram_address, sdram_wdata, sdram_byte_en);
                next_addr = {3'b000, sdram_address[10:2], 1'b0};
                next_ba   = sdram_address[12:11];
                next_cmd  = CMD_WRITE;
                next_dqm  = ~sdram_byte_en[1:0];
                next_dq   = sdram_wdata[15:0];
                next_dqe  = 1'b1;
                sdram_ready= 1'b1;
                next_state = STATE_WRITE;
            end else begin // read
                $display("READ MASTER=%x ADDR=%x", sdram_master, sdram_address);
                next_addr = {3'b000, sdram_address[10:2], 1'b0};
                next_latched_address = sdram_address[10:0];
                next_ba   = sdram_address[12:11];
                next_cmd  = CMD_READ;
                next_dqm  = ~sdram_byte_en[1:0];
                next_col  = sdram_address[4:2] + 1'b1;
                next_master=sdram_master;
                sdram_ready= 1'b1;
                next_state = sdram_burst ? STATE_READ_BURST : STATE_READ;
            end
        end else begin
            next_addr = 13'bx;
            next_ba   = 2'bx;
            next_cmd  = CMD_NOP;
            next_dqm  = 2'b11;
            next_col  = 3'bx;
            next_state = STATE_IDLE;
            sdram_ready = 1'b1;
        end

    end else if (this_state==STATE_READ) begin
        if (this_counter<=1) 
            next_dqm  = 2'b0;
    
        if (this_counter==7'd3) 
            next_complete = this_master;

        if (this_counter==7'd4)
            next_valid = this_master;

        if (this_counter==7'd5)
            next_state = STATE_IDLE;

    end else if (this_state==STATE_READ_BURST) begin

        if (this_counter[0]==1'b1 && this_counter<=14) begin
            next_addr = {3'b000, this_latched_address[10:5],this_col, 1'b0};
            next_cmd  = CMD_READ;
            next_col  = this_col + 1'b1;
        end 

        if (this_counter>=0 && this_counter<=15) 
            next_dqm  = 2'b0;
    
        if (this_counter==7'd19) 
            next_state = STATE_IDLE;

        if (this_counter==7'd18) 
            next_complete = this_master;

        if (this_counter[0]==1'b0 && this_counter>=4 && this_counter<=18)
            next_valid = this_master;

    end else if (this_state==STATE_WRITE) begin
        next_dqm  = ~byte_en_delay;
        next_dq   = wdata_msb_delay;
        next_dqe  = 1'b1;
        next_state = STATE_IDLE;

    end else if (this_state==STATE_REFRESH) begin
        if (this_counter==2) begin
            next_addr = 13'h400;
            next_ba   = 2'b0;
            next_cmd  = CMD_PRECHARGE;
        end

        if (this_counter==4) begin
            next_cmd = CMD_REFRESH;
        end
        if (this_counter==10) begin
            next_state = STATE_IDLE;
        end

    end else if (this_state==STATE_ACTIVATE) begin
        next_state = STATE_IDLE;

    end else if (this_state==STATE_PRECHARGE) begin
        next_state = STATE_IDLE;
    end



    if (this_refresh_counter==700) begin
        next_refresh_needed = 1'b1;
        next_refresh_counter = 0;
    end
end

// maintain a copy of what state the sdram banks are in
always @(posedge clock) begin
    if (next_cmd==CMD_PRECHARGE && next_addr[10])
        bank_open = 4'h0;
    else if (next_cmd==CMD_PRECHARGE)
        bank_open[next_ba] = 1'b0;
    else if (next_cmd==CMD_ACT) begin
        bank_open[next_ba] = 1'b1;
        bank_addr[next_ba] = next_addr;
    end
end



always @(posedge clock) begin

//    if (this_state==STATE_IDLE && next_state==STATE_WRITE)
//        $display("TIME %t [%x]=%x %x", $time, sdram_address, sdram_wdata, sdram_byte_en);
    if (next_valid!=0)
        $display("READ DATA %d %x",next_valid,{DRAM_DQ,reg0_dq});

    this_counter <= next_counter;
    this_state   <= next_state;
    this_master  <= next_master;
    DRAM_ADDR    <= next_addr;
    DRAM_BA      <= next_ba;
    this_cmd     <= next_cmd;
    this_dq      <= next_dq;
    this_dqm     <= next_dqm;
    this_dqe     <= next_dqe;
    this_col     <= next_col;
    sdram_valid  <= next_valid;
    reg0_dq      <= DRAM_DQ;
    reg1_dq      <= reg0_dq;
    sdram_complete <= next_complete;
    sdram_valid  <= next_valid;
    prev_writes  <= {this_cmd==CMD_WRITE,prev_writes[1]};
    this_latched_address <= next_latched_address;
    this_refresh_counter <= next_refresh_counter;
    this_refresh_needed  <= next_refresh_needed;
    wdata_msb_delay    <= sdram_wdata[31:16];
    byte_en_delay    <= sdram_byte_en[3:2];
end



endmodule