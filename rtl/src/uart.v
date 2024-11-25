`timescale 1ns/1ns

module uart (
    input            clock,       // 100Mhz clock
    input            reset,
    
    input            UART_RX,     // Connections to the UART pins
    output reg       UART_TX,

    output reg       rx_complete, // Pulsed to indicate a byte has been received from the channel
    output reg [7:0] rx_data,     // The data received

    input            tx_valid,    // Asserted to indicate data is ready to be transmitted
    input [7:0]      tx_data,     // The data to transmit
    output reg       tx_complete  // Pulsed to indicate transmisssion is complete
);

// Our clock rate is 10ns bit period
// UART runs max 2M baud ==> 50 clock cycles per bit

//parameter UI_COUNTER = 16'd10416;    // 9600 Baud
//parameter UI_COUNTER = 16'd868;      // 115200 Baud
parameter UI_COUNTER = 16'd50;         // 2000000 Baud

// receiver state machine
reg        rx_bit, prev_rx_bit;   // registered version of pin to avoid metastability
reg        rx_active;
reg [15:0] rx_timer;
reg [3:0]  rx_index;

// transmit
reg        tx_active;
reg [15:0] tx_timer;
reg [3:0]  tx_index;
wire [9:0] tx_message = {1'b1,tx_data,1'b0};

always @(posedge clock) begin
    rx_bit <= UART_RX;     // synchronize the input
    prev_rx_bit <= rx_bit;
    UART_TX <= 1'b1;
    rx_complete <= 1'b0;
    tx_complete <= 1'b0;

    // =========================================
    //                  RX
    // =========================================

    if (reset) begin
        rx_active <= 1'b0;
        rx_timer  <= UI_COUNTER;
        rx_index  <= 4'b0;

    // Check for 2 consrcutive '0' bits to start a frame, as a bit of a buffer against noise
    end else if (!rx_active && rx_bit==1'b0 && prev_rx_bit==1'b0) begin
        // Got start of a frame
        rx_active <= 1;
        rx_timer  <= UI_COUNTER/2'd2-1'b1;
        rx_index  <= 4'h0;
    
    end else if (rx_active) begin
        rx_timer  <= rx_timer - 1'b1;
        if (rx_timer==0) begin
            rx_timer <= UI_COUNTER-1'b1;
            rx_index <= rx_index + 1'b1;
            if (rx_index>=1 && rx_index<=8)
                rx_data[rx_index-1'b1] <= rx_bit;
            if (rx_index==4'h9) begin
                rx_complete <= 1'b1;
                rx_active <= 1'b0;
            end
        end
    end

    // =========================================
    //                  TX
    // =========================================

    if (reset) begin
        tx_active <= 1'b0;
        tx_timer <= UI_COUNTER-1'b1;
        tx_index <= 4'h0;
        UART_TX  <= 1'b1;
        

    end else if (!tx_active && tx_valid) begin
        tx_active <= 1'b1;
        tx_timer  <= UI_COUNTER-1'b1;
        tx_index  <= 4'h0;

    end else if (tx_active) begin
        UART_TX <= tx_message[tx_index];
        tx_timer <= tx_timer - 1'b1;
        if (tx_timer==0) begin
            tx_timer <= UI_COUNTER-1'b1;
            tx_index <= tx_index + 1'b1;
            
            // pulse tx_complete when we finish transmitting the actual message (start of the stop bit)
            // This gives the fifo time to update ready for the next data
            if (tx_index==8)
                tx_complete <= 1'b1;  

            // At the end of the stop bit go back to the inactive state waiting for the next data
            if (tx_index==9) begin
                tx_active <= 1'b0;
            end
        end
    end
end
endmodule