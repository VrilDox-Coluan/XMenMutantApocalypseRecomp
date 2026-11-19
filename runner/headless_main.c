/* headless_main.c -- War of the Gems windowless validation host.
 *
 * usage: wotg_headless --rom <path.smc> [--frames N] [--dump-png path]
 *                      [--dump-every K] [--input tape] [--log path]
 *                      [--interp-targets file] [--lle]
 */
#include "wotg_game.h"
#include "wotg_compat.h"

#include "common_cpu_infra.h"
#include "common_rtl.h"
#include "cpu_state.h"
#include "sha256.h"
#include "snes/interp_bridge.h"
#include "snes/joypad.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "snes/tier2_capture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { kWidth = 256, kHeight = 224, kBpp = 4 };
static uint8_t s_pixels[kWidth * kHeight * kBpp];

/* ── Minimal PNG writer (uncompressed deflate) ─────────────────────────── */
static uint32_t s_crc_table[256];

static void CrcInit(void) {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
        s_crc_table[n] = c;
    }
}

static uint32_t CrcUpdate(uint32_t c, const uint8_t *p, size_t n) {
    for (size_t i = 0; i < n; i++) c = s_crc_table[(c ^ p[i]) & 0xff] ^ (c >> 8);
    return c;
}

static void Be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}

static int WriteChunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    uint8_t hdr[8];
    Be32(hdr, len);
    memcpy(hdr + 4, type, 4);
    uint32_t crc = CrcUpdate(0xffffffffu, hdr + 4, 4);
    crc = CrcUpdate(crc, data, len) ^ 0xffffffffu;
    uint8_t tail[4];
    Be32(tail, crc);
    return fwrite(hdr, 1, 8, f) == 8 &&
           (len == 0 || fwrite(data, 1, len, f) == len) &&
           fwrite(tail, 1, 4, f) == 4;
}

static int WritePng(const char *path, const uint8_t *px, int w, int h, int pitch) {
    const size_t row = (size_t)w * 3 + 1;
    const size_t raw_len = row * (size_t)h;
    uint8_t *raw = (uint8_t *)malloc(raw_len);
    if (!raw) return 0;

    uint32_t adler_a = 1, adler_b = 0;
    for (int y = 0; y < h; y++) {
        uint8_t *dst = raw + (size_t)y * row;
        const uint8_t *src = px + (size_t)y * (size_t)pitch;
        dst[0] = 0;
        for (int x = 0; x < w; x++) {
            dst[1 + x * 3 + 0] = src[x * 4 + 2]; /* R */
            dst[1 + x * 3 + 1] = src[x * 4 + 1]; /* G */
            dst[1 + x * 3 + 2] = src[x * 4 + 0]; /* B */
        }
        for (size_t i = 0; i < row; i++) {
            adler_a = (adler_a + dst[i]) % 65521u;
            adler_b = (adler_b + adler_a) % 65521u;
        }
    }

    size_t max_blocks = (raw_len + 65534) / 65535;
    uint8_t *z = (uint8_t *)malloc(2 + raw_len + max_blocks * 5 + 4);
    if (!z) { free(raw); return 0; }
    size_t zp = 0;
    z[zp++] = 0x78; z[zp++] = 0x01;
    size_t off = 0;
    while (off < raw_len) {
        size_t chunk = raw_len - off;
        if (chunk > 65535) chunk = 65535;
        z[zp++] = (off + chunk == raw_len) ? 1 : 0;
        z[zp++] = (uint8_t)(chunk & 0xff);
        z[zp++] = (uint8_t)(chunk >> 8);
        z[zp++] = (uint8_t)(~chunk & 0xff);
        z[zp++] = (uint8_t)((~chunk >> 8) & 0xff);
        memcpy(z + zp, raw + off, chunk);
        zp += chunk;
        off += chunk;
    }
    uint32_t adler = (adler_b << 16) | adler_a;
    Be32(z + zp, adler);
    zp += 4;
    free(raw);

    FILE *f = fopen(path, "wb");
    if (!f) { free(z); return 0; }
    static const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    uint8_t ihdr[13];
    Be32(ihdr + 0, (uint32_t)w);
    Be32(ihdr + 4, (uint32_t)h);
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    int ok = fwrite(sig, 1, 8, f) == 8 &&
             WriteChunk(f, "IHDR", ihdr, 13) &&
             WriteChunk(f, "IDAT", z, (uint32_t)zp) &&
             WriteChunk(f, "IEND", NULL, 0);
    fclose(f);
    free(z);
    return ok;
}

/* ── Input tape (little-endian uint16 per frame) ───────────────────────── */
static uint16_t *s_tape = NULL;
static size_t s_tape_frames = 0;

