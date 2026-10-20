#ifndef SEASONS_EDITOR_ROM_H
#define SEASONS_EDITOR_ROM_H
#include <vector>
#include <string>
#include <stdint.h>
namespace seasons_editor {
inline uint32_t rotate(uint32_t n,unsigned b) { return (n<<b)|(n>>(32-b)); }
inline std::string sha1(const uint8_t* input,size_t length) {
    std::vector<uint8_t> data(input,input+length);
    data.push_back(0x80);
    while(data.size()%64!=56)data.push_back(0);
    uint64_t bits=(uint64_t)length*8;
    for(int i=7;i>=0;--i)data.push_back((uint8_t)(bits>>(8*i)));
    uint32_t h[]={0x67452301u,0xefcdab89u,0x98badcfeu,0x10325476u,0xc3d2e1f0u};
    for(size_t p=0;p<data.size();p+=64) {
        uint32_t w[80];
        for(int i=0;i<16;++i) {
            w[i]=0;for(int j=0;j<4;++j)w[i]=(w[i]<<8)|data[p+i*4+j];
        }
        for(int i=16;i<80;++i)w[i]=rotate(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4];
        for(int i=0;i<80;++i) {
            uint32_t f,k;
            if(i<20){f=(b&c)|(~b&d);k=0x5a827999u;}
            else if(i<40){f=b^c^d;k=0x6ed9eba1u;}
            else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8f1bbcdcu;}
            else{f=b^c^d;k=0xca62c1d6u;}
            uint32_t t=rotate(a,5)+f+e+k+w[i];e=d;d=c;c=rotate(b,30);b=a;a=t;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;
    }
    static const char* hex="0123456789abcdef";std::string result;
    for(int i=0;i<5;++i)for(int shift=28;shift>=0;shift-=4)result+=hex[(h[i]>>shift)&15];
    return result;
}
inline bool supported_rom(const GBContext* c) {
    if(!c||!c->rom||c->rom_size!=1024*1024)return false;
    static const uint8_t* cached=NULL;static size_t size=0;static bool supported=false;
    if(cached!=c->rom||size!=c->rom_size) {
        cached=c->rom;size=c->rom_size;
        supported=sha1(c->rom,c->rom_size)=="ba1268290fb2b1b70505d2d7b5825fc8a4816a4b";
    }
    return supported;
}
}
#endif
