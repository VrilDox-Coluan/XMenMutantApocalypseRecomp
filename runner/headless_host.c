#include "types.h"
#include "wotg_compat.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct SpcPlayer;

bool g_new_ppu = true;
bool g_ws_active = false;
int g_ws_extra = 0;
struct SpcPlayer *g_spc_player = NULL;

void NORETURN Die(const char *error) {
    fprintf(stderr, "fatal: %s\n", error ? error : "unknown error");
    exit(EXIT_FAILURE);
}

void RtlApuLock(void) {}
void RtlApuUnlock(void) {}

void RtlDrawPpuFrame(uint8 *pixel_buffer, size_t pitch, uint32 render_flags) {
    (void)pixel_buffer;
    (void)pitch;
    (void)render_flags;
}

void MkDir(const char *s) { wotg_mkdir(s, 0755); }
void ChangeWindowScale(int scale_step) { (void)scale_step; }