static int LoadTape(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0 || (sz % 2) != 0) { fclose(f); return 0; }
    s_tape_frames = (size_t)sz / 2;
    s_tape = (uint16_t *)malloc((size_t)sz + 2);
    if (!s_tape) { fclose(f); return 0; }
    if (sz > 0 && fread(s_tape, 1, (size_t)sz, f) != (size_t)sz) {
        free(s_tape); s_tape = NULL; fclose(f); return 0;
    }
    fclose(f);
    return 1;
}

static uint32_t InputForFrame(long frame) {
    if (!s_tape || frame < 0 || (size_t)frame >= s_tape_frames) return 0;
    return (uint32_t)s_tape[frame];
}

/* ── Coverage hooks ────────────────────────────────────────────────────── */
static uint8_t *s_interp_seen = NULL;
static uint8_t *s_compiled_seen = NULL;
static unsigned long long s_interp_ops = 0;
static unsigned long s_interp_unique = 0;
static unsigned long long s_compiled_entries = 0;
static unsigned long s_compiled_unique = 0;

static void PcHook(uint32_t pc24, int m, int x) {
    (void)m; (void)x;
    pc24 &= 0xffffff;
    s_interp_ops++;
    if (!s_interp_seen[pc24]) {
        s_interp_seen[pc24] = 1;
        s_interp_unique++;
    }
}

static void BounceHook(uint32_t pc24, int m, int x) {
    pc24 &= 0xffffff;
    s_compiled_entries++;
    if (!s_compiled_seen[pc24]) {
        s_compiled_seen[pc24] = 1;
        s_compiled_unique++;
        if (getenv("WOTG_LOG_BOUNCE")) {
            fprintf(stderr, "[bounce] $%06X M%dX%d\n", pc24, m, x);
        }
    }
}

static uint8_t *ReadRomFile(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    size_t size = (size_t)sz;
    uint8_t *buf = (uint8_t *)malloc(size);
    if (!buf || fread(buf, 1, size, f) != size) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    if ((size % 1024) == 512) {
        memmove(buf, buf + 512, size - 512);
        size -= 512;
    }
    *out_size = size;
    return buf;
}

static void PrintHash(FILE *f, const uint8_t h[32]) {
    for (int i = 0; i < 32; i++) fprintf(f, "%02x", h[i]);
}

static void StemPath(char *out, size_t n, const char *png, long frame) {
    const char *dot = strrchr(png, '.');
    int stem = dot ? (int)(dot - png) : (int)strlen(png);
    snprintf(out, n, "%.*s_%06ld.png", stem, png, frame);
}

static void Usage(void) {
    fprintf(stderr,
        "usage: wotg_headless --rom <path.smc> [--frames N] [--dump-png path]\n"
        "                     [--dump-every K] [--input tape] [--log path]\n"
        "                     [--frame-log path] [--interp-targets file] [--lle]\n");
}

