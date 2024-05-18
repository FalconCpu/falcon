#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <fcntl.h>
#include <conio.h> // For _kbhit and _getch
// #include <unistd.h> // For fcntl on Windows

typedef const char* string;

#define COM_PORT "COM5"
#define BAUD_RATE 2000000
#define OUT_FILE_NAME "out.bin"

static HANDLE hSerial = INVALID_HANDLE_VALUE;

/// -----------------------------------------------------
///                       fatal
/// -----------------------------------------------------
/// Report an error and exit the program

static void fatal(string message, ...) {
    va_list va;
    va_start(va, message);
    printf("FATAL: ");
    vprintf(message, va);
    printf("\n");

    if (hSerial != INVALID_HANDLE_VALUE) 
        CloseHandle(hSerial);
    exit(20);
}

/// -----------------------------------------------------
///                       open_com_port
/// -----------------------------------------------------

static void open_com_port() {
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts = {0};

    // Open the serial port
    hSerial = CreateFile(COM_PORT, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hSerial == INVALID_HANDLE_VALUE) 
        fatal("Error opening serial port.");

    // Set serial port parameters
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams))
        fatal("Error getting serial port state.\n");

    dcbSerialParams.BaudRate = BAUD_RATE; // Set your baud rate here
    dcbSerialParams.ByteSize = 8; // 8-bit data
    dcbSerialParams.StopBits = TWOSTOPBITS; // One stop bit
    dcbSerialParams.Parity = NOPARITY; // No parity
    if (!SetCommState(hSerial, &dcbSerialParams))
        fatal("Error setting serial port state.\n");

    // Set timeouts
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(hSerial, &timeouts))
        fatal("Error setting timeouts.\n");
}

/// -----------------------------------------------------------------
///                    read_from_com_port
/// -----------------------------------------------------------------
/// Reads a byte from the com port, waits until data is availible
/// Returns the byte read, or -1 for an error

static int read_from_com_port() {

    char readBuffer[1];
    DWORD bytesRead = 0;
    
    if (!ReadFile(hSerial, readBuffer, 1, &bytesRead, NULL))
        return -1;

    if (bytesRead==0)
        return -1;

    return readBuffer[0];
}

/// -----------------------------------------------------------------
///                    send_file_to_com_port
/// -----------------------------------------------------------------

static void send_file_to_com_port(int file_number) {
    char filename[64];
    if (file_number==0)
        sprintf(filename,"%s",OUT_FILE_NAME);
    else
        sprintf(filename,"file%d.bin",file_number);


    FILE *fh = fopen(filename, "rb");
    if (fh==0)
        fatal("Cannot open file '%s'", filename);

    char data[256];
    int num_bytes;

    DWORD bytesWritten;

    while ((num_bytes = fread(&data, sizeof(char), 256, fh)) > 0 ) {
        if (!WriteFile(hSerial, &data, num_bytes, &bytesWritten, NULL)) {
            fclose(fh);
            fatal("Error writing to serial port.");
        }
    }

    fclose(fh);
}


/// -----------------------------------------------------------------
///                      main_loop
/// -----------------------------------------------------------------
// copy data from COM port to terminal,
// Terminate on null char
// If we receive an ESC char then send a copy of the ROM file

void main_loop() {
    printf("Opened serial port\n");
    while(1) {
        int com_data = read_from_com_port();
        switch(com_data) {
            case '\0':
                printf("Received NULL\n");
                send_file_to_com_port(0);
                break;

            case 1:
                com_data = read_from_com_port();
                printf("Received 1:%d\n", com_data);
                send_file_to_com_port(com_data);
                break;


            case -1:
                break;

            case 27:
                printf("Sending file to device\n");
                send_file_to_com_port(0);
                break;

            default:
                printf("%c", com_data);
        }

        if (_kbhit()) {
            int c = _getch();
            //printf("Sending ");
            WriteFile(hSerial, &c, 1, 0, NULL);
        }
    }
}

/// -----------------------------------------------------------------
///                    main
/// -----------------------------------------------------------------

int main() {
    open_com_port();

    // int fd = fileno(stdin); // Get file descriptor for standard input
    // int flags = fcntl(fd, F_GETFL); // Get current file flags
    // fcntl(fd, F_SETFL, flags | O_NONBLOCK); // Set non-blocking mode


    main_loop();

    CloseHandle(hSerial);
}