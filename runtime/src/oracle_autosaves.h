#ifndef ORACLE_AUTOSAVES_H
#define ORACLE_AUTOSAVES_H
#include "gbrt.h"
#ifdef __cplusplus
extern "C" {
#endif
int menu_gui_autosave_minutes(void);
void gb_autosaves_tick(GBContext* ctx, int minutes, bool paused);
int gb_autosaves_info(int rank, char* label, size_t size);
void* gb_autosaves_preview(int rank);
bool gb_autosaves_load(GBContext* ctx, int rank);
const char* gb_autosaves_status(void);
#ifdef __cplusplus
}
#endif
#endif
