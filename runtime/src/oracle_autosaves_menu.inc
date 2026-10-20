static bool show_autosaves=false;
static void draw_autosave_interval() {
    const int values[]={0,1,2,5,10,15};
    const char* labels[]={"Off","1 minute","2 minutes","5 minutes","10 minutes","15 minutes"};
    int selected=0;
    for(int i=0;i<6;++i)if(g_autosave_minutes==values[i])selected=i;
    if(ImGui::Combo("Autosave interval",&selected,labels,6))g_autosave_minutes=values[selected];
}
static void draw_autosaves(GBContext* ctx) {
    static int pending=0;
    ImGui::SetNextWindowSize(ImVec2(500,600),ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Autosaves",&show_autosaves)) {
        draw_autosave_interval();
        ImGui::TextWrapped("Real-time minutes during gameplay. The newest five are kept separately from manual slots. Loading replaces your current progress.");
        ImGui::TextWrapped("%s",gb_autosaves_status());
        ImGui::Separator();
        for(int rank=1;rank<=5;++rank) {
            ImGui::PushID(rank);
            char stamp[96];int state=gb_autosaves_info(rank,stamp,sizeof(stamp));
            void* texture=gb_autosaves_preview(rank);
            ImGui::BeginGroup();
            if(texture)ImGui::Image((ImTextureID)(intptr_t)texture,ImVec2(128,115));
            else ImGui::Dummy(ImVec2(128,115));
            ImGui::EndGroup();ImGui::SameLine();ImGui::BeginGroup();
            ImGui::Text("%d%s",rank,rank==1?" (newest)":"");
            ImGui::TextUnformatted(stamp);
            if(state==1&&ImGui::Button("Load..."))pending=rank;
            ImGui::EndGroup();ImGui::Separator();ImGui::PopID();
        }
        if(pending&&!ImGui::IsPopupOpen("Load autosave?"))ImGui::OpenPopup("Load autosave?");
        if(ImGui::BeginPopupModal("Load autosave?",NULL,ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextWrapped("Load autosave %d? Unsaved progress will be replaced.",pending);
            if(ImGui::Button("Load")){gb_autosaves_load(ctx,pending);pending=0;ImGui::CloseCurrentPopup();}
            ImGui::SameLine();
            if(ImGui::Button("Cancel")){pending=0;ImGui::CloseCurrentPopup();}
            ImGui::EndPopup();
        }
    }
    ImGui::End();
    if(!show_autosaves)pending=0;
}
