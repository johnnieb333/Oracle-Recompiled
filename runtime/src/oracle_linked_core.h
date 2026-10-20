/* Oracle US secret codec. C++ adaptation of ZoraSharp's US GameSecret,
 * RingSecret and Secret formats. Copyright (c) 2013-2018 Amy Nagle.
 * Modifications: 2026 C++ codec, strict parser, and native entry integration.
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * See oracle-linked-menu/THIRD_PARTY.txt and LGPL-3.0.txt. */
#ifndef ORACLE_LINKED_CORE_H
#define ORACLE_LINKED_CORE_H
#include "gbrt.h"
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>
#include <ctype.h>
namespace oracle_linked {
static const uint8_t cipher[48]={21,35,46,4,13,63,26,16,58,47,30,32,15,62,54,55,9,41,59,49,2,22,61,56,40,19,52,50,1,11,10,53,14,27,18,44,33,45,37,48,25,42,6,57,60,23,51,24};
static const char* const symbols[64]={"B","D","F","G","H","J","L","M","\xe2\x99\xa0","\xe2\x99\xa5","\xe2\x99\xa6","\xe2\x99\xa3","#","N","Q","R","S","T","W","Y","!","\xe2\x97\x8f","\xe2\x96\xb2","\xe2\x96\xa0","+","-","b","d","f","g","h","j","m","$","*","/",":","~","n","q","r","s","t","w","y","?","%","&","(","=",")","2","3","4","5","6","7","8","9","\xe2\x86\x91","\xe2\x86\x93","\xe2\x86\x90","\xe2\x86\x92","@"};
static const uint8_t native_symbols[64]={'B','D','F','G','H','J','L','M',0x13,0xbd,0x12,0x11,'#','N','Q','R','S','T','W','Y','!',0x10,0x7e,0x7f,'+','-','b','d','f','g','h','j','m','$','*','/',':','~','n','q','r','s','t','w','y','?','%','&','<','=','>','2','3','4','5','6','7','8','9',0x15,0x16,0x17,0x18,'@'};
struct Game {int id=1,target=0,animal=0x0b,behavior=0;char hero[6]="Link",child[6]="Pip";bool linked=true,hero_quest=false,free_ring=false;};
inline int bits(const std::vector<uint8_t>& d,int offset,int count){int v=0;for(int i=0;i<count;++i)if(d[(offset+i)/6]&(1u<<(5-(offset+i)%6)))v|=1u<<i;return v;}
inline void put(std::vector<uint8_t>& d,int offset,int count,int value){for(int i=0;i<count;++i){uint8_t mask=1u<<(5-(offset+i)%6);if(value&(1u<<i))d[(offset+i)/6]|=mask;else d[(offset+i)/6]&=~mask;}}
inline std::vector<uint8_t> encode(std::vector<uint8_t> d){int sum=0;for(uint8_t b:d)sum+=b;d.back()=sum&15;int key=d[0]>>3;for(size_t i=0;i<d.size();++i)d[i]^=cipher[key*4+i];d[0]=(d[0]&7)|(key<<3);return d;}
inline bool decode(const std::vector<uint8_t>& s,std::vector<uint8_t>& d,std::string& err){if(s.size()!=20&&s.size()!=15){err="Use a 20-symbol game secret or 15-symbol ring secret.";return false;}for(uint8_t b:s)if(b>63){err="Invalid secret symbol.";return false;}d=s;int key=s[0]>>3;for(size_t i=0;i<d.size();++i)d[i]^=cipher[key*4+i];d[0]=(d[0]&7)|(key<<3);int sum=0;for(size_t i=0;i+1<d.size();++i)sum+=d[i];if((sum&15)!=(d.back()&15)){err="Password checksum does not match. Nothing was changed.";return false;}if(bits(d,3,2)!=(s.size()==20?0:2)){err="Wrong secret type.";return false;}return true;}
inline bool valid_name(const char* n){size_t len=strlen(n);if(!len||len>5)return false;for(size_t i=0;i<len;++i){unsigned char c=n[i];if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c==' '||c=='-'||c=='\''))return false;}return true;}
inline bool game_secret(const Game& g,std::vector<uint8_t>& out,std::string& err){if(g.id<1||g.id>32767){err="Game ID must be 1-32767.";return false;}if(g.target<0||g.target>1||g.animal<0x0b||g.animal>0x0d||g.behavior<0||g.behavior>63){err="Invalid game, companion, or child behavior.";return false;}if(!valid_name(g.hero)||!valid_name(g.child)){err="Names need 1-5 English letters, digits, spaces, apostrophes or hyphens.";return false;}std::vector<uint8_t>d(20);int key=((g.id>>8)+(g.id&255))&7;put(d,0,3,key);put(d,5,15,g.id);put(d,20,1,g.hero_quest);put(d,21,1,g.target);const int hp[]={22,38,60,77,89},cp[]={30,46,68,97,106};for(int i=0;i<5;++i){put(d,hp[i],8,i<(int)strlen(g.hero)?(unsigned char)g.hero[i]:0);put(d,cp[i],8,i<(int)strlen(g.child)?(unsigned char)g.child[i]:0);}put(d,54,6,g.behavior);put(d,76,1,g.free_ring);put(d,85,4,g.animal);put(d,105,1,g.linked);out=encode(d);return true;}
inline bool ring_secret(int id,uint64_t rings,std::vector<uint8_t>& out,std::string& err){if(id<1||id>32767){err="Game ID must be 1-32767.";return false;}std::vector<uint8_t>d(15);put(d,0,3,((id>>8)+(id&255))&7);put(d,3,2,2);put(d,5,15,id);const int order[]={1,5,7,3,0,4,2,6};for(int i=0;i<8;++i)put(d,20+i*8,8,(rings>>(order[i]*8))&255);out=encode(d);return true;}
inline std::string format(const std::vector<uint8_t>& d,bool words=false){std::string s;for(size_t i=0;i<d.size();++i){if(i&&i%5==0)s+=' ';const char* c=symbols[d[i]];if(words){switch(d[i]){case 8:c="{spade}";break;case 9:c="{heart}";break;case 10:c="{diamond}";break;case 11:c="{club}";break;case 21:c="{circle}";break;case 22:c="{triangle}";break;case 23:c="{square}";break;case 59:c="{up}";break;case 60:c="{down}";break;case 61:c="{left}";break;case 62:c="{right}";break;}}s+=c;}return s;}
inline bool parse(const char* text,std::vector<uint8_t>& out,std::string& err){out.clear();if(!text||strlen(text)>2048){err="Password text is missing or too long.";return false;}const char* p=text;while(*p){if(isspace((unsigned char)*p)){++p;continue;}if(*p=='{'){const char* end=strchr(p,'}');if(!end){err="A symbol word is missing its closing brace.";return false;}std::string word(p+1,end);for(char& c:word)c=(char)tolower((unsigned char)c);int n=-1;const char* names[]={"spade","heart","diamond","club","circle","triangle","square","up","down","left","right"};const int ids[]={8,9,10,11,21,22,23,59,60,61,62};for(int i=0;i<11;++i)if(word==names[i])n=ids[i];if(n<0){err="Unknown symbol word.";return false;}out.push_back(n);p=end+1;}else{int found=-1;size_t length=0;for(int i=0;i<64;++i){size_t n=strlen(symbols[i]);if(!strncmp(p,symbols[i],n)){found=i;length=n;break;}}if(*p=='<'){found=48;length=1;}if(*p=='>'){found=50;length=1;}if(found<0){err="Unknown password character. Case matters; use Copy Password for generated codes.";return false;}out.push_back(found);p+=length;}if(out.size()>20){err="Too many password symbols.";return false;}}std::vector<uint8_t>d;return decode(out,d,err);}
inline uint8_t mem(const GBContext* c,int a){return c->wram[a-0xc000];}
/* Verified NEW-FILE secret screens. Native confirmation and validation stay
 * with the game; filling never changes SRAM or creates a game file. */
