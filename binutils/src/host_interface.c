#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <fcntl.h>
#include <conio.h> // For _kbhit and _getch
// #include <unistd.h> // For fcntl on Windows

typedef const char* string;

#define COM_PORT "COM4"
#define BAUD_RATE 2000000
#define OUT_FILE_NAME "out.bin"

static HANDLE hSerial = INVALID_HANDLE_VALUE;

#define DISK_SECTOR_SIZE    1024
#define DISK_NUMBER_SECTORS 4096
static FILE* diskImageFile;

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
///                    openDiskImageFile
/// -----------------------------------------------------
/// Open the disk image file

static void createDiskImageFile() {
    diskImageFile = fopen("diskImage.dat", "wb");
    if (diskImageFile == NULL)
        fatal("Can't open disk image file '%s'", "diskImage.dat");
    
    void* emptySector = calloc(1, DISK_SECTOR_SIZE);
    for (int i=0; i<DISK_NUMBER_SECTORS; i++)
        fwrite(emptySector, DISK_SECTOR_SIZE, 1, diskImageFile);
    free(emptySector);
    fclose(diskImageFile);
}


static void open_disk_image_file() {
    diskImageFile = fopen("diskImage.dat", "ab");
    if (diskImageFile == NULL)
        fatal("Can't open disk image file '%s'", OUT_FILE_NAME);
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
///                    run_loop
/// -----------------------------------------------------------------

static void run_loop() {
    while(1) {
        int c = read_from_com_port();
        if (c!=-1)
            printf("%c", c);
    }
}


/// -----------------------------------------------------------------
///                    main
/// -----------------------------------------------------------------

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    if (! strcmp(argv[1], "-createDiskImage")) {
        createDiskImageFile();
        return 0;
    }

    open_com_port();
    send_file_to_com_port(argv[1]);
    run_loop();

    CloseHandle(hSerial);
}
