`timescale 1ns / 1ns

module cpu_regfile(
    input               clock,
    input               stall,

    input [4:0]         p2_reg_a,
    input [4:0]         p2_reg_b,
    input [31:0]        p2_const,
    input               p2_bypass_a3,
    input               p2_bypass_b3,
    input               p2_bypass_a4,
    input               p2_bypass_b4,
    input               p2_b_is_const,

    output reg [31:0]   p3_data_a,
    output reg [31:0]   p3_data_b,
    output reg [31:0]   p3_data_c,

    input [4:0]         p4_reg_d,
    input [31:0]        p3_out,
    input [31:0]        p4_out
);

wire wren_a = !stall && p4_reg_d != 0;
wire [31:0] ram_a_out;
regfileram  regfileram_A (
    .clock(clock),
    .data(p4_out),
    .rdaddress(p2_reg_a),
    .wraddress(p4_reg_d),
    .wren(wren_a),
    .q(ram_a_out)
  );

wire wren_b = !stall && p4_reg_d != 0;
wire [31:0] ram_b_out;
regfileram  regfileram_B (
    .clock(clock),
    .data(p4_out),
    .rdaddress(p2_reg_b),
    .wraddress(p4_reg_d),
    .wren(wren_b),
    .q(ram_b_out)
  );

reg [31:0] p2_data_a;
reg [31:0] p2_data_b;

integer file;

// synthesis translate_off
initial begin
    file = $fopen("rtl_reg.log", "w");
end
// synthesis translate_on

always @(*) begin
    
    if (p2_bypass_a3)
        p2_data_a = p3_out;
    else if (p2_bypass_a4)
        p2_data_a = p4_out;
    else 
        p2_data_a = ram_a_out;

    if (p2_bypass_b3)
        p2_data_b = p3_out;
    else if (p2_bypass_b4)
        p2_data_b = p4_out;
    else if (p2_b_is_const)
        p2_data_b = p2_const;
    else
        p2_data_b = ram_b_out;

end

always @(posedge clock) begin
    if (!stall) begin
        // synthesis translate_off
        if (p4_reg_d != 0 && $time>=50)
                $fwrite(file,"$%2d = %x\n", p4_reg_d, p4_out);
        // synthesis translate_on

        p3_data_a <= p2_data_a;
        p3_data_b <= p2_data_b;
        p3_data_c <= p2_const;
    end
end

endmodule