inline bool paste_ready(const GBContext* c,std::string& err,int target=0){
 if(target<0||target>1){err="Unsupported target game.";return false;}
 if(!c||!c->wram||!c->hram||!c->vram||!c->eram||c->eram_size<8192){err="No running game is available.";return false;}
 const int slot_offset=target==0?0x1a:0x18;
 const int secret_type=target==0?0xcc88:0xcca2;
 if(mem(c,0xc2ee)!=0||mem(c,0xcbb3)!=6||mem(c,0xcbb4)!=1||mem(c,0xcbb7)!=0x82||mem(c,0xcbb8)!=19||mem(c,0xcbcb)!=0||mem(c,secret_type)!=0||mem(c,0xc4ab)!=0||c->hram[slot_offset]>=3){
  err=target==0?"In Ages, choose Secrets for an empty game-file slot and wait for its 20-symbol entry screen.":"In Seasons, choose Secrets for an empty game-file slot and wait for its 20-symbol entry screen.";return false;
 }
 const int file=c->hram[slot_offset];const int a[]={0x10,0x560,0xab0},b[]={0x1000,0x1550,0x1aa0};
 const char* signature=target==0?"Z21216-0":"Z11216-0";
 if(!memcmp(c->eram+a[file]+2,signature,8)||!memcmp(c->eram+b[file]+2,signature,8)){err="That game-file slot contains a save. Choose an empty slot first.";return false;}
 return true;
}
inline bool fill_game_entry(GBContext* c,const std::vector<uint8_t>& secret,int target,std::string& err){
 std::vector<uint8_t>d;
 if(secret.size()!=20||!decode(secret,d,err)){if(secret.size()!=20)err="This fill button accepts 20-symbol game secrets only.";return false;}
 if(target<0||target>1||bits(d,21,1)!=target){err="The password targets the other game. Choose the matching password target and generate again.";return false;}
 if(!paste_ready(c,err,target))return false;
 // Bank 5 contains the native secret alphabet graphics (64 glyphs of 32 bytes).
 // Match native textInput_updateEntryCursor: buffer at bank 4:DC00 and VRAM bank 1:8C00.
 for(int i=0;i<20;++i){c->wram[0x47c0+i]=native_symbols[secret[i]];memcpy(c->wram+0x4c00+i*32,c->wram+0x5000+secret[i]*32,32);memcpy(c->vram+0x2000+0xc00+i*32,c->wram+0x5000+secret[i]*32,32);}memset(c->wram+0x47c0+20,0x20,12);c->wram[0xbbe]=19;return true;}
