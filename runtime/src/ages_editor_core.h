#ifndef AGES_EDITOR_CORE_H
#define AGES_EDITOR_CORE_H
#include "gbrt.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
namespace ages_editor {
/* Verified against Stewmath/oracles-disasm, US Ages. Fixed WRAM bank only. */
struct Item { const char* name; uint8_t id; uint16_t level; uint8_t maximum; };
static const Item items[] = {
    {"Sword",5,0xc6b2,3},{"Shield",1,0xc6af,3},
    {"Boomerang",6,0,0},{"Seed Shooter",0x0f,0,0},
    {"Roc's Feather",0x17,0,0},{"Seed Satchel",0x19,0xc6b4,3},
    {"Harp of Ages",0x11,0,0},{"Switch Hook",0x0a,0xc6b6,2},
    {"Power Bracelet",0x16,0xc6b8,2},{"Shovel",0x15,0,0},
    {"Bombs",3,0,0},{"Bombchus",0x0d,0,0},{"Biggoron's Sword",0x0c,0,0},
    {"Flute (current companion)",0x0e,0,0},{"Cane of Somaria",4,0,0},
    {"Flippers",0x2e,0,0},{"Potion",0x2f,0,0},{"Mermaid Suit",0x4a,0,0}
};
static const int item_count = sizeof(items)/sizeof(items[0]);
inline uint8_t& mem(uint8_t* ram,int address) { return ram[address-0xc000]; }
inline uint8_t mem(const uint8_t* ram,int address) { return ram[address-0xc000]; }
inline bool flag(const uint8_t* r,int id) { return (mem(r,0xc69a+id/8)&(1u<<(id%8)))!=0; }
inline void setflag(uint8_t* r,int id,bool v) {
    uint8_t& b=mem(r,0xc69a+id/8); uint8_t m=(uint8_t)(1u<<(id%8));
    b=v ? (uint8_t)(b|m) : (uint8_t)(b&~m);
}
/* Require a real destination in both eras, so changing era cannot leave the
 * original warp menu searching an empty destination list. Do not invent visits. */
inline bool has_gale_destination(const uint8_t* r) {
    static const int present[]={0x13,0x78,0xc1};
    static const int past[]={0x08,0x25,0x2d,0x78,0x80,0xc1};
    bool now=(mem(r,0xc7ac)&0x90)==0x90; // planted scent tree AND visited
    bool then=false;
    for(int room:present)if(mem(r,0xc700+room)&0x10)now=true;
    for(int room:past)if(mem(r,0xc800+room)&0x10)then=true;
    return now&&then;
}
inline int bcd(int v) { return ((v>>4)&15)*10+(v&15); }
inline uint8_t to_bcd(int v) { return (uint8_t)(((v/10)<<4)|(v%10)); }
inline int capacity(int level) { static const int n[]={0,1,3,5}; return level>=0&&level<4?n[level]:0; }
struct Draft {
    uint8_t before[0x1000];
    bool owned[item_count]; int level[item_count];
    int rings[64]; /* 0=absent, 1=unappraised, 2=appraised/collected */
    int box_level, carried[5], active, songs, trade;
    uint8_t file_slot;
    int rupees, max_hearts, bomb_capacity, bombs, bombchus, seeds[5];
    bool refill_health, ammo_dirty[7];
};
inline const char* gameplay_block_reason(const GBContext* c) {
    if(!c||!c->wram||!c->hram)return "No running game is available.";
    const uint8_t* r=c->wram;
    if(memcmp(r+0x5b2,"Z21216-0",8)||mem(r,0xc611)!=1||c->hram[0x1a]>=3)
        return "No loaded Ages file was detected. Load your game first.";
    if(mem(r,0xc2ee)!=2)return "The game is loading a room or is outside normal gameplay.";
    if(mem(r,0xc2ef)!=1)return "A cutscene or room-initialization sequence is active.";
    if(mem(r,0xcc02)!=0)return "The game has disabled its menu during a transition.";
    if(mem(r,0xcbca)!=0)return "An item animation, collision lock, or scripted action is active.";
    /* Native updateMenus returns wOpenedMenuType. The load/active substates
     * are intentionally left behind when that menu closes. */
    if(mem(r,0xcbcb)!=0)return "An original in-game menu is open. Close it before editing.";
    if(mem(r,0xcba0)!=0)return "Dialogue or a text box is active. Finish it before editing.";
    if(mem(r,0xcc49)!=0)return "A warp is active. Wait until it finishes.";
    if(mem(r,0xcc4f)!=0)return "Link is in a forced action. Wait until it finishes.";
    if(mem(r,0xcd04)!=2||mem(r,0xcd00)!=1)return "A screen transition or scrolling lock is active.";
    return NULL;
}
inline bool active_game(const GBContext* c) { return gameplay_block_reason(c)==NULL; }
inline int ring_state(const uint8_t* r,int ring) {
    if (mem(r,0xc616+ring/8)&(1<<(ring%8))) return 2;
    for(int i=0;i<64;++i) if(mem(r,0xc5c0+i)!=0xff && (mem(r,0xc5c0+i)&63)==ring) return 1;
    return 0;
}
inline void capture(const GBContext* c,Draft& d) {
    memcpy(d.before,c->wram,sizeof(d.before));d.file_slot=c->hram[0x1a];
    for(int i=0;i<item_count;++i) {
        d.owned[i]=flag(c->wram,items[i].id);
        d.level[i]=items[i].level?mem(c->wram,items[i].level):0;
    }
    for(int i=0;i<64;++i)d.rings[i]=ring_state(c->wram,i);
    d.box_level=mem(c->wram,0xc6cc);d.active=mem(c->wram,0xc6cb);
    for(int i=0;i<5;++i)d.carried[i]=mem(c->wram,0xc6c6+i);
    d.songs=(mem(c->wram,0xc69e)>>5)&7;
    d.trade=flag(c->wram,0x41)?mem(c->wram,0xc6c0)+1:0;
    d.rupees=bcd(mem(c->wram,0xc6ad))+100*bcd(mem(c->wram,0xc6ae));
    d.max_hearts=mem(c->wram,0xc6ab)/4;
    d.refill_health=false;
    d.bomb_capacity=bcd(mem(c->wram,0xc6b1));
    d.bombs=bcd(mem(c->wram,0xc6b0));d.bombchus=bcd(mem(c->wram,0xc6b3));
    for(int i=0;i<5;++i)d.seeds[i]=bcd(mem(c->wram,0xc6b9+i));
    memset(d.ammo_dirty,0,sizeof(d.ammo_dirty));
}
inline void normalize_box(Draft& d) {
    int cap=capacity(d.box_level);
    for(int i=0;i<5;++i) {
        if(i>=cap || d.carried[i]<0 || d.carried[i]>=64 || d.rings[d.carried[i]]!=2) d.carried[i]=255;
        for(int j=0;j<i;++j)if(d.carried[i]!=255&&d.carried[i]==d.carried[j])d.carried[i]=255;
    }
    if(d.active!=255) {
        bool present=false;
        for(int i=0;i<cap;++i)if(d.carried[i]==(d.active&63))present=true;
        if(!present)d.active=255;
    }
}
inline bool build(const GBContext* c,const Draft& d,uint8_t out[0x1000],char* error,size_t n) {
#define FAIL(message) do { snprintf(error,n,"%s",message); return false; } while(0)
    if(!active_game(c))FAIL("Load a file and stand still outside dialogue, menus, or transitions.");
    if(c->hram[0x1a]!=d.file_slot || memcmp(c->wram+0x5b0,d.before+0x5b0,0x550))
        FAIL("The loaded file changed. Close and reopen the editor to refresh it.");
    memcpy(out,c->wram,0x1000);
    if(d.box_level<0||d.box_level>3||d.songs<0||d.songs>7)FAIL("Invalid ring box or harp tune selection.");
    for(int i=0;i<item_count;++i) {
        const Item& item=items[i];
        if(d.owned[i]&&item.level&&(d.level[i]<1||d.level[i]>item.maximum))FAIL("Choose a valid equipment level.");
        bool was=flag(d.before,item.id);
        if(was!=d.owned[i])setflag(out,item.id,d.owned[i]);
        if(item.id<0x20) {
            if(!d.owned[i]&&was) {
                for(int j=0;j<18;++j)if(mem(out,0xc688+j)==item.id)mem(out,0xc688+j)=0;
            } else if(d.owned[i]) {
                bool present=false;
                for(int j=0;j<18;++j)if(mem(out,0xc688+j)==item.id)present=true;
                if(!present) {
                    int free_slot=-1;
                    for(int j=2;j<18;++j)if(mem(out,0xc688+j)==0){free_slot=j;break;}
                    if(free_slot<0)for(int j=0;j<2;++j)if(mem(out,0xc688+j)==0){free_slot=j;break;}
                    if(free_slot<0)FAIL("Inventory is full. Remove another item first.");
                    mem(out,0xc688+free_slot)=item.id;
                }
            }
        }
        if(item.level && (d.owned[i] || was!=d.owned[i]))mem(out,item.level)=d.owned[i]?(uint8_t)d.level[i]:0;
        if(d.owned[i]&&!was&&item.id==3) {
            if(!mem(out,0xc6b1))mem(out,0xc6b1)=0x10;
            if(!mem(out,0xc6b0))mem(out,0xc6b0)=mem(out,0xc6b1);
        }
        if(d.owned[i]&&!was&&item.id==0x0d&&!mem(out,0xc6b3))mem(out,0xc6b3)=0x10;
        if(d.owned[i]&&!was&&item.id==0x19) {
            setflag(out,0x20,true);
            if(!mem(out,0xc6b9))mem(out,0xc6b9)=0x20;
        }
    }
    if(d.owned[6]) {
        if(!d.songs)FAIL("Choose at least one harp tune before enabling the Harp of Ages.");
        for(int i=0;i<3;++i)setflag(out,0x25+i,(d.songs&(1<<i))!=0);
        int selected=mem(out,0xc6b7);
        if(selected<1||selected>3||!(d.songs&(1<<(selected-1)))) {
            selected=1;while(!(d.songs&(1<<(selected-1))))++selected;
        }
        mem(out,0xc6b7)=(uint8_t)selected;
    } else if(flag(d.before,0x11)) {
        for(int i=0;i<3;++i)setflag(out,0x25+i,false);
        mem(out,0xc6b7)=0;
    }
    if(d.owned[17]&&!d.owned[15])FAIL("Enable Flippers before enabling the Mermaid Suit.");
    if((mem(c->wram,0xcc5d)||(mem(c->wram,0xcc34)&0x40))&&((flag(d.before,0x2e)&&!d.owned[15])||(flag(d.before,0x4a)&&!d.owned[17])))
        FAIL("Leave the water before removing swimming equipment.");
    if(flag(out,0x19)) {
        const int caps[]={0,20,50,99};int cap=caps[mem(out,0xc6b4)];
        for(int i=0;i<5;++i)if(bcd(mem(out,0xc6b9+i))>cap)mem(out,0xc6b9+i)=to_bcd(cap);
    }
    if(d.rupees<0||d.rupees>999)FAIL("Rupees must be between 0 and 999.");
    if(d.max_hearts<3||d.max_hearts>16)FAIL("Heart containers must be between 3 and 16.");
    if(d.bomb_capacity!=10&&d.bomb_capacity!=30&&d.bomb_capacity!=50)
        FAIL("Choose a bomb capacity of 10, 30, or 50.");
    if(d.bombs<0||d.bombs>99||d.bombchus<0||d.bombchus>99)FAIL("Ammo amounts must be between 0 and 99.");
    int old_money=bcd(mem(d.before,0xc6ad))+100*bcd(mem(d.before,0xc6ae));
    if(d.rupees!=old_money) {
        mem(out,0xc6ad)=to_bcd(d.rupees%100);mem(out,0xc6ae)=to_bcd(d.rupees/100);
        /* Ages has no ore display selector. */
        mem(out,0xcbe5)=mem(out,0xc6ad);mem(out,0xcbe6)=mem(out,0xc6ae);
    }
    if(d.max_hearts*4!=mem(d.before,0xc6ab)||d.refill_health) {
        mem(out,0xc6ab)=(uint8_t)(d.max_hearts*4);
        if(d.refill_health||mem(out,0xc6aa)>mem(out,0xc6ab))mem(out,0xc6aa)=mem(out,0xc6ab);
        mem(out,0xcbe4)=mem(out,0xc6aa);
    }
    mem(out,0xc6b1)=to_bcd(d.bomb_capacity);
    if(d.ammo_dirty[0]||d.bombs!=bcd(mem(d.before,0xc6b0))) {
        if(d.bombs>0&&!d.owned[10])FAIL("Enable Bombs in Equipment before adding bomb ammo.");
        mem(out,0xc6b0)=to_bcd(d.bombs>d.bomb_capacity?d.bomb_capacity:d.bombs);
    }
    if(bcd(mem(out,0xc6b0))>d.bomb_capacity)mem(out,0xc6b0)=to_bcd(d.bomb_capacity);
    if(d.ammo_dirty[1]||d.bombchus!=bcd(mem(d.before,0xc6b3))) {
        if(d.bombchus>0&&!d.owned[11])FAIL("Enable Bombchus in Equipment before adding their ammo.");
        mem(out,0xc6b3)=to_bcd(d.bombchus);
    }
    const int seed_caps[]={0,20,50,99};
    const int seed_cap=d.owned[5]?seed_caps[d.level[5]]:0;
    for(int i=0;i<5;++i) {
        if(d.seeds[i]<0||d.seeds[i]>99)FAIL("Seed amounts must be between 0 and 99.");
        if(d.ammo_dirty[i+2]||d.seeds[i]!=bcd(mem(d.before,0xc6b9+i))) {
            if(i==3&&d.seeds[i]>0&&!has_gale_destination(c->wram))FAIL("Visit a Gale Seed warp tree in both eras before adding Gale Seeds. The original warp menu needs a destination.");
            if(d.seeds[i]>0&&!d.owned[5])FAIL("Enable a Seed Satchel before adding seeds.");
            int amount=d.seeds[i]>seed_cap?seed_cap:d.seeds[i];
            mem(out,0xc6b9+i)=to_bcd(amount);
            if(amount>0)setflag(out,0x20+i,true);
        }
    }
    bool changed_rings=false;
    for(int ring=0;ring<64;++ring) {
        if(d.rings[ring]<0||d.rings[ring]>2)FAIL("Invalid ring selection.");
        if(d.rings[ring]==ring_state(d.before,ring))continue;
        changed_rings=true;
        uint8_t& owned=mem(out,0xc616+ring/8);uint8_t bit=(uint8_t)(1<<(ring%8));
        owned=d.rings[ring]==2?(uint8_t)(owned|bit):(uint8_t)(owned&~bit);
        for(int j=0;j<64;++j) {
            uint8_t& entry=mem(out,0xc5c0+j);
            if(entry!=255&&(entry&63)==ring)entry=255;
        }
    }
    /* Free every changed ring first, so a full list can be edited in one Apply. */
    for(int ring=0;ring<64;++ring) {
        if(d.rings[ring]!=ring_state(d.before,ring) && d.rings[ring]==1) {
            int empty=-1;
            for(int j=0;j<64;++j)if(mem(out,0xc5c0+j)==255){empty=j;break;}
            if(empty<0)FAIL("The unappraised ring list is full.");
            mem(out,0xc5c0+empty)=(uint8_t)(ring|64);
        }
    }
    if(changed_rings) {
        int count=0;
        for(int j=0;j<64;++j)if(mem(out,0xc5c0+j)!=255)mem(out,0xc5c0+count++)=mem(out,0xc5c0+j);
        for(int j=count;j<64;++j)mem(out,0xc5c0+j)=255;
        mem(out,0xc6cd)=to_bcd(count);
        bool any=count>0;
        for(int j=0;j<8;++j)if(mem(out,0xc616+j))any=true;
        setflag(out,0x2d,any);
    }
    if(d.trade<0||d.trade>12)FAIL("Choose a valid trading item.");
    if(d.trade!=(flag(d.before,0x41)?mem(d.before,0xc6c0)+1:0)) {
        setflag(out,0x41,d.trade>0);
        if(d.trade>0)mem(out,0xc6c0)=(uint8_t)(d.trade-1);
    }
    if(d.active!=255 && (d.active<0||d.active>127))FAIL("Invalid active ring.");
    int cap=capacity(d.box_level);
    for(int i=0;i<5;++i) {
        int ring=i<cap?d.carried[i]:255;
        if(ring!=255 && (ring<0||ring>=64||d.rings[ring]!=2))FAIL("Only appraised rings can go in the ring box.");
        for(int j=0;j<i;++j)if(ring!=255&&ring==d.carried[j])FAIL("A ring cannot occupy two ring-box slots.");
        mem(out,0xc6c6+i)=(uint8_t)ring;
    }
    if(d.active!=255) {
        bool present=false;
        for(int i=0;i<cap;++i)if(d.carried[i]==(d.active&63))present=true;
        if(!present)FAIL("The equipped ring must be in the ring box.");
    }
    mem(out,0xc6cb)=(uint8_t)d.active;
    mem(out,0xc6cc)=(uint8_t)d.box_level;
    setflag(out,0x2c,d.box_level!=0);
    if(d.box_level)mem(out,0xc615)=1;
    /* Native pickup code refreshes A/B and counts with these two bits. */
    mem(out,0xcbe9)|=15; /* Items, counts, hearts, and rupees. */
    return true;
#undef FAIL
}
/* Native mode8 pickup code already keeps the greater weapon level. Only the
 * satchel uses increment mode2: cap its overflow without suppressing events. */
inline uint8_t cap_satchel_write(GBContext* c,uint8_t value) {
    if(c&&c->rom&&c->rom_size>0x150&&c->wram&&
       !memcmp(c->rom+0x134,"ZELDA NAYRU",11) &&
       !memcmp(c->wram+0x5b2,"Z21216-0",8) &&
       mem(c->wram,0xc6b4)==3&&value==4&&flag(c->wram,0x19))return 3;
    return value;
}
inline uint8_t cap_resource_write(GBContext* c,uint16_t address,uint8_t value) {
    if(!c||!c->rom||c->rom_size<=0x150||!c->wram||
       memcmp(c->rom+0x134,"ZELDA NAYRU",11)||memcmp(c->wram+0x5b2,"Z21216-0",8))return value;
    /* Native heart pickups add 4; bomb-capacity upgrades add BCD 20. */
    if(address==0xc6ab&&mem(c->wram,0xc6ab)==64&&value==68)return 64;
    if(address==0xc6b1&&mem(c->wram,0xc6b1)==0x50&&value==0x70)return 0x50;
    if(address==0xc6b0&&mem(c->wram,0xc6b1)==0x50&&value==0x70)return 0x50;
    return value;
}
}
#endif
