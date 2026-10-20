/* Linked Game menu, stage one. Included by menu_gui.cpp after editor helpers. */
#include "oracle_linked_core.h"
static oracle_linked::Game linked_game;
static bool linked_ring_choices[64]={};
static int linked_kind=0;
static char linked_text[2048]="";
static std::string linked_status="Generate a password, or paste an existing one to check it.";
static void draw_linked_menu(GBContext* ctx){
    using namespace oracle_linked;
    ImGui::SetNextWindowSize(ImVec2(540,580),ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Linked Game",&show_linked_menu)){
        ImGui::TextUnformatted("Gameplay paused");
        const bool ages=ages_editor::supported_rom(ctx),seasons=seasons_editor::supported_rom(ctx);
        if(!ages&&!seasons){ImGui::TextWrapped("Supports the verified USA Ages and Seasons ROMs only.");ImGui::End();return;}
        const char* kinds[]={"New linked game (20 symbols)","Ring transfer (15 symbols)"};
        ImGui::Combo("Password type",&linked_kind,kinds,2);
        if(ImGui::Button("Read current loaded file")){
            if(ctx&&ctx->wram&&mem(ctx,0xc2ee)==2&&(!memcmp(ctx->wram+0x5b2,"Z21216-0",8)||!memcmp(ctx->wram+0x5b2,"Z11216-0",8))){
                linked_game.id=(mem(ctx,0xc600)|(mem(ctx,0xc601)<<8))&32767;
                linked_game.target=ages?1:0;
                for(int i=0;i<5;++i){linked_game.hero[i]=(char)mem(ctx,0xc602+i);linked_game.child[i]=(char)mem(ctx,0xc609+i);}linked_game.hero[5]=linked_game.child[5]=0;
                int animal=mem(ctx,0xc610);linked_game.animal=animal>=11&&animal<=13?animal:11;
                linked_game.behavior=mem(ctx,0xc60f)&63;linked_game.free_ring=mem(ctx,0xc615)!=0;
                for(int i=0;i<64;++i)linked_ring_choices[i]=(mem(ctx,0xc616+i/8)&(1u<<(i%8)))!=0;
                linked_status="Read the loaded file. Target game was set to the other adventure. Check the choices before generating.";
            }else linked_status="Load your normal game first to read its identity. You can also enter the choices manually.";
        }
        ImGui::InputInt("Game ID (1-32767)",&linked_game.id);
        if(!linked_kind){
            const char* games[]={"Ages","Seasons"};ImGui::Combo("Password target",&linked_game.target,games,2);
            ImGui::InputText("Hero name (up to 5)",linked_game.hero,sizeof(linked_game.hero));
            ImGui::InputText("Child name (up to 5)",linked_game.child,sizeof(linked_game.child));
            const char* animals[]={"Ricky","Dimitri","Moosh"};int choice=linked_game.animal-11;
            if(ImGui::Combo("Companion",&choice,animals,3))linked_game.animal=choice+11;
            ImGui::Checkbox("Linked story",&linked_game.linked);ImGui::SameLine();ImGui::Checkbox("Hero's Quest",&linked_game.hero_quest);
            ImGui::Checkbox("Free ring already received",&linked_game.free_ring);
            if(ImGui::TreeNode("Child behavior (advanced)")){ImGui::SliderInt("Value",&linked_game.behavior,0,63);ImGui::TreePop();}
        }else{
            ImGui::TextWrapped("Use the receiving file's matching Game ID. Ring transfer still goes through the game's ring snake.");
            if(ImGui::Button("Select all rings"))for(bool& b:linked_ring_choices)b=true;
            ImGui::SameLine();if(ImGui::Button("Clear rings"))for(bool& b:linked_ring_choices)b=false;
            ImGui::BeginChild("Linked ring choices",ImVec2(0,125),true);
            for(int i=0;i<64;++i)ImGui::Checkbox(editor_ring_names[i],&linked_ring_choices[i]);
            ImGui::EndChild();
        }
        if(ImGui::Button("Generate password")){
            std::vector<uint8_t> secret;std::string error;uint64_t rings=0;for(int i=0;i<64;++i)if(linked_ring_choices[i])rings|=UINT64_C(1)<<i;
            bool ok=linked_kind?ring_secret(linked_game.id,rings,secret,error):game_secret(linked_game,secret,error);
            if(ok){std::vector<uint8_t> decoded;ok=decode(secret,decoded,error);}
            if(ok){snprintf(linked_text,sizeof(linked_text),"%s",format(secret,true).c_str());linked_status="Generated and checksum checked. Symbol words below represent the game's special characters.";}
            else linked_status=error;
        }
        ImGui::Separator();
        ImGui::InputTextMultiline("Password",linked_text,sizeof(linked_text),ImVec2(-1,85));
        if(ImGui::Button("Copy Password")){
            std::vector<uint8_t> s;std::string error;
            if(parse(linked_text,s,error)){linked_status=SDL_SetClipboardText(format(s).c_str())==0?"Password copied, including its special symbols.":"Clipboard copy failed.";}else linked_status=error;
        }
        ImGui::SameLine();if(ImGui::Button("Paste from Clipboard")){
            char* text=SDL_GetClipboardText();std::vector<uint8_t>s;std::string error;
            if(parse(text,s,error)){snprintf(linked_text,sizeof(linked_text),"%s",format(s,true).c_str());linked_status="Clipboard password parsed and checksum checked.";}else linked_status=error;
            SDL_free(text);
        }
        if(ImGui::Button("Check password")){
            std::vector<uint8_t>s,d;std::string error;
            if(parse(linked_text,s,error)&&decode(s,d,error)){
                char info[180];snprintf(info,sizeof(info),"Valid %s secret. Game ID: %d.%s",s.size()==20?(bits(d,21,1)?"Seasons game":"Ages game"):"ring",bits(d,5,15),s.size()==20?(bits(d,105,1)?" Linked story enabled.":" Linked story disabled."):" Native recipient checks still apply.");linked_status=info;
            }else linked_status=error;
        }
        if(linked_kind&&ages){
            ImGui::TextWrapped("Ages ring-screen filling is pending its own entry-state check. Ring generation, checking and clipboard work here.");
        }else{
            const char* label=linked_kind?"Fill Seasons ring-secret screen":(ages?"Fill Ages password screen":"Fill Seasons password screen");
            if(ImGui::Button(label)){
                std::vector<uint8_t>s;std::string error;bool ok=parse(linked_text,s,error);
                if(ok)ok=linked_kind?fill_seasons_ring_entry(ctx,s,error):fill_game_entry(ctx,s,ages?0:1,error);
                if(ok)linked_status=linked_kind?
                    "Filled the ring-secret screen. Close this window, then use the game's OK option. The snake will check and add the rings.":
                    "Filled the new-game entry screen. Close this window, then use the game's OK option to confirm.";
                else linked_status=error;
            }
            ImGui::TextWrapped(linked_kind?
                "Fill works only on the red snake's 15-symbol entry screen and requires this file's Game ID. Confirm with the game's OK option.":
                "Fill works only on this game's new-game secret screen for an empty file. It does not create a file or press OK.");
        }
        ImGui::Separator();ImGui::TextWrapped("%s",linked_status.c_str());
    }
    ImGui::End();
}
