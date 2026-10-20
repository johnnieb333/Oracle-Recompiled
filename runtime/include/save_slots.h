#ifndef GBRT_SAVE_SLOTS_H
#define GBRT_SAVE_SLOTS_H
#include "gbrt.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GBRT_SAVE_SLOT_COUNT 10
void gb_slots_auto_save(GBContext* ctx);
int gb_slots_selected(void);
void gb_slots_select(int slot);
int gb_slots_info(int slot, char* label, size_t size);
void* gb_slots_preview(int slot);
const char* gb_slots_status(void);
void gb_slots_shutdown(void);
size_t gb_audio_snapshot_size(void);
bool gb_audio_snapshot_write(const void* apu, void* out, size_t size);
bool gb_audio_snapshot_read(void* apu, const void* data, size_t size);
#ifdef __cplusplus
}
#endif
#endif
