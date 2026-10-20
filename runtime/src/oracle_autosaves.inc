/* Five rotating snapshots; manual slots, legacy auto, and editor backup stay separate. */
static uint32_t autosave_last_tick = 0;
static uint64_t autosave_elapsed = 0;
static int autosave_last_minutes = -1;
static bool autosave_clock_started = false;
static char autosave_status[192] = "Autosaves use separate files from manual slots.";

static int autosave_slot_at_rank(int rank) {
    if (rank < 1 || rank > 5) return -1;
    struct Entry {int slot; int64_t stamp;};
    Entry entries[5];
    for (int i=0;i<5;++i) {
        int slot=12+i;
        SlotPreview* preview=slot_preview_entry(slot);
        entries[i]={slot,preview && preview->state==1?preview->timestamp:INT64_MIN};
    }
    for (int i=0;i<5;++i) for(int j=i+1;j<5;++j)
        if(entries[j].stamp>entries[i].stamp) {Entry t=entries[i];entries[i]=entries[j];entries[j]=t;}
    return entries[rank-1].slot;
}
static bool autosave_write(GBContext* ctx) {
    if (!slot_ready(ctx)) return false;
    int chosen=12;int64_t oldest=INT64_MAX;
    for(int slot=12;slot<=16;++slot) {
        SlotHeader h={};std::vector<uint8_t> data;
        if(!slot_read(slot,ctx,h,data)){chosen=slot;break;}
        if(h.timestamp<oldest){oldest=h.timestamp;chosen=slot;}
    }
    const int selected=slot_selected;
    slot_selected=chosen;
    bool ok=slot_save_state_impl(ctx);
    slot_selected=selected;
    snprintf(autosave_status,sizeof(autosave_status),"%s",ok?"Autosave completed.":"Autosave failed. Previous save retained.");
    return ok;
}
extern "C" void gb_autosaves_tick(GBContext* ctx,int minutes,bool paused) {
    uint32_t now=SDL_GetTicks();
    if(!autosave_clock_started||minutes!=autosave_last_minutes) {
        autosave_clock_started=true;autosave_last_tick=now;
        autosave_elapsed=0;autosave_last_minutes=minutes;return;
    }
    const uint32_t delta=now-autosave_last_tick;autosave_last_tick=now;
    if(minutes<=0||paused||!slot_ready(ctx))return;
    /* Oracle gameplay state; do not rotate snapshots while on title/file screens. */
    if(ctx->wram[0x2ee]!=2)return;
    autosave_elapsed+=delta;
    if(autosave_elapsed>=uint64_t(minutes)*60000) {
        autosave_elapsed=0;
        autosave_write(ctx);
    }
}
extern "C" int gb_autosaves_info(int rank,char* label,size_t size) {
    int slot=autosave_slot_at_rank(rank);
    return gb_slots_info(slot,label,size);
}
extern "C" void* gb_autosaves_preview(int rank) {
    return gb_slots_preview(autosave_slot_at_rank(rank));
}
extern "C" bool gb_autosaves_load(GBContext* ctx,int rank) {
    int slot=autosave_slot_at_rank(rank);
    if(slot<12||slot>16)return false;
    const int selected=slot_selected;slot_selected=slot;
    bool ok=slot_load_state_impl(ctx);slot_selected=selected;
    snprintf(autosave_status,sizeof(autosave_status),"%s",ok?"Autosave loaded.":"Could not load autosave: empty, damaged, or incompatible.");
    autosave_elapsed=0;autosave_last_tick=SDL_GetTicks();
    return ok;
}
extern "C" const char* gb_autosaves_status(void){return autosave_status;}