/* Seasons red-snake entry: 15 symbols, original in-game menu type 6. */
inline bool seasons_ring_ready(const GBContext* c,std::string& err){
 if(!c||!c->wram||!c->hram||!c->vram||!c->eram||c->eram_size<8192){err="No running game is available.";return false;}
 if(memcmp(c->wram+0x5b2,"Z11216-0",8)||mem(c,0xc611)!=0||c->hram[0x18]>=3||mem(c,0xc2ee)!=2||mem(c,0xc2ef)!=1||mem(c,0xcbcb)!=6||mem(c,0xcbb4)!=1||mem(c,0xcbb7)!=0x81||mem(c,0xcbb8)!=14||mem(c,0xcca2)!=2||mem(c,0xc4ab)!=0){
  err="In Seasons, ask the red snake to enter a ring secret and wait for its 15-symbol screen.";return false;
 }
 if(!(mem(c,0xc697)&0x10)||mem(c,0xc6c6)<1||mem(c,0xc6c6)>3){err="The loaded file needs its own ring box.";return false;}
 return true;
}
inline bool fill_seasons_ring_entry(GBContext* c,const std::vector<uint8_t>& secret,std::string& err){
 std::vector<uint8_t>d;
 if(secret.size()!=15||!decode(secret,d,err)){if(secret.size()!=15)err="Choose a 15-symbol ring password for this screen.";return false;}
 if(!seasons_ring_ready(c,err))return false;
 const int id=mem(c,0xc600)|(mem(c,0xc601)<<8);
 if(id!=bits(d,5,15)){err="Ring password Game ID differs from this file. Use Read current loaded file, then generate again.";return false;}
 for(int i=0;i<15;++i){
  c->wram[0x47c0+i]=native_symbols[secret[i]];
  memcpy(c->wram+0x4c00+i*32,c->wram+0x5000+secret[i]*32,32);
  memcpy(c->vram+0x2c00+i*32,c->wram+0x5000+secret[i]*32,32);
 }
 memset(c->wram+0x47c0+15,0x20,17);c->wram[0xbbe]=14;
 return true;
}
inline bool fill_ages_entry(GBContext* c,const std::vector<uint8_t>& secret,std::string& err){return fill_game_entry(c,secret,0,err);}
inline bool fill_seasons_entry(GBContext* c,const std::vector<uint8_t>& secret,std::string& err){return fill_game_entry(c,secret,1,err);}
}
#endif
