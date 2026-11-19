#include "host_main.h"
#include "wotg_game.h"
#include "config.h"
#include "display_aspect.h"
#include "snes/ppu.h"

static void XmenPrepareFrame(int dw, int dh, int *w, int *h) {
    if (!g_config.widescreen) {
        *w = 256;
        *h = 224;
        return;
    }
    double aspect = (dw > 0 && dh > 0) ? (double)dw / dh : (16.0 / 9.0);
    if (aspect < 16.0 / 9.0) {
        aspect = 16.0 / 9.0;
    }
    SnesDisplayFrame f = SnesDisplayAspect_ComputeAdaptiveFrame(
        256, 224, kPpuBufWidth, aspect,
        SnesDisplayAspect_Clamp(g_config.display_aspect));
    *w = f.width;
    *h = 224;
}

int main(int argc, char **argv) {
    static const SnesDesktopHostGame game = {
        .display_name             = "X-Men: Mutant Apocalypse",
        .window_title             = "X-Men: Mutant Apocalypse (Recompiled)",
        .region                   = "USA",
        .rom_file                 = "X-Men - Mutant Apocalypse (USA).sfc",
        .expected_sha256_hex      = "65fe17fd6b297f52df6ce9812ecb02c3bb1bfda3ebc05a19c4a8decbf9a446ae",
        .expected_crc32_hex       = "5e34822a",
        .game_id                  = "xmen",
        .build_version            = "dev",
        .env_prefix               = "XMEN",
        .game_info                = &kWotgGameInfo,
        .num_players              = 1,
        .frame_width              = 342,
        .frame_height             = 224,
        .native_widescreen        = 1,
        .state_menu_hotkeys       = 1,
        .display_aspect_supported = 1,
        .shader_supported         = 0,
        .prepare_frame            = XmenPrepareFrame,
    };
    return snesrecomp_desktop_main(&game, argc, argv);
}
