#include <stdio.h>
#include <stdlib.h>
#include "fpl.h"

// typedef unsigned long long Ull;

// struct Bitmap {
//     int numRows;
//     int numCols;
//     int rowShift;
//     Ull* data;
// };

// ====================================================================
//                       Constructor
// ====================================================================

Bitmap new_Bitmap(int num_rows, int num_cols) {
    Bitmap ret = new(struct Bitmap);
    ret->num_rows = num_rows;
    ret->num_cols = num_cols;

    #if __SIZEOF_LONG_LONG__ != 8
    #error "This compiler implementation requires ULL (unsigned long long) to be 64 bits. Please adjust the implementation or port to a compatible architecture."
    #endif

    ret->row_shift = 0;
    while(num_cols>=64) {
        ret->row_shift++;
        num_cols = num_cols >> 1;
    }

    ret->data = new_array(Ull, num_rows << ret->row_shift);
    return ret;
}

// ====================================================================
//                       Delete
// ====================================================================
void delete_Bitmap(Bitmap bm) {
    free(bm->data);
    free(bm);
}


// ====================================================================
//                       bitmap_set
// ====================================================================
// Set a bit in a bitmap
void   bitmap_set(Bitmap bm, int row, int col) {
    assert(row>=0 && row<bm->num_rows);
    assert(col>=0 && col<bm->num_cols);

    int addr = (row << bm->row_shift) + (col >> 6);
    int bit = col & 0x3f;
    bm->data[addr] |= 1L<<bit;
}

// ====================================================================
//                       bitmap_clear
// ====================================================================
// Set a bit in a bitmap
void   bitmap_clear(Bitmap bm, int row, int col) {
    assert(row>=0 && row<bm->num_rows);
    assert(col>=0 && col<bm->num_cols);

    int addr = (row << bm->row_shift) + (col >> 6);
    int bit = col & 0x3f;
    bm->data[addr] &= ~(1L<<bit);
}



// ====================================================================
//                       bitmap_get
// ====================================================================
// Get a bit from a bitmap
int   bitmap_get(Bitmap bm, int row, int col) {
    assert(row>=0 && row<bm->num_rows);
    assert(col>=0 && col<bm->num_cols);

    int addr = (row << bm->row_shift) + (col >> 6);
    int bit = col & 0x3f;
    return (bm->data[addr] >> bit) & 1;
}


// ====================================================================
//                       bitmap_or_row
// ====================================================================
// OR a row in one bitmap with a row in another bitmap
// Returns 1 if any new bits are set in the destination bitmap
int    bitmap_or_row(Bitmap dest_bm, int dest_row, Bitmap source_bm, int source_row) {
    assert(dest_bm->num_cols == source_bm->num_cols);
    assert(dest_row>=0 && dest_row<dest_bm->num_rows);
    assert(source_row>=0 && source_row<source_bm->num_rows);

    int dest_addr = (dest_row << dest_bm->row_shift);
    int source_addr = (source_row << source_bm->row_shift);
    int num_words = 1 << dest_bm->row_shift;

    int ret =0;
    for(int index=0; index<num_words; index++) {
        if (source_bm->data[source_addr+index] & ~dest_bm->data[dest_addr+index])
            ret = 1;

        dest_bm->data[dest_addr+index] |= source_bm->data[source_addr+index];
    }
    return ret;
}

// ====================================================================
//                       bitmap_or_and_not_row
// ====================================================================
// combines the data in a row of the destination bitmap with a row from bitmap A AND'ed with a row from bitmap B
void  bitmap_or_and_not_row(Bitmap dest_bm, int dest_row, Bitmap bm_a, int row_a, Bitmap bm_b, int row_b) {
    assert(dest_bm->num_cols == bm_a->num_cols);
    assert(dest_bm->num_cols == bm_b->num_cols);
    assert(dest_row>=0 && dest_row<dest_bm->num_rows);
    assert(row_a>=0 && row_a<bm_a->num_rows);
    assert(row_b>=0 && row_b<bm_b->num_rows);

    int dest_addr = (dest_row << dest_bm->row_shift);
    int a_addr = (row_a << bm_a->row_shift);
    int b_addr = (row_b << bm_b->row_shift);
    int num_words = 1 << dest_bm->row_shift;

    for(int index=0; index<num_words; index++)
        dest_bm->data[dest_addr+index] |= bm_a->data[a_addr+index] & ~bm_b->data[b_addr+index];
}