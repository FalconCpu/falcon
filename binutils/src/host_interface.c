#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <fcntl.h>
#include <conio.h> // For _kbhit and _getch
#include <errno.h>
// #include <unistd.h> // For fcntl on Windows

typedef const char* string;

#define COM_PORT "COM4"
#define BAUD_RATE 2000000

static HANDLE hSerial = INVALID_HANDLE_VALUE;

#define DISK_SECTOR_SIZE    1024
#define DISK_NUMBER_SECTORS 4096
static FILE* diskImageFile;

#define FILESYSPKT_OPEN     0x40B2B1B0 
#define FILESYSPKT_CLOSE    0x41B2B1B0 
#define FILESYSPKT_READ     0x42B2B1B0 
#define FILESYSPKT_WRITE    0x43B2B1B0 
#define FILESYSPKT_DATA     0x44B2B1B0 
#define FILESYSPKT_ERROR    0x45B2B1B0 
#define FILESYSPKT_READDIR  0x46B2B1B0 
#define FILESYSPKT_NEXTDIR  0x47B2B1B0 
#define FILESYSPKT_CLOSEDIR 0x48B2B1B0 



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

    dcbSerialParams.BaudRate = BAUD_RATE;    // Set your baud rate here
    dcbSerialParams.ByteSize = 8;            // 8-bit data
    dcbSerialParams.StopBits = TWOSTOPBITS;  // Two stop bit
    dcbSerialParams.Parity = NOPARITY;       // No parity
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

    return readBuffer[0] & 0xff;
}

/// -----------------------------------------------------------------
///                    read word from com port
/// -----------------------------------------------------------------

static int read_word_from_com_port() {
    int c1 = read_from_com_port();
    int c2 = read_from_com_port();
    int c3 = read_from_com_port();
    int c4 = read_from_com_port();
    if (c1==-1 || c2==-1 || c3==-1 || c4==-1) {
        printf("Error reading from com port\n");
        return -1;
    }
    return (c1&0xff) | ((c2&0xff)<<8) | ((c3&0xff)<<16) | ((c4&0xff)<<24);
}

/// -----------------------------------------------------------------
///                    read string from com port
/// -----------------------------------------------------------------

static int read_string_from_com_port(char *dest, int max_length) {
    int c;
    int i=0;
    while(1) {
        c = read_from_com_port();
        if (c==-1) {
            printf("Error reading from com port\n");
            return -1;
        }
        if (c==0)
            break;
        dest[i++] = c;
        if (i>=max_length)
            break;
    }
    dest[i] = 0;
    return i;
}


/// -----------------------------------------------------------------
///                    read buffer from com port
/// -----------------------------------------------------------------

static int read_buffer_from_com_port(char *dest, int max_length) {
    int c;
    int i=0;
    while(1) {
        c = read_from_com_port();
        if (c==-1) {
            printf("Error reading from com port\n");
            return -1;
        }
        dest[i++] = c;
        if (i>=max_length)
            break;
    }
    dest[i] = 0;
    return i;
}

/// -----------------------------------------------------------------
///                    send_buffer_to_com_port
/// -----------------------------------------------------------------

static void send_buffer_to_com_port(char* buffer, int length) {
    DWORD bytesWritten = 0;
    if (!WriteFile(hSerial, buffer, length, &bytesWritten, NULL))
        fatal("Error writing to serial port.\n");
}

/// -----------------------------------------------------------------
///                    send_file_to_com_port
/// -----------------------------------------------------------------
/// send the boot rom to the com port

static void send_file_to_com_port(char* file_name) {
    FILE *fh = fopen(file_name, "r");
    if (fh==0)
        fatal("Cannot open file '%s'", file_name);

    int buffer[16384];
    char line[100];
    int num_words;

    while( fgets(line, sizeof(line), fh) != NULL ) {
        sscanf(line, "%x", &buffer[num_words++]);
    }
    fclose(fh);

    if (num_words==0)
        fatal("No data in file '%s'", file_name);

    // send the start marker
    int start_marker = 0xB0;
    if (!WriteFile(hSerial, &start_marker, 1, NULL, NULL))
        fatal("Error sending data to serial port.\n");

    // Send the data
    DWORD bytesWritten = 0;
    if (!WriteFile(hSerial, buffer, num_words*4, &bytesWritten, NULL) || bytesWritten != num_words*4)
        fatal("Error sending program data");
}

/// -----------------------------------------------------------------
///                    print_host
/// -----------------------------------------------------------------
/// print a message, color coded for host

#define COLOR_RESET "\033[0m"
#define COLOR_FPGA "\033[1;32m"  // Bright Green for FPGA
#define COLOR_HOST "\033[1;34m"  // Bright Blue for Host

static void print_host(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(COLOR_HOST);
    vprintf(fmt, args);
    printf(COLOR_RESET);
    va_end(args);
}



/// -----------------------------------------------------------------
///                    handle open
/// -----------------------------------------------------------------

static void handle_open() {
    char file_name[100];

    char mode_string[4];

    int mode = read_word_from_com_port();
    int file_name_length = read_string_from_com_port(file_name, sizeof(file_name));
    if (file_name_length==-1) {
        print_host("Error reading file name");
        return;
    }

    mode_string[0] = mode & 0xff;
    mode_string[1] = 0;

    FILE* fh = fopen(file_name, mode_string);
    int packet[2];

    if (fh==0) {
        print_host("Error opening file '%s'\n", file_name);
        packet[0] = FILESYSPKT_ERROR;
        packet[1] = errno;
    } else {
        print_host("Opened file '%s' %p\n", file_name, fh);
        packet[0] = FILESYSPKT_DATA;
        packet[1] = (int) fh;
    }
    send_buffer_to_com_port((char*)packet, 8);
    return;

    int a = EISDIR;
}


