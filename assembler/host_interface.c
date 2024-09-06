#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <fcntl.h>
#include <conio.h> // For _kbhit and _getch
// #include <unistd.h> // For fcntl on Windows

typedef const char* string;

#define COM_PORT "COM3"
#define BAUD_RATE 2000000
#define OUT_FILE_NAME "out.bin"

static HANDLE hSerial = INVALID_HANDLE_VALUE;

const char* YELLOW = "\e[33m";
const char* RED    = "\e[31m";
const char* DEFAULT = "\e[0m";

#define COMMAND_WORD_BOOT 0xcefa07b0
#define COMMAND_WORD_WRITE 0x7e4107b0
#define COMMAND_WORD_READ 0xAD3E07B0
#define COMMAND_WORD_OK 0x70AC07B0
#define COMMAND_WORD_ERROR 0x01E007B0
#define COMMAND_WORD_DATA 0x7ADA07B0

#define NUM_DISK_BLOCKS 16
#define BLOCK_SIZE 512
int* diskImage;

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

    unsigned char readBuffer[1];
    DWORD bytesRead = 0;
    
    if (!ReadFile(hSerial, readBuffer, 1, &bytesRead, NULL))
        return -1;

    if (bytesRead==0)
        return -1;

    //printf("<%02x>\n", readBuffer[0]);
    return readBuffer[0];
}

/// -----------------------------------------------------------------
///                    read_command_word
/// -----------------------------------------------------------------
/// Assume we have already seen the 0xB0 to initiate a command word

// static int read_command_word() {
//     int byte1 = read_from_com_port();
//     if (byte1 != 0x07)
//         return 0;

//     int byte2 = read_from_com_port();
//     int byte3 = read_from_com_port();
//     return 0xB0 | (byte1<<8) | (byte2<<16) | (byte3<<24);
// }




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
        fatal("%sCannot open file '%s'%s", RED, filename, DEFAULT);

    int data;
    int num_bytes;

    DWORD bytesWritten;

    data = 0xB007C0DE;
    // data[0]=0xde;
    // data[1]=0xc0;
    // data[2]=0x07;
    // data[3]=0xB0;
    WriteFile(hSerial, &data, 4, &bytesWritten, NULL);

    int crc = 0;

    while ((num_bytes = fread(&data, sizeof(char), 4, fh)) > 0 ) {
        //int word = data[0] | (data[1]<<8) | (data[2]<<16) | (data[3]<<24);
        crc = crc*31 + data;


        if (!WriteFile(hSerial, &data, num_bytes, &bytesWritten, NULL)) {
            fclose(fh);
            fatal("%sError writing to serial port.%s", RED, DEFAULT);
        }

        // printf("Writing %x\n", data);
        data = 0;
    }

    // data[0] = (crc>>0) & 0xff;
    // data[1] = (crc>>8) & 0xff;
    // data[2] = (crc>>16) & 0xff;
    // data[3] = (crc>>24) & 0xff;
    WriteFile(hSerial, &crc, 4, &bytesWritten, NULL);
    printf("CRC = %x\n",crc);

    fclose(fh);
}

/// -----------------------------------------------------------------
///                    initialize_memory_file
/// -----------------------------------------------------------------

void initialize_memory_file() {
    // lets make the disk 64Mb - 131072 blocks
    diskImage = calloc(1,NUM_DISK_BLOCKS * BLOCK_SIZE);
    if (diskImage==0)
        fatal("Cannot allocate memory for disk image");

    FILE *fh = fopen("diskImage.bin","rb");
    if (fh==0) {
        printf("%sNo disk image found - initialzing one%s\n", RED, DEFAULT);
        FILE *fh = fopen("diskImage.bin","wb");
        if (fh==0) 
            fatal("Cannot open file");
        fwrite(diskImage, BLOCK_SIZE, NUM_DISK_BLOCKS,  fh);
        fclose(fh);
    } else {
        printf("Reading disk image\n");
        fread(diskImage, BLOCK_SIZE, NUM_DISK_BLOCKS, fh);
        fclose(fh);
    }
}


/// -----------------------------------------------------------------
///                      read_word
/// -----------------------------------------------------------------

