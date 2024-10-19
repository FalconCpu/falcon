
module instruction_cache(
    input        clock,
    input        reset,

    // Connection to the CPU instruction bus
    input      [31:0] instr_addr,      // Address of instruction from CPU
    output reg [31:0] instr_data,      // Instruction to CPU
    output reg        instr_cache_miss,  // Signal to indicate instruction is valid (CPU stalls if low)
    output reg        instr_error,     // Signal an invalid address
    input             instr_flush,     // Cache flush signal (invalidate entire cache)

    // Connection to the data bus arbiter (Read only)
    output reg        icache_request,  // Request to memory bus
    output reg [25:0] icache_address,  // Memory address for fetch
    input  [31:0]     icache_rdata,    // Data from memory bus
    input             icache_valid,    // Indicates valid data from memory bus
    input             icache_complete, // Signals the end of a memory bus transaction

    // connection to the ROM - bypasses the cache
    input  [31:0]  rom_data

);

// Direct mapped cache, 16kB size
// Configured as a 512 lines of 32 bytes (8 words)
// Hence input address can be broken down into the following fields:-
//
// 109876 543210987654 321098765 43210
// aaaaaa tttttttttttt lllllllll ooooo
// a = area of memory, 000000 = RAM, 111111=ROM others=invalid
// t = address cache tag
// l = cache line
// o = offset into line

reg [31:0] cache_ram[0:4095];

// The cache tag field needs to match 12 bits of the address, plus one bit for valid
reg [12:0] tag_ram[0:511];

// Pipeline the instr addr, and the cache tag fetch
reg [31:0] instr_addr_q;
reg [12:0] cache_tag_q;
reg [31:0] cache_data_q;

// Registers to store the cache flush
reg [8:0] flush_line, next_flush_line;
reg       flushing, next_flushing;      // Set to indicate a cache flush is in progress
reg       fetching, next_fetching;      // Set to indicate a cache line fetch is in progress

reg         next_request;  // Request to memory bus
reg [25:0]  next_address;  // Memory address for fetch

reg [7:0] bit_valid;

always @(*) begin
    // Interface to the SDRAM
    if (icache_complete) begin
        next_fetching = 0;
        next_request = 0;
        next_address = 26'bx;
    end else if (icache_valid) begin
        next_request = 0;
        next_fetching = 1;
        next_address = {icache_address[25:5], icache_address[4:0]+4'd4};
    end else begin
        next_request = icache_request;
        next_fetching = fetching;
        next_address = icache_address;
    end

    // Interface to the CPU

    case(instr_addr_q[31:26])
        6'b000000: begin  // Address is to the RAM
            instr_error = 1'b0;
            if (flushing) begin
                instr_data = 32'bx;
                instr_cache_miss = 1'b1;
            end else if (icache_valid && icache_address==instr_addr_q[25:0]) begin
                // Pass data from SDRAM through to CPU directly
                instr_data = icache_rdata;
                instr_cache_miss = 1'b0;    
            end else if ((cache_tag_q[12] || bit_valid[instr_addr_q[4:2]]) && cache_tag_q[11:0]==instr_addr_q[25:14]) begin
                // Got a cache hit
                instr_data = cache_data_q;
                instr_cache_miss = 1'b0;
            end else if (next_fetching==1'b0) begin
                // Got a cache miss - request a fetch
                instr_data = 32'bx;
                instr_cache_miss = 1'b1;
                next_request = 1'b1;
                next_fetching = 1'b1;
                next_address = instr_addr_q[25:0];
            end else begin
                instr_data = 32'bx;
                instr_cache_miss = 1'b1;
            end
        end

        6'b111111: begin  // Address is to the ROM
            instr_data = rom_data;
            instr_error = 1'b0;
            instr_cache_miss = 1'b0;
        end

        default: begin  // Invalid address
            instr_data = 32'bx;
            instr_error = 1'b1;
            instr_cache_miss = 1'b0;
        end
    endcase

    // Flush state machine
    if (reset || instr_flush) begin
        next_flushing = 1'b1;
        next_flush_line = 0;
    end else if (flushing) begin
        next_flush_line = flush_line + 1'b1;
        next_flushing = (flush_line!=511);
    end else begin
        next_flush_line = 0;
        next_flushing = 0;
    end

    if (reset) begin
        next_fetching = 0;
        next_request = 0;
        next_address = 0;
    end


end


always @(posedge clock) begin
    if (flushing)
        tag_ram[flush_line] = 0;
    else if (icache_valid)
        tag_ram[ icache_address[13:5] ] = {icache_complete, icache_address[25:14] };

    if (icache_valid) 
        cache_ram [ icache_address[13:2] ] = icache_rdata;

    
    if (icache_valid)
        bit_valid[ icache_address[4:2]] = 1'b1;
    else if (next_fetching==0 || next_request)
        bit_valid = 0;

    instr_addr_q <= instr_addr;
    cache_tag_q  <= tag_ram[ instr_addr[13:5] ];
    cache_data_q <= cache_ram[ instr_addr[13:2] ];

    flush_line <= next_flush_line;
    flushing <= next_flushing;
    fetching <= next_fetching;
    icache_address <= next_address;
    icache_request <= next_request;


end


endmodule
