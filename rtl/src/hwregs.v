`timescale 1ns/1ns

module hwregs(
    input          clock,
    input          reset,

    // connection to the data bus
    input            hwregs_request,
    input            hwregs_write,
    input   [15:0]   hwregs_address,
    input   [31:0]   hwregs_wdata,
    input   [3:0]    hwregs_byte_en,
    output reg       hwregs_ack,
    output reg[31:0] hwregs_rdata,

    // Connections to peripherals
    output reg[23:0]  seven_segment,
    output reg[7:0]   seven_segment_brightness
);

always @(posedge clock) begin
    hwregs_ack <= hwregs_request;
    hwregs_rdata <= 0;

    // Handle reset
    if (reset) begin
        seven_segment <= 0;
        seven_segment_brightness <= 0;
    end


end


endmodule