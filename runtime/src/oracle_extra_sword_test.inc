/* EXTRA_FLUTE_V1: isolated USA Seasons extra-item input.
 * A/B assignments and native key memory are never rewritten.
 * Each item has one shared input state, even if assigned to X and Y.
 */
#include <stdlib.h>
#ifndef ORACLE_INDEPENDENT_UNIT
#include "seasons_editor_rom.h"
#endif
extern "C" int menu_gui_extra_items_enabled(void);
extern "C" int menu_gui_extra_indicators_enabled(void);
extern "C" int menu_gui_get_extra_x_item(void);
extern "C" int menu_gui_get_extra_y_item(void);
extern "C" int menu_gui_get_extra_l_item(void);
extern "C" int menu_gui_get_extra_r_item(void);
namespace oracle_sword_test {
struct Binding {bool raw,held,blocked;uint8_t item;};
struct ItemInput {uint8_t item,mask;bool held,request,pressed;};
static GBContext* owner=NULL;
static Binding bindings[4]={{false,false,true,0},{false,false,true,0},{false,false,true,0},{false,false,true,0}};
static ItemInput inputs[15]={{5,4,false,false,false},{0x17,8,false,false,false},{6,16,false,false,false},{1,32,false,false,false},{0x16,64,false,false,false},{3,128,false,false,false},{0x15,4,false,false,false},{0x19,8,false,false,false},{0x13,16,false,false,false},{0x0d,32,false,false,false},{0x0c,64,false,false,false},{0x1e,128,false,false,false},{7,4,false,false,false},{8,8,false,false,false},{0x0e,16,false,false,false}};
static bool eligible=false,query=false;
static uint16_t eligible_sp=0,return_sp=0;
static uint8_t query_item=0,query_mask=0;
static bool valid_item(int item){return item==5||item==0x17||item==6||item==1||item==0x16||item==3||item==0x15||item==0x19||item==0x13||item==0x0d||item==0x0c||item==0x1e||item==7||item==8||item==0x0e;}
static ItemInput* input_for(int item){for(int i=0;i<15;++i)if(inputs[i].item==item)return &inputs[i];return NULL;}
static bool enabled_for(GBContext* ctx){
    const bool enabled = menu_gui_extra_items_enabled() != 0;
    return enabled&&ctx&&ctx->wram&&ctx->hram&&seasons_editor::supported_rom(ctx);
}
static bool loaded(GBContext* ctx){return ctx&&ctx->wram&&ctx->hram&&ctx->hram[0x18]<3&&ctx->wram[0x611]==0&&memcmp(ctx->wram+0x5b2,"Z11216-0",8)==0;}
static bool owned_item(GBContext* ctx,int item){
    if(!loaded(ctx))return false;const uint8_t* r=ctx->wram;
    if(item==0x0e)return (r[0x693]&0x40)&&r[0x6af]<=3;
    if(item==7)return (r[0x692]&0x80)!=0;
    if(item==8)return (r[0x693]&1)!=0;
    if(item==0x1e)return (r[0x695]&0x40)!=0;
    if(item==0x0c)return (r[0x693]&0x10)!=0;
    if(item==0x0d)return (r[0x693]&0x20)!=0;
    if(item==0x13)return (r[0x694]&8)&&r[0x6b3]>=1&&r[0x6b3]<=2;
    if(item==0x19)return (r[0x695]&2)&&r[0x6ae]>=1&&r[0x6ae]<=3;
    if(item==0x15)return (r[0x694]&0x20)!=0;
    if(item==3)return (r[0x692]&8)!=0;
    if(item==0x16)return (r[0x694]&0x40)!=0;
    if(item==1)return (r[0x692]&2)&&r[0x6a9]>=1&&r[0x6a9]<=3;
    if(item==6)return (r[0x692]&0x40)&&r[0x6b1]>=1&&r[0x6b1]<=2;
    if(item==5)return (r[0x692]&0x20)&&r[0x6ac]>=1&&r[0x6ac]<=3;
    if(item==0x17)return (r[0x694]&0x80)&&r[0x6b4]>=1&&r[0x6b4]<=2;
    return false;
}
static const uint8_t* parent(GBContext* ctx,unsigned slot){return ctx->wram+0x1000+(slot-0xd0)*0x100;}
static bool private_mask(unsigned mask){return mask==4||mask==8||mask==16||mask==32||mask==64||mask==128;}
static bool private_parent(GBContext* ctx){
    if(!ctx||!ctx->wram)return false;
    for(unsigned slot=0xd2;slot<=0xd5;++slot){const uint8_t* p=parent(ctx,slot);if(p[0]&&valid_item(p[1])&&private_mask(p[3]))return true;}
    return false;
}
static bool held_for(GBContext* ctx,int item){ItemInput* in=input_for(item);return in&&in->held&&owned_item(ctx,item);}
static bool can_start(GBContext* ctx,int item){
    if(item==3&&ctx&&ctx->wram&&parent(ctx,0xd2)[0]&&parent(ctx,0xd2)[1]==3&&parent(ctx,0xd2)[4]>=2)return false;
    if(item==0x16&&ctx&&ctx->wram&&parent(ctx,0xd2)[0]&&parent(ctx,0xd2)[1]==0x16)return false;
    if(!owned_item(ctx,item)||menu_gui_is_active())return false;const uint8_t* r=ctx->wram;
    return r[0x2ee]==2&&r[0x2ef]==1&&!r[0xbcb]&&!r[0xba0]&&!r[0xc02]&&!r[0xc67]&&!r[0xc6a]&&(r[0xd00]==1||r[0xd00]==0x81)&&r[0xd04]==2;
}
static void reset_input(GBContext* ctx){
    owner=ctx;eligible=query=false;query_item=query_mask=0;
    for(int i=0;i<4;++i)bindings[i]={false,false,true,0};
    for(int i=0;i<15;++i)inputs[i].held=inputs[i].request=inputs[i].pressed=false;
}
static bool borrowing(GBContext* ctx){return enabled_for(ctx)&&((owner==ctx&&query)||private_parent(ctx));}
static void before_load(GBContext* ctx){reset_input(ctx);if(enabled_for(ctx)){g_joypad_buttons=0xff;g_joypad_dpad=0xff;}}
static void shutdown(GBContext* ctx){reset_input(ctx);}
static void poll(GBContext* ctx,SDL_GameController* pad,bool reserve_x,bool focused){
    if(!reserve_x){reset_input(ctx);return;}if(owner!=ctx)reset_input(ctx);
    const int choices[4]={menu_gui_get_extra_x_item(),menu_gui_get_extra_y_item(),menu_gui_get_extra_l_item(),menu_gui_get_extra_r_item()};
    const int buttons[4]={SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER};
    const bool usable=focused&&pad&&!menu_gui_is_active();
    for(int i=0;i<4;++i){
        Binding& b=bindings[i];b.raw=pad&&SDL_GameControllerGetButton(pad,(SDL_GameControllerButton)buttons[i]);
        const uint8_t next=valid_item(choices[i])?(uint8_t)choices[i]:0;
        if(next!=b.item){b.item=next;b.held=false;b.blocked=b.raw;}
        if(!b.raw)b.blocked=false;
        if(!usable||!owned_item(ctx,b.item)){b.held=false;b.blocked=b.raw;}
        else b.held=b.raw&&!b.blocked;
    }
    for(int i=0;i<15;++i){
        ItemInput& in=inputs[i];const bool was=in.held;
        in.held=false;
        for(int b=0;b<4;++b)if(bindings[b].item==in.item&&bindings[b].held)in.held=true;
        in.pressed=in.held&&!was;
        if(!in.held)in.request=false;else if(!was||in.item==1||in.item==0x16)in.request=true;
    }
}
static bool read(GBContext* ctx,uint16_t addr,uint8_t* value){
    if(ctx!=owner||!enabled_for(ctx)||ctx->rom_bank!=6)return false;
    /* Native Bracelet/Bomb state 3 tests fresh A/B presses directly at 06:5052.
     * Supply a transient press only here; never alter the game's key memory. */
    if((ctx->pc==0x5052||ctx->pc==0x5055)&&addr==0xcc46&&ctx->wram_bank==1&&ctx->d==0xd2){
        const uint8_t* p=parent(ctx,0xd2);
        ItemInput* in=(p[1]==0x16||p[1]==3)?input_for(p[1]):NULL;
        if(in&&in->pressed&&in->held&&owned_item(ctx,p[1])&&!menu_gui_is_active()&&p[0]&&p[4]==3){
            in->pressed=false;in->request=false;*value=ctx->wram[0xc46]|1;return true;
        }
    }
    const bool fresh=(ctx->pc==0x491e||ctx->pc==0x491f)&&addr==0xcc46;
    const bool continuous=(ctx->pc==0x491e||ctx->pc==0x491f)&&addr==0xcc45;
    ItemInput* native=input_for(ctx->e);
    if(!query&&native&&native->request&&native->held&&can_start(ctx,native->item)&&(fresh||(continuous&&(native->item==1||native->item==0x16)))&&
       ((ctx->d==1&&ctx->wram[0x681]==native->item)||(ctx->d==2&&ctx->wram[0x680]==native->item))){
        *value=ctx->wram[addr-0xc000]|ctx->d;native->request=false;return true;
    }
    if(query){
        if((ctx->pc==0x48f6||ctx->pc==0x48f7)&&addr==0xc680){*value=query_item;return true;}
        if(fresh||(continuous&&(query_item==1||query_item==0x16))){*value=query_mask;return true;}
    }else if((ctx->pc==0x48f6||ctx->pc==0x48f7)&&addr==0xc680&&ctx->d==2){
        bool pending=false;for(int i=0;i<15;++i)if(inputs[i].request&&inputs[i].held&&can_start(ctx,inputs[i].item))pending=true;
        if(pending&&gb_read16(ctx,ctx->sp)==0x48c1){eligible=true;eligible_sp=(uint16_t)(ctx->sp+2);}
    }
    if((ctx->pc==0x53eb||ctx->pc==0x53ee)&&addr==0xcc45&&ctx->d>=0xd2&&ctx->d<=0xd5&&ctx->wram_bank==1&&ctx->hl==((ctx->d<<8)|3)){
        const uint8_t* p=parent(ctx,ctx->d);
        if(p[0]&&valid_item(p[1])){
            if(private_mask(p[3])){*value=held_for(ctx,p[1])?p[3]:0;return true;}
            if(held_for(ctx,p[1])&&((p[3]==1&&ctx->wram[0x681]==p[1])||(p[3]==2&&ctx->wram[0x680]==p[1]))){*value=ctx->wram[0xc45]|p[3];return true;}
        }
    }
    return false;
}
static bool enter(GBContext* ctx){
    if(ctx!=owner||!enabled_for(ctx)||ctx->rom_bank!=6||ctx->pc!=0x48c1)return false;
    if(query){
        if(ctx->sp!=return_sp)return false;
        query=false;
    }
    if(!eligible)return false;
    if(ctx->sp!=eligible_sp){eligible=false;return false;}
    for(int i=0;i<15;++i){
        ItemInput& in=inputs[i];
        if(!in.request||!in.held||!can_start(ctx,in.item))continue;
        in.request=false;return_sp=ctx->sp;query_item=in.item;query_mask=in.mask;query=true;
        ctx->de=(uint16_t)((query_mask<<8)|0x80);gb_push16(ctx,0x48c1);ctx->pc=0x48f3;gb_tick(ctx,24);return true;
    }
    eligible=false;return false;
}
static uint8_t assignment(GBContext* ctx,uint16_t addr,uint8_t value){
    if(ctx==owner&&enabled_for(ctx)&&ctx->rom_bank==6&&ctx->wram_bank==1&&(ctx->pc==0x485d||ctx->pc==0x485e)&&
       (addr==0xd203||addr==0xd303||addr==0xd403||addr==0xd503)){
        const uint8_t* p=parent(ctx,addr>>8);if(p[0]&&valid_item(p[1])&&private_mask(p[3]))return p[3];
    }
    return value;
}
static void observe(GBContext* ctx,SDL_GameController*,bool focused){
    if(!enabled_for(ctx))return;
    static bool seen=false;static uint8_t previous[11];static unsigned lines=0;
    uint8_t sample[11]={(uint8_t)bindings[0].raw,(uint8_t)bindings[1].raw,(uint8_t)inputs[0].held,(uint8_t)inputs[1].held,
        bindings[0].item,bindings[1].item,(uint8_t)query,(uint8_t)focused,ctx->wram[0x681],ctx->wram[0x680],ctx->wram[0x2ee]};
    if(lines<400&&(!seen||memcmp(previous,sample,sizeof sample))){
        memcpy(previous,sample,sizeof sample);seen=true;++lines;
        fprintf(stderr,"[EXTRA-XY] X=%u Y=%u sword=%u feather=%u Xitem=%02X Yitem=%02X call=%u focus=%u A=%02X B=%02X game=%02X\n",
            (unsigned)sample[0],(unsigned)sample[1],(unsigned)sample[2],(unsigned)sample[3],(unsigned)sample[4],(unsigned)sample[5],
            (unsigned)sample[6],(unsigned)sample[7],(unsigned)sample[8],(unsigned)sample[9],(unsigned)sample[10]);
    }
}
#ifdef LA_HAS_IMGUI
static void draw_indicator(GBContext* ctx,SDL_GameController*,const SDL_Rect* dst){
    if(!enabled_for(ctx)||!menu_gui_extra_indicators_enabled()||!dst)return;ImGuiIO& io=ImGui::GetIO();
    const float sx=io.DisplayFramebufferScale.x>0?io.DisplayFramebufferScale.x:1,sy=io.DisplayFramebufferScale.y>0?io.DisplayFramebufferScale.y:1;
    ImDrawList* d=ImGui::GetForegroundDrawList();const char* labels[4]={"X","Y","L","R"};
    for(int i=0;i<4;++i){const Binding& b=bindings[i];
        const float x=(dst->x+dst->w)/sx-120+i*30,y=(dst->y+dst->h)/sy-30;
        const ImU32 color=(!b.raw||!valid_item(b.item))?IM_COL32(55,60,65,225):b.held?IM_COL32(28,160,85,245):IM_COL32(195,125,20,245);
        d->AddRectFilled(ImVec2(x,y),ImVec2(x+24,y+24),color,5);d->AddRect(ImVec2(x,y),ImVec2(x+24,y+24),IM_COL32(230,235,240,255),5);
        const ImVec2 t=ImGui::CalcTextSize(labels[i]);d->AddText(ImVec2(x+(24-t.x)/2,y+(24-t.y)/2),IM_COL32(255,255,255,255),labels[i]);
    }
}
#endif
}
#include "oracle_extra_ages_test.inc"
namespace oracle_extra_test {
static bool ages(GBContext* c){return oracle_ages_test::enabled_for(c);}
static bool enabled_for(GBContext* c){return oracle_sword_test::enabled_for(c)||ages(c);}
static bool borrowing(GBContext* c){return ages(c)?oracle_ages_test::borrowing(c):oracle_sword_test::borrowing(c);}
static void before_load(GBContext* c){oracle_sword_test::before_load(c);oracle_ages_test::before_load(c);}
static void shutdown(GBContext* c){oracle_sword_test::shutdown(c);oracle_ages_test::shutdown(c);}
static void poll(GBContext* c,SDL_GameController* p,bool reserve,bool focus){
    oracle_sword_test::poll(c,p,reserve&&oracle_sword_test::enabled_for(c),focus);
    oracle_ages_test::poll(c,p,reserve&&ages(c),focus);
}
static void observe(GBContext* c,SDL_GameController* p,bool focus){if(ages(c))oracle_ages_test::observe(c,p,focus);else oracle_sword_test::observe(c,p,focus);}
static bool read(GBContext* c,uint16_t a,uint8_t* v){return ages(c)?oracle_ages_test::read(c,a,v):oracle_sword_test::read(c,a,v);}
static bool enter(GBContext* c){return ages(c)?oracle_ages_test::enter(c):oracle_sword_test::enter(c);}
static uint8_t assignment(GBContext* c,uint16_t a,uint8_t v){return ages(c)?oracle_ages_test::assignment(c,a,v):oracle_sword_test::assignment(c,a,v);}
#ifdef LA_HAS_IMGUI
static void draw_indicator(GBContext* c,SDL_GameController* p,const SDL_Rect* d){if(ages(c))oracle_ages_test::draw_indicator(c,p,d);else oracle_sword_test::draw_indicator(c,p,d);}
#endif
}
extern "C" bool oracle_independent_sword_read(GBContext* c,uint16_t a,uint8_t* v){return oracle_extra_test::read(c,a,v);}
extern "C" bool oracle_independent_sword_enter(GBContext* c){return oracle_extra_test::enter(c);}
extern "C" uint8_t oracle_independent_sword_assignment(GBContext* c,uint16_t a,uint8_t v){return oracle_extra_test::assignment(c,a,v);}
