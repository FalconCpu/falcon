#include <stdio.h>

int main() {
    FILE *f_in = fopen("font.txt","r");
    char buf[64];
    char title[64];
    for(int c=32; c<=126; c++) {
        fgets(title, 64, f_in);  // skip line for char name
        printf("dcb ");
        for(int k=0; k<12; k++) {
            fgets(buf, 64, f_in);
            int res=0;
            for(int x=0; x<=7; x++) 
                if (buf[x]=='X')
                    res |= 1<<x;
            if (k!=0)
                printf(",");
            printf("0%xH", res);
        }
        printf(" ; %s",title);
    }
}