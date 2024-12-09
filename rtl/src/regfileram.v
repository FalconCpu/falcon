module regfileram (
	input clock,
	input [31:0] data,
	input [4:0] rdaddress,
	input [4:0] wraddress,
	input wren,
	output [31:0] q);

reg [31:0] ram[31:0];

assign q = ram[rdaddress];

initial begin
	ram[0] = 0;
end

always @(posedge clock) begin
	if (wren) begin
		ram[wraddress] <= data;
	end
end

endmodule