/// -----------------------------------------------------------------
///                    handle read
/// -----------------------------------------------------------------

static void handle_read() {
    FILE* fh = (FILE*)read_word_from_com_port();
    int length = read_word_from_com_port();
    char* buffer = (char*)malloc(length+8);
    int num_read = fread(buffer+8, 1, length, fh);
    // print_host("Read %d bytes from file\n", num_read);

    ((int*)buffer)[0] = FILESYSPKT_DATA;
    ((int*)buffer)[1] = num_read;
    send_buffer_to_com_port(buffer, num_read+8);
    free(buffer);
}

/// -----------------------------------------------------------------
///                    handle write
/// -----------------------------------------------------------------

static void handle_write() {
    FILE* fh = (FILE*)read_word_from_com_port();
    int length = read_word_from_com_port();
    char* buffer = (char*)malloc(length+8);
    read_buffer_from_com_port(buffer, length);
    int num_written = fwrite(buffer, 1, length, fh);
    // print_host("Wrote %d bytes to file %p\n", num_written, fh);
    free(buffer);

    // Send response
    int packet[2];
    packet[0] = FILESYSPKT_DATA;
    packet[1] = num_written;
    send_buffer_to_com_port((char*)packet, 8);
}

/// -----------------------------------------------------------------
///                     handle_close
/// -----------------------------------------------------------------

static void handle_close() {
    FILE* fh = (FILE*)read_word_from_com_port();
    print_host("Closing file %p\n", fh);
    fclose(fh);
}


/// -----------------------------------------------------------------
///                    handleReadDirectory
/// -----------------------------------------------------------------

static void sendDirectoryEntry(HANDLE hFind, WIN32_FIND_DATA findFileData) {
    int response[64];
    int fileNameLength = strlen(findFileData.cFileName);
    response[0] = FILESYSPKT_DATA;
    response[1] = (int)hFind;
    response[2] = findFileData.nFileSizeLow;
    response[3] = findFileData.dwFileAttributes;
    response[4] = fileNameLength;
    strncpy((char*)&response[5], findFileData.cFileName, 100);
    // print_host("Sending directory entry: %s %d\n", findFileData.cFileName, fileNameLength);
    send_buffer_to_com_port((char*)response, fileNameLength+20);
}


static void handleReadDirectory() {
    char file_name[100];
    int file_name_length = read_string_from_com_port(file_name, sizeof(file_name));

    int response[64];


    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile("*", &findFileData);

    print_host("Opening directory '%s'\n", file_name);
    if (hFind == INVALID_HANDLE_VALUE) {
        response[0] = FILESYSPKT_ERROR;
        response[1] = ERROR_FILE_NOT_FOUND;
        send_buffer_to_com_port((char*)response, 8);
        return;
    }

    sendDirectoryEntry(hFind, findFileData);
}

/// -----------------------------------------------------------------
///                    handleNextDir
/// -----------------------------------------------------------------

static void handleNextDir() {
    int response[64];

    WIN32_FIND_DATA findFileData;
    HANDLE hFind = (HANDLE) read_word_from_com_port();

    int success = FindNextFile(hFind, &findFileData);

    if (success == 0) {
        int packet[2];
        packet[0] = FILESYSPKT_ERROR;
        packet[1] = ERROR_NO_MORE_FILES;
        send_buffer_to_com_port((char*)packet,8);
        return;
    } 

    sendDirectoryEntry(hFind, findFileData);
}

/// -----------------------------------------------------------------
///                    handleCloseDir
/// -----------------------------------------------------------------

static void handleCloseDir() {
    HANDLE hFind = (HANDLE) read_word_from_com_port();
    print_host("Closing directory handle %p\n", hFind);
    FindClose(hFind);
}

/// -----------------------------------------------------------------
///                    command_mode
/// -----------------------------------------------------------------
/// When the FPGA sends an 0xB0 byte we enter command mode.
/// All commands begin with the byte string 0xB0 0xB1 0xB2 with 
/// next byte being the command.
/// If we receive any violations of this we exit command mode back to
/// normal operation mode.

static void command_mode() {
    // The first 0xB0 has already been read by the time we get here

    int c1 = read_from_com_port();
    if (c1!=0xB1)
        return;

    int c2 = read_from_com_port();
    if (c2!=0xB2)
        return;

    int c3 = read_from_com_port();

    int c = (0xB0) | (c1<<8) | (c2<<16) | (c3<<24);
    switch(c) {
        case FILESYSPKT_OPEN:       handle_open();       break;
        case FILESYSPKT_CLOSE:      handle_close();      break;
        case FILESYSPKT_READ:       handle_read();       break;
        case FILESYSPKT_WRITE:      handle_write();      break;
        case FILESYSPKT_READDIR:    handleReadDirectory(); break;
        case FILESYSPKT_NEXTDIR:    handleNextDir();     break;
        case FILESYSPKT_CLOSEDIR:   handleCloseDir();    break;

        default:
            print_host("Unknown command %x\n", c);
            break;
    }
    return;
}


/// -----------------------------------------------------------------
///                    run_loop
/// -----------------------------------------------------------------

static void run_loop() {
    while(1) {
        int c = read_from_com_port();
        if (c==0xB0)
            command_mode();
        else if (c!=-1)
            printf("%c", c);
    }
}


/// -----------------------------------------------------------------
///                    main
/// -----------------------------------------------------------------

int main(int argc, char** argv) {
    open_com_port();
    send_file_to_com_port("asm.hex");
    run_loop();

    CloseHandle(hSerial);
}