static int read_word() {
    int byte0 = read_from_com_port();
    int byte1 = read_from_com_port();
    int byte2 = read_from_com_port();
    int byte3 = read_from_com_port();

    return (byte0<<0) | (byte1<<8) | (byte2<<16) | (byte3<<24);
}

/// -----------------------------------------------------------------
///                      write_word
/// -----------------------------------------------------------------

static void write_word(int word) {
    unsigned long bytesWritten;
    WriteFile(hSerial, &word, 4, &bytesWritten, NULL);
}



/// -----------------------------------------------------------------
///                      write block
/// -----------------------------------------------------------------

static void write_block() {
    int block_number = read_word();
    if (block_number<0 || block_number>NUM_DISK_BLOCKS) {
        printf("%sInvalid block number%s\n",RED,DEFAULT);
        return;
    }

    int crc = block_number;
    printf("WRITE BLOCK %d\n", block_number);
    for(int k=0; k<128; k++) {
        int word = read_word();
        crc = 31*crc + word;
        diskImage[block_number*128 + k] = word;
        printf("%08X ", word);
        if ((k&7)==7)
            printf("\n");
    }
    int rxcrc = read_word();
    printf("RXCRC=%08x CALC_CRC=%08x   %s\n", rxcrc, crc, (rxcrc==crc)?"MATCH":"ERROR");

    if (rxcrc==crc)
        write_word(COMMAND_WORD_OK);
    else
        write_word(COMMAND_WORD_ERROR);  


    FILE *fh = fopen("diskImage.bin","rb+");
    if (fh==0)
        fatal("Cannot open file");
    fseek(fh, BLOCK_SIZE*block_number, SEEK_SET);
    fwrite(diskImage+ 128*block_number, BLOCK_SIZE, 1,  fh);
    fclose(fh);

}

/// -----------------------------------------------------------------
///                      read block
/// -----------------------------------------------------------------

static void read_block() {
    int block_number = read_word();
    printf("READ BLOCK %d\n", block_number);

    write_word(COMMAND_WORD_DATA);
    int crc = 0;
    for(int k=0; k<128; k++) {
        int data = diskImage[128*block_number + k];
        write_word(data);
        crc = crc*31 + data;
    }
    write_word(crc);
}

/// -----------------------------------------------------------------
///                      uart_command
/// -----------------------------------------------------------------
/// Received a high byte - start of a command

void uart_command(int byte0) {
    int byte1= read_from_com_port();
    // if (byte1!=0x07)
    //     return;

    int byte2= read_from_com_port();
    int byte3= read_from_com_port();
    unsigned int word = (byte0) | (byte1<<8) | (byte2<<16) | (byte3<<24);

    printf("%sReceived command %x  - ", YELLOW, word);
    if (word==COMMAND_WORD_BOOT) {
        printf("Sending boot image\n");
        send_file_to_com_port(0);
    } else if (word==COMMAND_WORD_READ) {
        read_block();
    } else if (word==COMMAND_WORD_WRITE)
        write_block();
    else
        printf("%sUnknown command %x\n", RED, word);

    printf("%s",DEFAULT);
}


/// -----------------------------------------------------------------
///                      main_loop
/// -----------------------------------------------------------------
// copy data from COM port to terminal,
// Terminate on null char
// If we receive an ESC char then send a copy of the ROM file

void main_loop() {
    printf("Opened serial port\n");

    // drain anything in the pipe
    while(read_from_com_port()==-1)
        ;


    while(1) {
        int com_data = read_from_com_port();
        if (com_data==-1)
            ;
        else if (com_data==0xB0 || com_data==0xCE)
            uart_command(com_data);
        else
            printf("%c", com_data);
    }

    // if (_kbhit()) {
    //     int c = _getch();
    //     //printf("Sending ");
    //     WriteFile(hSerial, &c, 1, 0, NULL);
    // }
}

/// -----------------------------------------------------------------
///                    main
/// -----------------------------------------------------------------

int main() {
    initialize_memory_file();
    open_com_port();

    // int fd = fileno(stdin); // Get file descriptor for standard input
    // int flags = fcntl(fd, F_GETFL); // Get current file flags
    // fcntl(fd, F_SETFL, flags | O_NONBLOCK); // Set non-blocking mode


    main_loop();

    CloseHandle(hSerial);
}