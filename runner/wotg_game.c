#include "wotg_game.h"
#include "beam_frame_driver.h"
#include <stdio.h>
#include <stdlib.h>

static void WotgInitialize(void) {
    if (!getenv("SNESRECOMP_LLE_INTERP_TARGET_FILE")) {
        const char *candidates[] = {
            "runner/wotg_interp_targets.txt",
            "../runner/wotg_interp_targets.txt",
            "wotg_interp_targets.txt",
            NULL
        };
        for (int i = 0; candidates[i]; i++) {
            FILE *f = fopen(candidates[i], "r");
            if (f) {
                fclose(f);
#ifdef _WIN32
                _putenv_s("SNESRECOMP_LLE_INTERP_TARGET_FILE", candidates[i]);
#else
                setenv("SNESRECOMP_LLE_INTERP_TARGET_FILE", candidates[i], 1);
#endif
                break;
            }
        }
    }
    if (!getenv("SNESRECOMP_INTERP_STEP_CAP")) {
#ifdef _WIN32
        _putenv_s("SNESRECOMP_INTERP_STEP_CAP", "20000000");
#else
        setenv("SNESRECOMP_INTERP_STEP_CAP", "20000000", 0);
#endif
    }
}
static void WotgRunOneFrame(void) { snes_beam_frame_driver_run_frame(); }
static void WotgDrawPpuFrame(void) { snes_beam_frame_driver_draw_ppu_frame(); }
static void WotgSessionReset(void) { snes_beam_frame_driver_reset(); }

const RtlGameInfo kWotgGameInfo = {
    .title            = "wotg",
    .initialize       = &WotgInitialize,
    .run_frame        = &WotgRunOneFrame,
    .draw_ppu_frame   = &WotgDrawPpuFrame,
    .save_name_prefix = "wotg",
    .session_reset    = &WotgSessionReset,
};
