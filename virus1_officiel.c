#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>


int main(int argc, char **argv){
    FILE *r = fopen("/dev/random","rb");
    char n[8];
    snprintf(n,8,"f%05d", fgetc(r));
    FILE *s = fopen(argv[0], "rb"), *d = fopen(n,"wb");
    int a;
    char buffer[128];
    while(a = fread(buffer,1,128,s)) fwrite(buffer,1,a,d);
    fclose(r); fclose(s); fclose(d);
}