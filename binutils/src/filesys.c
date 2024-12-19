#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

typedef const char* string;

// *****************************************************************
//                        FalconFileSys
// *****************************************************************
// Provide a means of interacting with the Falcon's file system
// from the host side.
//
// The Falcon FPGA accesses a file on the host PC as if it were
// a disk drive - reading and writing sectors.
// 
// However the host PC does not need to do this - and can simply map
// the whole disk image into memory.

// -----------------------------------------------------------------
//                    disk structure
// -----------------------------------------------------------------
// Sector 0      : Boot sector
// Sector 1..N-1 : Bitmap 1 bit per sector - continues 
// Sector N      : Start of root directory

// -----------------------------------------------------------------
//                    directory entry
// -----------------------------------------------------------------
// Structure used to represent an file on the disk

typedef struct  {           // Total length = 64 bytes
    int  type;              // offset 0    see constants below
    int  date;              // offset 4
    int  permissions;       // offset 8
    int  length;            // offset 12   length of file in bytes
    int  direct;            // offset 16   handles files up to 1K
    int  indirect;          // offset 20   handles files up to 1K+256K
    int  double_indirect;   // offset 24   handles files up to 1K+256K+64M
    int  num_sectors;       // offset 28   counts number of sectors allocated to this file
    char name[32];          // offset 32
} DirectoryEntry;

DirectoryEntry* rootDirectory;

void fatal(string msg,...) __attribute__((noreturn));
void* my_malloc(size_t size);
#define assert(A) if (!(A)) fatal("Assertion failed: %s", #A);
#define divideRoundUp(a,b) (((a)+(b)-1)/(b))

#define SECTOR_SIZE 1024
#define NUM_SECTORS 8192

static unsigned char* diskImage;
int line_number = 0;


// values for the type field
#define DIRENT_FILE 0x46494C45
#define DIRENT_DIR  0x44495245

static DirectoryEntry* rootDirectory;

/// -----------------------------------------------------------------
///                    Allocate Sector
/// -----------------------------------------------------------------
/// Allocate a sector. 

static int allocateSector() {
    for(int i = 0; i < NUM_SECTORS; i++) {
        int addr = (1*SECTOR_SIZE) + i/8;   // bitmap is in sector 1, 8 bits per byte
        int bitIndex = i % 8;
        if ((diskImage[addr] & (1 << bitIndex)) == 0) {
            diskImage[addr] |= (1 << bitIndex);
            return i;
        }
    }
    fatal("No free sectors");
}

static void markSectorAllocated(int sector) {
    int addr = (1*SECTOR_SIZE) + sector/8;   // bitmap is in sector 1, 8 bits per byte
    int bitIndex = sector % 8;
    if ((diskImage[addr] & (1 << bitIndex)) == 0)
        diskImage[addr] |= (1 << bitIndex);
    else
        fatal("Sector %d already allocated", sector);
}


/// -----------------------------------------------------------------
///                    Free Sector
/// -----------------------------------------------------------------

static void freeSector(int sector) {
    int addr = (1*SECTOR_SIZE) + sector/8;   // bitmap is in sector 1, 8 bits per byte
    int bitIndex = sector % 8;
    if ((diskImage[addr] & (1 << bitIndex)) == 0)
        fatal("Sector %d already free", sector);
    diskImage[addr] &= ~(1 << bitIndex);
}

/// -----------------------------------------------------------------
///                    writeDiskImageFile
/// -----------------------------------------------------------------
/// Write the modified disk image back to disk.

static void writeDiskImageFile() {
    FILE* file = fopen("disk.img", "wb");
    if (file == NULL)
        fatal("Can't create disk image file");

    fwrite(diskImage, SECTOR_SIZE, NUM_SECTORS, file);
    fclose(file);
}

/// -----------------------------------------------------------------
///                    writeInt
/// -----------------------------------------------------------------
/// Write an int value into the disk image (Treats a sector as an array of int)

static void writeInt(int sectorNumber, int offsetIntoSector, int value) {
    if (sectorNumber < 0 || sectorNumber >= NUM_SECTORS || offsetIntoSector < 0 || offsetIntoSector > SECTOR_SIZE/sizeof(int))
        fatal("Invalid sector or offset");
    int* address = (int*)(diskImage + (sectorNumber*SECTOR_SIZE) + offsetIntoSector*sizeof(int));
    *address = value;
}

/// -----------------------------------------------------------------
///                    readInt
/// -----------------------------------------------------------------
/// Read an int value from the disk image (Treats a sector as an array of int)

static int readInt(int sectorNumber, int offsetIntoSector) {
    if (sectorNumber < 0 || sectorNumber >= NUM_SECTORS || offsetIntoSector < 0 || offsetIntoSector > SECTOR_SIZE/sizeof(int))
        fatal("Invalid sector or offset");
    int* address = (int*)(diskImage + (sectorNumber*SECTOR_SIZE) + offsetIntoSector*sizeof(int));
    return *address;
}

/// -----------------------------------------------------------------
///                    extendFileBySector
/// -----------------------------------------------------------------
/// Allocate a sector for the file and link it in the DirectoryEntry.
/// Handles direct, indirect, and double-indirect blocks transparently.

static void extendFileBySector(DirectoryEntry* file) {
    // First sector goes in the direct block
    if (file->num_sectors==0) {
        file->direct = allocateSector();
        file->num_sectors++;
        return;
    }

    int index = file->num_sectors - 1;  // exclude the direct sector

    // Allocate new index block if needed
    if (index==0) 
        // Allocate the first index block
        file->indirect = allocateSector();
    else if (index==256) {
        // Need to allocate the double-indirect block, and the first index block in it
        file->double_indirect = allocateSector();
        writeInt(file->double_indirect, (index/256)-1, allocateSector());
    } else if (index%256==0) {
        // Subsequent index blocks go in the double-indirect block
        writeInt(file->double_indirect, (index/256)-1, allocateSector());
    }
        
    // Find the index block to write to 
    int indexblock;
    if (index<256)
        indexblock = file->indirect;
    else
        indexblock = readInt(file->double_indirect, (index/256)-1);

    writeInt(indexblock, index%256, allocateSector());
    file->num_sectors++;
}

/// -----------------------------------------------------------------
///                    formatDiskImage
/// -----------------------------------------------------------------

static void formatDiskImage() {
    diskImage = my_malloc(SECTOR_SIZE * NUM_SECTORS);
    markSectorAllocated(0);    // boot sector
    int bitmapSize = divideRoundUp(NUM_SECTORS, SECTOR_SIZE*8);   // Calculate the number of sectors required for the bitmap
    for(int i = 0; i < bitmapSize; i++)
        markSectorAllocated(1+i);

    rootDirectory = (DirectoryEntry*)(diskImage);
    rootDirectory->type = DIRENT_DIR;
    rootDirectory->date = 0;
    rootDirectory->permissions = 0;
    rootDirectory->length = 0;
}

/// -----------------------------------------------------------------
///                    main
/// -----------------------------------------------------------------

int main(int argc, char** argv) {
    if (argc < 2)
        fatal("No arguments specified");

    if (!strcmp(argv[1],"format")) {
        formatDiskImage();
    } else {
        printf("Unrecognized command '%s'", argv[1]);
    }
}