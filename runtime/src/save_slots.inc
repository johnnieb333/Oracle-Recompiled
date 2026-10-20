/* Included by platform_sdl.cpp: desktop slot state, no CMake changes needed. */
#include "save_slots.h"
#include <vector>
#include <ctime>
#include <cstring>
#include <type_traits>

static int slot_selected = 1;  /* 0 is reserved for automatic snapshots. */
static char slot_status[192] = "Choose a slot to save or load.";
static unsigned slot_revision = 1;
static const uint32_t slot_magic = 0x32534F5A; /* ZOS2 */
struct SlotHeader {
    uint32_t magic, version, rom_hash, rom_size;
    uint32_t context_size, ppu_size, audio_size, eram_size;
    uint32_t payload_size, checksum;
    int64_t timestamp;
};
static_assert(sizeof(SlotHeader) == 48, "Unexpected slot header layout");
static_assert(std::is_trivially_copyable<GBPPU>::value, "PPU must be pointer-free POD");

#define SLOT_FIELDS(X) \
    X(af) X(bc) X(de) X(hl) X(sp) X(pc) \
    X(f_z) X(f_n) X(f_h) X(f_c) X(ime) X(ime_pending) \
    X(halted) X(stopped) X(halt_bug) X(speed_switch_halt) X(dma) \
    X(rom_bank) X(ram_bank) X(wram_bank) X(vram_bank) \
    X(mbc_type) X(ram_enabled) X(mbc_mode) X(rom_bank_upper) \
    X(rtc_mode) X(rtc_reg) X(cycles) X(frame_cycles) X(last_sync_cycles) \
    X(frame_done) X(div_counter) X(rtc) X(last_joypad)

static uint32_t slot_hash(const void* p, size_t size) {
    const uint8_t* b = (const uint8_t*)p;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < size; ++i) h = (h ^ b[i]) * 16777619u;
    return h;
}
static size_t slot_scalar_size() {
    size_t n = 0;
#define COUNT_FIELD(f) n += sizeof(((GBContext*)0)->f);
    SLOT_FIELDS(COUNT_FIELD)
#undef COUNT_FIELD
    return n;
}
static size_t slot_payload_size(const GBContext* ctx) {
    return slot_scalar_size() + SS_WRAM_SIZE + SS_VRAM_SIZE + SS_OAM_SIZE +
        SS_HRAM_SIZE + SS_IO_SIZE + ctx->eram_size + sizeof(GBPPU) +
        gb_audio_snapshot_size() + GB_FRAMEBUFFER_SIZE * sizeof(uint32_t);
}
static bool slot_ready(const GBContext* ctx) {
    return ctx && ctx->rom && ctx->rom_size && ctx->wram && ctx->vram &&
        ctx->oam && ctx->hram && ctx->io && ctx->ppu && ctx->apu &&
        (!ctx->eram_size || ctx->eram);
}
static void slot_path(int slot, char* out, size_t size) {
    char filename[64];
    if (slot >= 12 && slot <= 16) snprintf(filename, sizeof(filename), "autosave-%02d.bin", slot-11);
    else if (slot == 11) snprintf(filename, sizeof(filename), "savestate-before-editor.bin");
    else if (slot == 0) snprintf(filename, sizeof(filename), "savestate-auto.bin");
    else snprintf(filename, sizeof(filename), "savestate-slot%02d.bin", slot);
    char* base = NULL;
#ifdef __ANDROID__
    if (!g_save_dir[0]) {
        const char* internal = SDL_AndroidGetInternalStoragePath();
        if (internal) {
            snprintf(out, size, "%s/%s", internal, filename);
            return;
        }
    }
#endif
    base = sdl_save_prefix_dup();
    snprintf(out, size, "%s%s", base ? base : "", filename);
    if (base) SDL_free(base);
}
static FILE* slot_open(int slot, const char* mode) {
    char path[512];
    slot_path(slot, path, sizeof(path));
    FILE* f = fopen(path, mode);
    if (!f && mode[0] == 'r') {
        char backup[520];
        snprintf(backup, sizeof(backup), "%s.bak", path);
        f = fopen(backup, mode); /* Recover if interrupted during replacement. */
    }
    return f;
}
static bool slot_read(int slot, GBContext* ctx, SlotHeader& h, std::vector<uint8_t>& data) {
    if (!slot_ready(ctx) || slot < 0 || slot > 16) return false;
    FILE* f = slot_open(slot, "rb");
    if (!f) return false;
    bool ok = fread(&h, sizeof(h), 1, f) == 1;
    const size_t expected = slot_payload_size(ctx);
    ok = ok && h.magic == slot_magic && h.version == 2 &&
        h.rom_size == ctx->rom_size && h.rom_hash == slot_hash(ctx->rom, ctx->rom_size) &&
        h.context_size == sizeof(GBContext) && h.ppu_size == sizeof(GBPPU) &&
        h.audio_size == gb_audio_snapshot_size() && h.eram_size == ctx->eram_size &&
        h.payload_size == expected;
    if (ok) {
        data.resize(expected);
        ok = fread(data.data(), 1, expected, f) == expected && fgetc(f) == EOF &&
             !ferror(f) && slot_hash(data.data(), data.size()) == h.checksum;
    }
    fclose(f);
    return ok;
}
extern "C" int gb_slots_selected(void) { return slot_selected; }
extern "C" void gb_slots_select(int slot) {
    if (slot >= 1 && slot <= GBRT_SAVE_SLOT_COUNT) slot_selected = slot;
}
extern "C" const char* gb_slots_status(void) { return slot_status; }

