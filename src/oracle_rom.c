#include "oracle_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t rol(uint32_t x,unsigned n){return (x<<n)|(x>>(32-n));}
void oracle_sha1(const uint8_t* bytes,size_t size,char hex[41]) {
    uint32_t h[5]={0x67452301u,0xefcdab89u,0x98badcfeu,0x10325476u,0xc3d2e1f0u};
    size_t blocks=size/64+1+((size%64)>=56);
    for(size_t block=0;block<blocks;++block){
        uint32_t w[80]={0};
        for(size_t j=0;j<64;++j){
            size_t p=block*64+j;uint8_t b=0;
            if(p<size)b=bytes[p];else if(p==size)b=0x80;
            if(p>=blocks*64-8)b=(uint8_t)((uint64_t)size*8>>(8*(blocks*64-1-p)));
            w[j/4]|=(uint32_t)b<<(24-8*(j%4));
        }
        for(int i=16;i<80;++i)w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4];
        for(int i=0;i<80;++i){
            uint32_t f,k;if(i<20){f=(b&c)|(~b&d);k=0x5a827999u;}
            else if(i<40){f=b^c^d;k=0x6ed9eba1u;}
            else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8f1bbcdcu;}
            else{f=b^c^d;k=0xca62c1d6u;}
            uint32_t t=rol(a,5)+f+e+k+w[i];e=d;d=c;c=rol(b,30);b=a;a=t;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;
    }
    for(int i=0;i<5;++i)snprintf(hex+i*8,9,"%08x",h[i]);
}
int oracle_read_rom(const char* path,const char* expected,uint8_t** data,size_t* size,char* error,size_t error_size){
    *data=NULL;*size=0;
    FILE*f=fopen(path,"rb");
    if(!f){snprintf(error,error_size,"Cannot open ROM: %s",path);return 0;}
    const size_t wanted=1048576;
    uint8_t* bytes=(uint8_t*)malloc(wanted);
    if(!bytes){fclose(f);snprintf(error,error_size,"Not enough memory to read ROM.");return 0;}
    size_t n=fread(bytes,1,wanted,f);int extra=fgetc(f);int failed=ferror(f);fclose(f);
    if(n!=wanted||extra!=EOF||failed){free(bytes);snprintf(error,error_size,"ROM must be exactly 1048576 bytes (1 MiB).");return 0;}
    char digest[41];oracle_sha1(bytes,n,digest);
    if(strcmp(digest,expected)){free(bytes);snprintf(error,error_size,"Unsupported ROM. SHA-1: %s\nRequired SHA-1: %s",digest,expected);return 0;}
    *data=bytes;*size=n;return 1;
}
