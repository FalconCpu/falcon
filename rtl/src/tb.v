`timescale 1ns / 1ns

module tb;

  // Parameters

  //Ports
  reg AUD_ADCDAT;
  wire AUD_ADCLRCK;
  wire AUD_BCLK;
  wire AUD_DACDAT;
  wire AUD_DACLRCK;
  wire AUD_XCK;
  reg CLOCK_50;
  wire [12:0] DRAM_ADDR;
  wire [1:0] DRAM_BA;
  wire [2:0] DRAM_CMD;
  wire DRAM_CKE;
  wire DRAM_CLK;
  wire DRAM_CS_N;
  wire [15:0] DRAM_DQ;
  wire [1:0] DRAM_DQM;
  wire FPGA_I2C_SCLK;
  wire FPGA_I2C_SDAT;
  wire [6:0] HEX0;
  wire [6:0] HEX1;
  wire [6:0] HEX2;
  wire [6:0] HEX3;
  wire [6:0] HEX4;
  wire [6:0] HEX5;
  reg [3:0] KEY;
  wire [9:0] LEDR;
  wire PS2_CLK;
  wire PS2_CLK2;
  wire PS2_DAT;
  wire PS2_DAT2;
  reg [9:0] SW;
  wire VGA_BLANK_N;
  wire [7:0] VGA_B;
  wire VGA_CLK;
  wire [7:0] VGA_G;
  wire VGA_HS;
  wire [7:0] VGA_R;
  wire VGA_SYNC_N;
  wire VGA_VS;
  wire [35:0] GPIO_0;
  wire [35:0] GPIO_1;

  initial begin
    SW = 10'h14;
    KEY = 4'hf;
  end

  Falcon  Falcon_inst (
    .AUD_ADCDAT(AUD_ADCDAT),
    .AUD_ADCLRCK(AUD_ADCLRCK),
    .AUD_BCLK(AUD_BCLK),
    .AUD_DACDAT(AUD_DACDAT),
    .AUD_DACLRCK(AUD_DACLRCK),
    .AUD_XCK(AUD_XCK),
    .CLOCK_50(CLOCK_50),
    .DRAM_ADDR(DRAM_ADDR),
    .DRAM_BA(DRAM_BA),
    .DRAM_CAS_N(DRAM_CMD[1]),
    .DRAM_CKE(DRAM_CKE),
    .DRAM_CLK(DRAM_CLK),
    .DRAM_CS_N(DRAM_CS_N),
    .DRAM_DQ(DRAM_DQ),
    .DRAM_LDQM(DRAM_DQM[0]),
    .DRAM_RAS_N(DRAM_CMD[2]),
    .DRAM_UDQM(DRAM_DQM[1]),
    .DRAM_WE_N(DRAM_CMD[0]),
    .FPGA_I2C_SCLK(FPGA_I2C_SCLK),
    .FPGA_I2C_SDAT(FPGA_I2C_SDAT),
    .HEX0(HEX0),
    .HEX1(HEX1),
    .HEX2(HEX2),
    .HEX3(HEX3),
    .HEX4(HEX4),
    .HEX5(HEX5),
    .KEY(KEY),
    .LEDR(LEDR),
    .PS2_CLK(PS2_CLK),
    .PS2_CLK2(PS2_CLK2),
    .PS2_DAT(PS2_DAT),
    .PS2_DAT2(PS2_DAT2),
    .SW(SW),
    .VGA_BLANK_N(VGA_BLANK_N),
    .VGA_B(VGA_B),
    .VGA_CLK(VGA_CLK),
    .VGA_G(VGA_G),
    .VGA_HS(VGA_HS),
    .VGA_R(VGA_R),
    .VGA_SYNC_N(VGA_SYNC_N),
    .VGA_VS(VGA_VS),
    .GPIO_0(GPIO_0),
    .GPIO_1(GPIO_1)
  );

  micron_sdram i_sdram (
    .Dq(DRAM_DQ),
    .Addr(DRAM_ADDR),
    .Ba(DRAM_BA),
    .Clk(DRAM_CLK),
    .Cke(DRAM_CKE),
    .Cs_n(DRAM_CS_N),
    .Ras_n(DRAM_CMD[2]),
    .Cas_n(DRAM_CMD[1]),
    .We_n(DRAM_CMD[0]),
    .Dqm(DRAM_DQM));


//always #5  clk = ! clk ;

initial begin
    $dumpvars(5);   // was 5
    #2500000;
    $finish;
end

always begin
    CLOCK_50 = 1'b0;
    #10;
    CLOCK_50 = 1'b1;
    #10;
end

endmodule