int main(int argc, char **argv) {
    const char *rom_path = NULL, *dump_png = NULL, *input_path = NULL, *log_path = NULL;
    const char *frame_log_path = NULL;
    long frames = 600, dump_every = 0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *v = i + 1 < argc ? argv[i + 1] : NULL;
        if (!strcmp(a, "--rom") && v) { rom_path = v; i++; }
        else if (!strcmp(a, "--frames") && v) { frames = strtol(v, NULL, 10); i++; }
        else if (!strcmp(a, "--dump-png") && v) { dump_png = v; i++; }
        else if (!strcmp(a, "--dump-every") && v) { dump_every = strtol(v, NULL, 10); i++; }
        else if (!strcmp(a, "--input") && v) { input_path = v; i++; }
        else if (!strcmp(a, "--frame-log") && v) { frame_log_path = v; i++; }
        else if (!strcmp(a, "--interp-targets") && v) { setenv("SNESRECOMP_LLE_INTERP_TARGET_FILE", v, 1); i++; }
        else if (!strcmp(a, "--lle")) { setenv("SNESRECOMP_LLE_BOUNCE", "0", 1); }
        else if ((!strcmp(a, "--log") || !strcmp(a, "--coverage-log")) && v) { log_path = v; i++; }
        else { Usage(); return 2; }
    }
    if (!rom_path) { Usage(); return 2; }
    if (frames < 1 || frames > 10000000) { fprintf(stderr, "bad --frames\n"); return 2; }

    size_t rom_size = 0;
    uint8_t *rom = ReadRomFile(rom_path, &rom_size);
    if (!rom) { fprintf(stderr, "cannot read ROM: %s\n", rom_path); return 2; }

    if (input_path && !LoadTape(input_path)) {
        fprintf(stderr, "cannot load tape: %s\n", input_path);
        return 2;
    }

    if (log_path) {
        setenv("SNESRECOMP_TIER2_CAPTURE", "1", 0);
    }

    CrcInit();
    s_interp_seen = (uint8_t *)calloc(1u << 24, 1);
    s_compiled_seen = (uint8_t *)calloc(1u << 24, 1);
    if (!s_interp_seen || !s_compiled_seen) { fprintf(stderr, "oom\n"); return 2; }
    g_interp_bridge_pc_hook = PcHook;
    g_interp_bridge_bounce_hook = BounceHook;

    RtlRegisterGame(&kWotgGameInfo);
    if (!SnesInit(rom, (int)rom_size)) {
        fprintf(stderr, "snesrecomp rejected the ROM\n");
        return 4;
    }
    tier2_capture_configure(0, -1, -1);

    FILE *frame_log = frame_log_path ? fopen(frame_log_path, "wb") : NULL;
    if (frame_log_path && !frame_log) { fprintf(stderr, "cannot write %s\n", frame_log_path); return 7; }

    unsigned long active_frames = 0;
    long frame = 0;
    for (frame = 0; frame < frames; frame++) {
        RtlRunFrame(InputForFrame(frame));
        if (frame_log) {
            uint32_t hw = 2166136261u;
            for (size_t k = 0; k < 0x20000; k++) hw = (hw ^ g_ram[k]) * 16777619u;
            uint32_t hv = 2166136261u;
            const uint8_t *vb = (const uint8_t *)g_snes->ppu->vram;
            for (size_t k = 0; k < sizeof g_snes->ppu->vram; k++) hv = (hv ^ vb[k]) * 16777619u;
            fprintf(frame_log, "%ld %08X %08X\n", frame + 1, (unsigned)hw, (unsigned)hv);
        }
        PpuBeginDrawing(g_ppu, s_pixels, kWidth * kBpp, kPpuRenderFlags_NewRenderer);
        kWotgGameInfo.draw_ppu_frame();
        if (g_fail) {
            fprintf(stderr, "runtime failure (g_fail) at frame %ld\n", frame);
            break;
        }
        int active = 0;
        for (size_t i = 0; i < sizeof s_pixels; i += 4) {
            if (s_pixels[i] | s_pixels[i + 1] | s_pixels[i + 2]) { active = 1; break; }
        }
        active_frames += active;

        if (dump_png && dump_every > 0 && (frame + 1) % dump_every == 0) {
            char p[1024];
            StemPath(p, sizeof p, dump_png, frame + 1);
            WritePng(p, s_pixels, kWidth, kHeight, kWidth * kBpp);
        }
    }
    if (dump_png && dump_every <= 0) {
        WritePng(dump_png, s_pixels, kWidth, kHeight, kWidth * kBpp);
    }

    long ran = frame < frames ? frame + 1 : frames;
    int t2_sites = 0;
    unsigned long long t2_clean = 0, t2_bail = 0;
    interp_tier2_stats(&t2_sites, &t2_clean, &t2_bail);
    long tier_downs = interp_tier_hit_count();

    uint8_t fh[32], wh[32];
    sha256_compute(s_pixels, sizeof s_pixels, fh);
    sha256_compute(g_ram, 0x20000, wh);

    printf("frames=%ld video_active_frames=%lu\n", ran, active_frames);
    printf("video_state inidisp=$%02x bgmode=$%02x main=$%02x sub=$%02x frame_counter=%d\n",
           g_ppu->inidisp, g_ppu->bgmode, g_ppu->screenEnabled[0],
           g_ppu->screenEnabled[1], snes_frame_counter);
    printf("frame_sha256="); PrintHash(stdout, fh);
    printf("\nwram_sha256="); PrintHash(stdout, wh);
    printf("\ninterp_opcodes=%llu interp_unique_pcs=%lu compiled_entries=%llu compiled_unique_entries=%lu\n",
           s_interp_ops, s_interp_unique, s_compiled_entries, s_compiled_unique);
    printf("tier_downs=%ld tier2_sites=%d tier2_clean=%llu tier2_bail=%llu\n",
           tier_downs, t2_sites, t2_clean, t2_bail);

    if (log_path) {
        FILE *lf = fopen(log_path, "w");
        if (lf) {
            fprintf(lf, "frames=%ld\nvideo_active_frames=%lu\ninterp_opcodes=%llu\ninterp_unique_pcs=%lu\n"
                        "compiled_entries=%llu\ncompiled_unique_entries=%lu\ntier_downs=%ld\n"
                        "tier2_sites=%d\ntier2_clean=%llu\ntier2_bail=%llu\n",
                    ran, active_frames, s_interp_ops, s_interp_unique,
                    s_compiled_entries, s_compiled_unique, tier_downs,
                    t2_sites, t2_clean, t2_bail);
            fclose(lf);
        }
    }

    if (tier2_capture_enabled()) {
        Tier2CoverageWriteDefaultManifest(kWotgGameInfo.title);
        tier2_capture_close();
    }
    if (frame_log) fclose(frame_log);
    free(s_interp_seen);
    free(s_compiled_seen);
    free(s_tape);
    free(rom);
    return g_fail ? 5 : 0;
}
