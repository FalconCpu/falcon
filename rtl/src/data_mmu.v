module data_mmu(
    input clock,
    input reset,
    input supervisor,

    // interface to the config bus
    input [31:0] cfg_segment_start,       // Start Address of segment to add
    input [31:0] cfg_segment_end,         // End address of segment to add
    input        cfg_segment_write_enable,// Write enable flag for the segment
    input        cfg_segment_add,         // Strobed to add this segment
    input        cfg_segment_clear,       // Clear all segments

    // interface to cpu bus
    input         cpu_request,
    input         cpu_write,
    input [31:0]  cpu_address,

    output reg    segment_fault
);

parameter NUM_SEGMENTS = 8;

reg [31:0] segment_start [0:NUM_SEGMENTS-1];
reg [31:0] segment_end [0:NUM_SEGMENTS-1];
reg        segment_write_enable [0:NUM_SEGMENTS-1];
reg [2:0]  ptr;             
reg        access_allowed;

integer i;

always @(*) begin
    access_allowed = 0;
    for(i = 0; i < NUM_SEGMENTS; i = i + 1) begin
        if (cpu_address >= segment_start[i] && 
            cpu_address < segment_end[i] && 
            (!cpu_write || segment_write_enable[i])) begin
            access_allowed = 1;
        end
    end

    // Fault if a request is made, the access isn't allowed, and it's not in supervisor mode
    segment_fault = cpu_request && !access_allowed && !supervisor;
end


always @(posedge clock) begin
    if (reset || cfg_segment_clear) begin
        for(i = 0; i < NUM_SEGMENTS; i = i + 1) begin
            segment_start[i] <= 32'hFFFFFFFF; // Initialize to invalid addresses
            segment_end[i]   <= 32'b0;
            segment_write_enable[i] <= 1'b0;
        end
        ptr <= 0; // Reset pointer
    end 
    else if (cfg_segment_add) begin
        if (ptr < NUM_SEGMENTS) begin
            segment_start[ptr] <= cfg_segment_start;
            segment_end[ptr] <= cfg_segment_end;
            segment_write_enable[ptr] <= cfg_segment_write_enable;
            ptr <= ptr + 1; // Increment pointer for next segment
        end
    end
end

endmodule