static bool slot_save_state_impl(GBContext* ctx) {
    if (!slot_ready(ctx)) return false;
    const int slot = slot_selected;
    std::vector<uint8_t> data(slot_payload_size(ctx));
    size_t pos = 0;
    const auto put = [&](const void* p, size_t n) {
        if (n) memcpy(data.data() + pos, p, n);
        pos += n;
    };
#define PUT_FIELD(f) put(&ctx->f, sizeof(ctx->f));
    SLOT_FIELDS(PUT_FIELD)
#undef PUT_FIELD
    put(ctx->wram, SS_WRAM_SIZE); put(ctx->vram, SS_VRAM_SIZE);
    put(ctx->oam, SS_OAM_SIZE); put(ctx->hram, SS_HRAM_SIZE);
    put(ctx->io, SS_IO_SIZE); put(ctx->eram, ctx->eram_size);
    put(ctx->ppu, sizeof(GBPPU));
    if (!gb_audio_snapshot_write(ctx->apu, data.data() + pos, gb_audio_snapshot_size())) return false;
    pos += gb_audio_snapshot_size();
    put(((GBPPU*)ctx->ppu)->rgb_framebuffer, GB_FRAMEBUFFER_SIZE * sizeof(uint32_t));
    SlotHeader h = {};
    h.magic = slot_magic; h.version = 2;
    h.rom_hash = slot_hash(ctx->rom, ctx->rom_size); h.rom_size = (uint32_t)ctx->rom_size;
    h.context_size = sizeof(GBContext); h.ppu_size = sizeof(GBPPU);
    h.audio_size = (uint32_t)gb_audio_snapshot_size(); h.eram_size = (uint32_t)ctx->eram_size;
    h.payload_size = (uint32_t)data.size(); h.checksum = slot_hash(data.data(), data.size());
    h.timestamp = (int64_t)time(NULL);
    char path[512], temp[520], backup[520];
    slot_path(slot, path, sizeof(path));
    snprintf(temp, sizeof(temp), "%s.tmp", path);
    snprintf(backup, sizeof(backup), "%s.bak", path);
    FILE* f = fopen(temp, "wb");
    bool ok = f != NULL;
    if (f) {
        ok = fwrite(&h, sizeof(h), 1, f) == 1 &&
             fwrite(data.data(), 1, data.size(), f) == data.size();
        if (fflush(f) != 0) ok = false;
        if (fclose(f) != 0) ok = false;
    }
    if (ok) {
        FILE* old = fopen(path, "rb");
        const bool had_old = old != NULL;
        if (old) fclose(old);
        if (had_old) { remove(backup); ok = rename(path, backup) == 0; }
        if (ok && rename(temp, path) != 0) {
            ok = false;
            if (had_old) rename(backup, path);
        }
    }
    if (!ok) remove(temp);
    if (slot >= 12) {
        fprintf(stderr, "[AUTO] Autosave %s\n", ok ? "saved" : "failed");
    } else if (slot == 0) {
        fprintf(stderr, "[STATE] Automatic snapshot %s\n", ok ? "saved" : "failed");
    } else {
        snprintf(slot_status, sizeof(slot_status), ok ? "Saved slot %d." : "Could not save slot %d. Previous save retained.", slot);
        fprintf(stderr, "[STATE] %s\n", slot_status);
    }
    ++slot_revision; /* Preview resources are refreshed next frame, never mid-draw. */
    return ok;
}

void gb_platform_save_state(GBContext* ctx) { (void)slot_save_state_impl(ctx); }

