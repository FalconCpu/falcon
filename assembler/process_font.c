#include <stdio.h>

int main() {
    FILE *f_in = fopen("font.txt","r");
    FILE* f_out[4];
    f_out[0] = fopen("font0.hex","w");
    f_out[1] = fopen("font1.hex","w");
    f_out[2] = fopen("font2.hex","w");
    f_out[3] = fopen("font3.hex","w");

    char buf[64];
    char title[64];
    for(int c=32; c<=126; c++) {
        fgets(title, 64, f_in);  // skip line for char name
        for(int k=0; k<12; k++) {
            fgets(buf, 64, f_in);
            int res=0;
            for(int x=0; x<=7; x++) 
                if (buf[x]=='X')
                    res |= 1<<x;
            fprintf(f_out[k&3], "%x\n", res);
        }
    }
}