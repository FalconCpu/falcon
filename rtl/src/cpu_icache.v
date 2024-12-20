`timescale 1ns / 1ns

// =======================================================
//                  F32 Instruction Cache
// =======================================================
// A 16kB instruction cache formed as a direct-mapped cache 
// with 32 byte cache lines.

module cpu_icache(
    input               clock,  
    input               reset,      
    input               stall,              // The CPU is stalled - hold output stable

    // connections from the instruction decoder
    input      [31:0]   inst_address,
    output reg [31:0]   inst_rdata,
    output reg          inst_valid,
    output reg          inst_invalid_address,

    // connection to the instruction ROM
    output     [15:0]   imem_address,
    input      [31:0]   imem_rdata,

    // connections to the memory bus
    output reg          icache_request,     // request to the memory system
    output reg [25:0]   icache_address,     // Address to read from
    input      [31:0]   icache_rdata,       // data read from sdram
    input               icache_ack,         // transaction is acknowledged 
    input               icache_valid,       // data is valid
    input               icache_complete);   // transaction is complete

// Address format:
// 109876543210987654 321098765 432 10
// TTTTTTTTTTTTTTTTTT LLLLLLLLL OOO BB  
// T = Tag            18 bits
// L = Line number     9 bits
// O = Offset          3 bits
// B = Byte number     2 bits

reg [31:0]  cache_ram[0:4095];
reg [18:0]  tag_ram[0:511];           // Bit [18]=Valid, [17:0]=Tag

reg this_imem, next_imem;              // Is the address in the instruction memory
reg this_cacheable, next_cacheable;    // Is the address in cachable range
reg this_stalled;                      // The CPU is stalled


reg [31:0] this_data;             // Data fetched from the cache
reg [18:0] this_tag;              // Tag of the cache line
reg [31:0] this_address;          // Address of data we are outputting

reg [31:0] fetch_address;         // Address of the next instruction to fetch
reg        next_request;
wire [4:0]  addr_inc = fetch_address[4:0]+5'h4; // Calculate the next address in the line (wrap around)

reg cache_hit, cache_miss;
reg [31:0] stall_data; // If the cpu is stalled, this is the instruction we are holding
reg        stall_valid;// Data was valid when the CPU was stalled   


reg [2:0] this_state, next_state;
parameter STATE_IDLE    = 3'b000;
parameter STATE_REQUEST = 3'b001;
parameter STATE_FETCH   = 3'b010;
parameter STATE_WAIT    = 3'b011;
parameter STATE_RESET   = 3'b100;

assign imem_address = inst_address[15:0];

always @(*) begin
    cache_hit  = this_cacheable && this_tag[18] && (this_tag[17:0] == this_address[31:14]);
    cache_miss = this_cacheable && !cache_hit;

    // State machine to fetch instructions from the memory
    next_state = this_state;
    next_request = icache_request;
    case (this_state) 
        STATE_IDLE: begin
            next_request = cache_miss;
            icache_address = this_address[25:0];
            if (cache_miss)
                next_state = STATE_REQUEST;
        end

        STATE_REQUEST: begin
            next_request = ! icache_ack;
            icache_address = fetch_address[25:0];
            if (icache_ack)
                next_state = STATE_FETCH;
        end

        STATE_FETCH: begin
            next_request = 1'b0;
            icache_address = fetch_address[25:0];
            if (icache_complete)
                next_state = STATE_WAIT;    // Allow time for the data to be written to the cache
        end

        STATE_WAIT: begin
            next_request = 1'b0;
            icache_address = fetch_address[25:0];
            next_state = STATE_IDLE;
        end

        STATE_RESET: begin
            next_request = 1'b0;
            icache_address = 26'bx;
            if (fetch_address==14'h3fe0)
                next_state = STATE_IDLE;
        end
    endcase

    // Determine the data to be returned
    if (this_stalled) begin
        inst_rdata = stall_data;
        inst_valid = stall_valid;
    end else if (this_imem) begin
        inst_rdata = imem_rdata;
        inst_valid = 1'b1;
    end else if (this_cacheable && this_address==fetch_address && icache_valid) begin
        // Bypass the cache writeback for critical words
        inst_rdata = icache_rdata;
        inst_valid = 1'b1;
    end else if (cache_hit) begin
        inst_rdata = this_data;
        inst_valid = 1'b1;
    end else if (cache_miss) begin
        inst_rdata = 32'bx;
        inst_valid = 1'b0;
    end else begin
        $display("Warning: instruction cache invalid state at address %x time %d", inst_address,$time);
        inst_rdata = 32'bx;
        inst_valid = 1'b1;
    end

    if (reset) begin
        next_state <= STATE_RESET;
        next_request <= 1'b0;
        icache_address <= 26'b0;
    end
end 



always @(posedge clock) begin

    // Determine which region the address is in
    this_imem            <= (inst_address >= 32'hFFFF0000);
    this_cacheable       <= (inst_address < 32'h04000000);
    inst_invalid_address <= (inst_address>=32'h04000000 && inst_address<32'hFFFF0000);

    // preserve the state during stall
    this_stalled <= stall;
    if (stall && !this_stalled) begin
        stall_data <= inst_rdata;
        stall_valid <= inst_valid;
    end

    // Update the cache with the data from the memory
    if (icache_valid)
        cache_ram[fetch_address[13:2]] = icache_rdata;

    if (reset)
        fetch_address <= 32'h0;
    else if (this_state==STATE_RESET)
        fetch_address <= fetch_address + 32'h20;  // stepm through every cache line
    else if (this_state==STATE_IDLE)
        fetch_address <= icache_address;
    else if (icache_valid)
        fetch_address <= {fetch_address[31:5], addr_inc};
    
    if (this_state==STATE_RESET)
        tag_ram[fetch_address[13:5]] <= 18'h2xxxx;
    else if (icache_complete)
        tag_ram[fetch_address[13:5]] <= {1'b1,fetch_address[31:14]};
    else if (this_state==STATE_REQUEST||this_state==STATE_FETCH)
        tag_ram[fetch_address[13:5]] <= 18'h2xxxx;

    // fetch the instruction from the cache
    this_data      <= cache_ram[inst_address[13:2]];
    this_tag       <= tag_ram[inst_address[13:5]];
    this_address   <= inst_address;
    this_state     <= next_state;
    icache_request <= next_request;

end

endmodule