static bool slot_load_state_impl(GBContext* ctx) {
    SlotHeader h = {};
    std::vector<uint8_t> data;
    if (!slot_read(slot_selected, ctx, h, data)) {
        snprintf(slot_status, sizeof(slot_status), "Slot %d is empty, damaged, or incompatible.", slot_selected);
        return false;
    }
    /* Validate the complete file before modifying any live game state. */
    GBContext staged = *ctx;
    size_t pos = 0;
    const auto take = [&](void* p, size_t n) {
        if (n) memcpy(p, data.data() + pos, n);
        pos += n;
    };
#define TAKE_FIELD(f) take(&staged.f, sizeof(staged.f));
    SLOT_FIELDS(TAKE_FIELD)
#undef TAKE_FIELD
    /* Keep process-owned pointers and callbacks from the current context. */
#define APPLY_FIELD(f) memcpy(&ctx->f, &staged.f, sizeof(ctx->f));
    SLOT_FIELDS(APPLY_FIELD)
#undef APPLY_FIELD
    take(ctx->wram, SS_WRAM_SIZE); take(ctx->vram, SS_VRAM_SIZE);
    take(ctx->oam, SS_OAM_SIZE); take(ctx->hram, SS_HRAM_SIZE);
    take(ctx->io, SS_IO_SIZE); take(ctx->eram, ctx->eram_size);
    take(ctx->ppu, sizeof(GBPPU));
    gb_audio_snapshot_read(ctx->apu, data.data() + pos, gb_audio_snapshot_size());
    if (g_audio_device) SDL_LockAudioDevice(g_audio_device);
    memset(g_audio_buffer, 0, sizeof(g_audio_buffer));
    g_audio_write_pos = 0; g_audio_read_pos = 0;
    if (g_audio_device) SDL_UnlockAudioDevice(g_audio_device);
    if(slot_selected >= 12) snprintf(slot_status,sizeof(slot_status),"Loaded autosave.");
    else snprintf(slot_status, sizeof(slot_status), "Loaded slot %d.", slot_selected);
    fprintf(stderr, "[STATE] %s\n", slot_status);
    return true;
}

void gb_platform_load_state(GBContext* ctx) { (void)slot_load_state_impl(ctx); }

struct SlotPreview {
    unsigned revision;
    int64_t timestamp;
    SDL_Texture* texture;
    int state;
    char label[96];
};
static SlotPreview slot_previews[16] = {};
static SlotPreview* slot_preview_entry(int slot) {
    if (slot < 1 || slot > 16) return NULL;
    SlotPreview& entry = slot_previews[slot - 1];
    if (entry.revision == slot_revision) return &entry;
    if (entry.texture) SDL_DestroyTexture(entry.texture);
    entry.texture = NULL;
    SlotHeader h = {};
    std::vector<uint8_t> data;
    if (slot_read(slot, g_ctx, h, data)) {
        entry.state = 1;
        entry.timestamp = h.timestamp;
        time_t stamp = (time_t)h.timestamp;
        struct tm* local = localtime(&stamp);
        if (local) strftime(entry.label, sizeof(entry.label), "%Y-%m-%d %H:%M:%S", local);
        else snprintf(entry.label, sizeof(entry.label), "Saved");
        if (g_renderer) {
            entry.texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STATIC, GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
            if (entry.texture) {
                SDL_SetTextureBlendMode(entry.texture, SDL_BLENDMODE_NONE);
                const void* pixels = data.data() + data.size() - GB_FRAMEBUFFER_SIZE * sizeof(uint32_t);
                if (SDL_UpdateTexture(entry.texture, NULL, pixels, GB_SCREEN_WIDTH * sizeof(uint32_t)) != 0) {
                    SDL_DestroyTexture(entry.texture); entry.texture = NULL;
                }
            }
        }
    } else {
        FILE* f = slot_open(slot, "rb");
        entry.state = f ? -1 : 0;
        entry.timestamp = INT64_MIN;
        if (f) fclose(f);
        snprintf(entry.label, sizeof(entry.label), "%s", entry.state ? "Damaged or incompatible" : "Empty slot");
    }
    entry.revision = slot_revision;
    return &entry;
}
extern "C" int gb_slots_info(int slot, char* label, size_t size) {
    SlotPreview* e = slot_preview_entry(slot);
    if (label && size) snprintf(label, size, "%s", e ? e->label : "Invalid slot");
    return e ? e->state : -1;
}
extern "C" void* gb_slots_preview(int slot) {
    SlotPreview* e = slot_preview_entry(slot);
    return e ? e->texture : NULL;
}
extern "C" void gb_slots_shutdown(void) {
    for (int i = 0; i < 16; ++i) {
        if (slot_previews[i].texture) SDL_DestroyTexture(slot_previews[i].texture);
        slot_previews[i] = SlotPreview{};
    }
    ++slot_revision;
}
#undef SLOT_FIELDS

extern "C" void gb_slots_auto_save(GBContext* ctx) {
    const int selected = slot_selected;
    slot_selected = 0;
    gb_platform_save_state(ctx);
    slot_selected = selected;
}

/* Dedicated editor restore point: never consumes or replaces manual slots. */
extern "C" bool gb_slots_editor_backup(GBContext* ctx) {
    const int selected = slot_selected;
    slot_selected = 11;
    bool ok = slot_save_state_impl(ctx);
    slot_selected = selected;
    return ok;
}
extern "C" bool gb_slots_editor_restore(GBContext* ctx) {
    const int selected = slot_selected;
    slot_selected = 11;
    bool ok = slot_load_state_impl(ctx);
    slot_selected = selected;
    return ok;
}
