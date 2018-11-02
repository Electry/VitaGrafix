#include <vitasdk.h>
#include <taihen.h>

#include "tools.h"
#include "config.h"
#include "patch.h"
#include "main.h"


static uint8_t vgPatchIsGame(const char titleid[], const char self[], uint32_t nid) {
    VG_GameSupport supp = GAME_UNSUPPORTED;

    if (!strncmp(titleid, TITLEID_ANY, TITLEID_LEN) ||
            !strncmp(titleid, g_main.titleid, TITLEID_LEN)) {
        if (self[0] == '\0' || strstr(g_main.sceInfo.path, self)) {
            if (nid == NID_ANY || nid == g_main.info.module_nid) {
                supp = GAME_SUPPORTED;
            } else {
                supp = GAME_WRONG_VERSION;
            }
        } else {
            supp = GAME_SELF_SHELL;
        }
    }

    if (supp > g_main.support)
        g_main.support = supp;

    return supp == GAME_SUPPORTED;
}

static void vgPatchAddOffset(const char titleid[], const char self[], uint32_t nid, uint8_t num, ...) {
    if (vgPatchIsGame(titleid, self, nid)) {
        va_list va;
        va_start(va, num);
        for (uint8_t i = 0; i < num; i++) {
            g_main.offset[i] = va_arg(va, uint32_t);
        }
        va_end(va);
    }
}

int sceDisplaySetFrameBuf_withWait(const SceDisplayFrameBuf *pParam, int sync) {
    int ret = TAI_CONTINUE(int, g_main.hook_ref[0], pParam, sync);
    sceDisplayWaitVblankStartMulti(2);
    return ret;
}
int sceCtrlReadBufferPositive_peekPatched(int port, SceCtrlData *pad_data, int count) {
    return sceCtrlPeekBufferPositive(port, pad_data, count);
}
int sceCtrlReadBufferPositive2_peekPatched(int port, SceCtrlData *pad_data, int count) {
    return sceCtrlPeekBufferPositive2(port, pad_data, count);
}

void vgPatchGame() {
    //
    // Killzone Mercenary
    //
    if (vgPatchIsGame("PCSF00243", SELF_EBOOT, NID_ANY) || // EU [1.12]
            vgPatchIsGame("PCSF00403", SELF_EBOOT, NID_ANY) || // EU [1.12]
            vgPatchIsGame("PCSA00107", SELF_EBOOT, NID_ANY) || // US [1.12]
            vgPatchIsGame("PCSC00045", SELF_EBOOT, NID_ANY) || // JP [1.12]
            vgPatchIsGame("PCSD00071", SELF_EBOOT, NID_ANY)) { // ASIA [1.12]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_ENABLED);

        if (vgConfigIsIbEnabled()) {
            uint8_t nop_x4[8] = {0x00, 0xBF, 0x00, 0xBF, 0x00, 0xBF, 0x00, 0xBF};
            uint32_t data32_w_h_w_h[4] = {
                g_main.config.ib[0].width, g_main.config.ib[0].height,
                g_main.config.ib[0].width, g_main.config.ib[0].height
            };

            vgInjectData(0, 0x15A5C8, &nop_x4, sizeof(nop_x4));
            vgInjectData(1, 0xD728, &data32_w_h_w_h, sizeof(data32_w_h_w_h));
        }
        if (vgConfigIsFpsEnabled()) {
            uint32_t data32_vblank = g_main.config.fps == FPS_60 ? 0x1 : 0x2;

            vgInjectData(0, 0x9706A4, &data32_vblank, sizeof(data32_vblank));

            if (g_main.config.fps == FPS_60) {
                vgHookFunctionImport(0x67E7AB83, sceCtrlReadBufferPositive_peekPatched);
                vgHookFunctionImport(0xC4226A3E, sceCtrlReadBufferPositive2_peekPatched);
            }
        }
    }
    //
    // Persona 4 Golden
    //
    else if (vgPatchIsGame("PCSB00245", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE00120", SELF_EBOOT, NID_ANY) || // US
             vgPatchIsGame("PCSG00004", SELF_EBOOT, NID_ANY) || // JP [1.01]
             vgPatchIsGame("PCSG00563", SELF_EBOOT, NID_ANY) || // JP
             vgPatchIsGame("PCSH00021", SELF_EBOOT, NID_ANY)) { // ASIA
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            float float_w_h[2] = {(float)g_main.config.ib[0].width, (float)g_main.config.ib[0].height};

            vgPatchAddOffset("PCSB00245", SELF_EBOOT, NID_ANY, 1, 0xDBCFC);
            vgPatchAddOffset("PCSE00120", SELF_EBOOT, NID_ANY, 1, 0xDBCEC);
            vgPatchAddOffset("PCSG00004", SELF_EBOOT, NID_ANY, 1, 0xDBD9C);
            vgPatchAddOffset("PCSG00563", SELF_EBOOT, NID_ANY, 1, 0xDBD9C);
            vgPatchAddOffset("PCSH00021", SELF_EBOOT, NID_ANY, 1, 0xF1C50);

            vgInjectData(1, g_main.offset[0], &float_w_h, sizeof(float_w_h));
        }
    }
    //
    // WRC 3: FIA World Rally Championship
    //
    else if (vgPatchIsGame("PCSB00204", SELF_EBOOT, NID_ANY)) { // EU [1.01]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r5_width[4], movs_r6_height[4];
            vgMakeThumb2_T2_MOV(5, 1, g_main.config.ib[0].width, movs_r5_width);
            vgMakeThumb2_T2_MOV(6, 1, g_main.config.ib[0].height, movs_r6_height);

            vgInjectData(0, 0xAC430A, &movs_r5_width, sizeof(movs_r5_width));
            vgInjectData(0, 0xAC4310, &movs_r6_height, sizeof(movs_r6_height));
        }
    }
    //
    // WRC 4: FIA World Rally Championship
    //
    else if (vgPatchIsGame("PCSB00345", SELF_EBOOT, NID_ANY) || // EU [1.01]
             vgPatchIsGame("PCSE00411", SELF_EBOOT, NID_ANY)) { // US
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r0_width[4], movs_r4_height[4];
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[0].width, movs_r0_width);
            vgMakeThumb2_T2_MOV(4, 1, g_main.config.ib[0].height, movs_r4_height);

            vgPatchAddOffset("PCSB00345", SELF_EBOOT, NID_ANY, 2, 0xAC297C, 0xAC2982);
            vgPatchAddOffset("PCSE00411", SELF_EBOOT, NID_ANY, 2, 0xAC46C4, 0xAC46CA);

            vgInjectData(0, g_main.offset[0], &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, g_main.offset[1], &movs_r4_height, sizeof(movs_r4_height));
        }
    }
    //
    // God of War Collection
    //
    else if (vgPatchIsGame("PCSF00438", "GOW1.self", 0x8638ffed) || // EU
             vgPatchIsGame("PCSF00438", "GOW2.self", 0x6531f96a) ||
             vgPatchIsGame("PCSA00126", "GOW1.self", 0x126f65c5) || // US
             vgPatchIsGame("PCSA00126", "GOW2.self", 0x0064ec7e) ||
             vgPatchIsGame("PCSC00059", "GOW1.self", 0x990f8128) || // JP
             vgPatchIsGame("PCSC00059", "GOW2.self", 0x395a00f6)) {
        vgConfigSetSupported(FT_ENABLED, FT_UNSUPPORTED, FT_ENABLED);

        if (vgConfigIsFbEnabled()) {
            uint8_t movs_r0_width[4], movs_r4_width[4], movs_r7_width[4];
            uint8_t movs_r1_height[4], movs_r2_height[4], movs_lr_height[4];
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.fb.width, movs_r0_width);
            vgMakeThumb2_T2_MOV(4, 1, g_main.config.fb.width, movs_r4_width);
            vgMakeThumb2_T2_MOV(7, 1, g_main.config.fb.width, movs_r7_width);
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.fb.height, movs_r1_height);
            vgMakeThumb2_T2_MOV(2, 1, g_main.config.fb.height, movs_r2_height);
            vgMakeThumb2_T2_MOV(REGISTER_LR, 1, g_main.config.fb.height, movs_lr_height);

            vgPatchAddOffset("PCSF00438", "GOW1.self", 0x8638ffed, 5,
                    0x9E212, 0x9F0F0, 0xA31C6, 0xCEF06, 0xA1098); // EU (GOW1.self)
            vgPatchAddOffset("PCSF00438", "GOW2.self", 0x6531f96a, 5,
                    0xCDAE6, 0xCE9C4, 0xD2DBA, 0xFF782, 0xD0C8C); // EU (GOW2.self)
            vgPatchAddOffset("PCSA00126", "GOW1.self", 0x126f65c5, 5,
                    0x9E36E, 0x9F24C, 0xA3322, 0xCF062, 0xA11F4); // US (GOW1.self)
            vgPatchAddOffset("PCSA00126", "GOW2.self", 0x0064ec7e, 5,
                    0xCD9AE, 0xCE88C, 0xD2C82, 0xFF64A, 0xD0B54); // US (GOW2.self)
            vgPatchAddOffset("PCSC00059", "GOW1.self", 0x990f8128, 5,
                    0x9E1E6, 0x9F0C4, 0xA319A, 0xCEEDA, 0xA106C); // JP (GOW1.self)
            vgPatchAddOffset("PCSC00059", "GOW2.self", 0x395a00f6, 5,
                    0xCD7DA, 0xCE6B8, 0xD2AAE, 0xFF476, 0xD0980); // JP (GOW2.self)

            vgInjectData(0, g_main.offset[0], &movs_r4_width, sizeof(movs_r4_width));
            vgInjectData(0, g_main.offset[0] + 8, &movs_r2_height, sizeof(movs_r2_height));
            vgInjectData(0, g_main.offset[1], &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, g_main.offset[1] + 8, &movs_r1_height, sizeof(movs_r1_height));
            vgInjectData(0, g_main.offset[2], &movs_r7_width, sizeof(movs_r7_width));
            vgInjectData(0, g_main.offset[2] + 6, &movs_r1_height, sizeof(movs_r1_height));
            vgInjectData(0, g_main.offset[3], &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, g_main.offset[3] + 8, &movs_r2_height, sizeof(movs_r2_height));
            vgInjectData(0, g_main.offset[4], &movs_lr_height, sizeof(movs_lr_height));
        }
        if (vgConfigIsFpsEnabled()) {
            uint8_t mov_r0_vblank[2];
            vgMakeThumb_T1_MOV(0, g_main.config.fps == FPS_60 ? 1 : 2, mov_r0_vblank);

            vgPatchAddOffset("PCSF00438", "GOW1.self", 0x8638ffed, 1, 0x9E228); // EU (GOW1.self)
            vgPatchAddOffset("PCSF00438", "GOW2.self", 0x6531f96a, 1, 0xCDAFC); // EU (GOW2.self)
            vgPatchAddOffset("PCSA00126", "GOW1.self", 0x126f65c5, 1, 0x9E384); // US (GOW1.self)
            vgPatchAddOffset("PCSA00126", "GOW2.self", 0x0064ec7e, 1, 0xCD9C4); // US (GOW2.self)
            vgPatchAddOffset("PCSC00059", "GOW1.self", 0x990f8128, 1, 0x9E1FC); // JP (GOW1.self)
            vgPatchAddOffset("PCSC00059", "GOW2.self", 0x395a00f6, 1, 0xCD7F0); // JP (GOW2.self)

            vgInjectData(0, g_main.offset[0], &mov_r0_vblank, sizeof(mov_r0_vblank));
        }
    }
    //
    // MUD - FIM Motocross World Championship
    //
    else if (vgPatchIsGame("PCSB00182", SELF_EBOOT, NID_ANY)) { // EU
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r5_width[4], movs_r6_height[4];
            vgMakeThumb2_T2_MOV(5, 1, g_main.config.ib[0].width, movs_r5_width);
            vgMakeThumb2_T2_MOV(6, 1, g_main.config.ib[0].height, movs_r6_height);

            vgInjectData(0, 0x9B8B52, &movs_r5_width, sizeof(movs_r5_width));
            vgInjectData(0, 0x9B8B58, &movs_r6_height, sizeof(movs_r6_height));
        }
    }
    //
    // MXGP: The Official Motocross Videogame
    //
    else if (vgPatchIsGame("PCSB00470", SELF_EBOOT, NID_ANY) || // EU
            vgPatchIsGame("PCSE00530", SELF_EBOOT, NID_ANY)) {  // US
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r0_width[4], movs_r5_height[4];
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[0].width, movs_r0_width);
            vgMakeThumb2_T2_MOV(5, 1, g_main.config.ib[0].height, movs_r5_height);

            vgPatchAddOffset("PCSB00470", SELF_EBOOT, NID_ANY, 1, 0xB1D47A);
            vgPatchAddOffset("PCSE00530", SELF_EBOOT, NID_ANY, 1, 0xB1D36A);

            vgInjectData(0, g_main.offset[0], &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, g_main.offset[0] + 6, &movs_r5_height, sizeof(movs_r5_height));
        }
    }
    //
    // F1 2011
    //
    else if (vgPatchIsGame("PCSB00027", SELF_EBOOT, NID_ANY)) { // EU
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r1_w_r2_h[8];
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.ib[0].width, movs_r1_w_r2_h);
            vgMakeThumb2_T2_MOV(2, 1, g_main.config.ib[0].height, &movs_r1_w_r2_h[4]);

            vgInjectData(0, 0x10F0AA, &movs_r1_w_r2_h, sizeof(movs_r1_w_r2_h));
        }
    }
    //
    // LittleBigPlanet
    //
    else if (vgPatchIsGame("PCSF00021", SELF_EBOOT, NID_ANY) || // EU [1.22]
             vgPatchIsGame("PCSA00017", SELF_EBOOT, NID_ANY) || // US [1.22]
             vgPatchIsGame("PCSC00013", SELF_EBOOT, NID_ANY) || // JP [1.22]
             vgPatchIsGame("PCSD00006", SELF_EBOOT, NID_ANY)) { // ASIA [1.22]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r1_width[4], movs_r2_height[4];
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.ib[0].width, movs_r1_width);
            vgMakeThumb2_T2_MOV(2, 1, g_main.config.ib[0].height, movs_r2_height);

            vgInjectData(0, 0x168546, &movs_r1_width, sizeof(movs_r1_width));
            vgInjectData(0, 0x168546 + 4, &movs_r2_height, sizeof(movs_r2_height));
            vgInjectData(0, 0x16856A, &movs_r1_width, sizeof(movs_r1_width));
            vgInjectData(0, 0x16856A + 4, &movs_r2_height, sizeof(movs_r2_height));
            vgInjectData(0, 0x168582, &movs_r1_width, sizeof(movs_r1_width));
            vgInjectData(0, 0x168582 + 8, &movs_r2_height, sizeof(movs_r2_height));
            vgInjectData(0, 0x1685B0, &movs_r1_width, sizeof(movs_r1_width));
            vgInjectData(0, 0x1685B0 + 4, &movs_r2_height, sizeof(movs_r2_height));
        }
    }
    //
    // Borderlands 2
    //
    else if (vgPatchIsGame("PCSF00570", SELF_EBOOT, NID_ANY) || // EU [1.07]
             vgPatchIsGame("PCSF00576", SELF_EBOOT, NID_ANY) || // EU [1.07]
             vgPatchIsGame("PCSE00383", SELF_EBOOT, NID_ANY)) { // US [1.09]
        vgConfigSetSupported(FT_ENABLED, FT_UNSUPPORTED, FT_UNSUPPORTED);

        if (vgConfigIsFbEnabled()) {
            uint32_t data32_w_h[2] = {g_main.config.fb.width, g_main.config.fb.height};
            vgInjectData(1, 0x24A94, &data32_w_h, sizeof(data32_w_h));
        }
    }
    //
    // Asphalt: Injection
    //
    else if (vgPatchIsGame("PCSB00040", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE00007", SELF_EBOOT, NID_ANY)) { // US
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint32_t data32_w_h[2] = {g_main.config.ib[0].width, g_main.config.ib[0].height};
            vgInjectData(1, 0x5A2C, &data32_w_h, sizeof(data32_w_h));
        }
    }
    //
    // LEGO Star Wars: The Force Awakens
    //
    else if (vgPatchIsGame("PCSB00877", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE00791", SELF_EBOOT, NID_ANY)) { // US
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r4_width[4], movs_r5_height[4];
            uint32_t data32_w_h[2] = {g_main.config.ib[0].width, g_main.config.ib[0].height};
            vgMakeThumb2_T2_MOV(4, 1, g_main.config.ib[0].width, movs_r4_width);
            vgMakeThumb2_T2_MOV(5, 1, g_main.config.ib[0].height, movs_r5_height);

            vgPatchAddOffset("PCSB00877", SELF_EBOOT, NID_ANY, 3, 0x1F313E, 0x4650, 0x2241C4);
            vgPatchAddOffset("PCSE00791", SELF_EBOOT, NID_ANY, 3, 0x1F315E, 0x4508, 0x2241E4);

            if (g_main.config.ib[0].width > 640 || g_main.config.ib[0].height > 368) {
                uint8_t movs_r1_parambufsize[4];
                vgMakeThumb2_T2_MOV(1, 1, (10 * 1024 * 1024), movs_r1_parambufsize);

                vgInjectData(0, g_main.offset[2], &movs_r1_parambufsize, sizeof(movs_r1_parambufsize));
            }

            vgInjectData(0, g_main.offset[0], &movs_r4_width, sizeof(movs_r4_width));
            vgInjectData(0, g_main.offset[0] + 6, &movs_r5_height, sizeof(movs_r5_height));
            vgInjectData(1, g_main.offset[1], &data32_w_h, sizeof(data32_w_h));
        }
    }
    //
    // World of Final Fantasy
    //
    else if (vgPatchIsGame("PCSB00951", SELF_EBOOT, NID_ANY) || // EU [1.03]
             vgPatchIsGame("PCSE00880", SELF_EBOOT, NID_ANY) || // US [1.03]
             vgPatchIsGame("PCSH00223", SELF_EBOOT, NID_ANY) || // ASIA [1.03]
             vgPatchIsGame("PCSG00709", SELF_EBOOT, NID_ANY)) { // JP [?]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r5_width[4], movs_r0_height[4];
            vgMakeThumb2_T2_MOV(5, 1, g_main.config.ib[0].width, movs_r5_width);
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[0].height, movs_r0_height);

            vgPatchAddOffset("PCSB00951", SELF_EBOOT, NID_ANY, 2, 0x22C9E6, 0x429568);
            vgPatchAddOffset("PCSE00880", SELF_EBOOT, NID_ANY, 2, 0x22C9FE, 0x429580);
            vgPatchAddOffset("PCSH00223", SELF_EBOOT, NID_ANY, 2, 0x22CA16, 0x429598);
            vgPatchAddOffset("PCSG00709", SELF_EBOOT, NID_ANY, 2, 0x22C9DE, 0x429560);

            if (g_main.config.ib[0].width > 640 || g_main.config.ib[0].height > 384) {
                uint8_t movs_r1_uncache[4];
                vgMakeThumb2_T2_MOV(1, 1, 0x5800000 - (4 * 1024 * 1024), movs_r1_uncache);

                vgInjectData(0, g_main.offset[1], &movs_r1_uncache, sizeof(movs_r1_uncache));
            }

            vgInjectData(0, g_main.offset[0], &movs_r5_width, sizeof(movs_r5_width));
            vgInjectData(0, g_main.offset[0] + 6, &movs_r0_height, sizeof(movs_r0_height));
        }
    }
    //
    // Ridge Racer
    //
    else if (vgPatchIsGame("PCSB00048", SELF_EBOOT, NID_ANY) || // EU [1.02]
             vgPatchIsGame("PCSE00001", SELF_EBOOT, NID_ANY) || // US [1.02]
             vgPatchIsGame("PCSG00001", SELF_EBOOT, NID_ANY)) { // JP [1.04]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint32_t data32_w_h[2] = {g_main.config.ib[0].width, g_main.config.ib[0].height};

            vgInjectData(1, 0x53E4, &data32_w_h, sizeof(data32_w_h));
        }
    }
    //
    // Utawarerumono: Mask of Deception
    //
    else if (vgPatchIsGame("PCSB01093", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE00959", SELF_EBOOT, NID_ANY)) { // US
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t mov_r1_width[4], mov_r0_height[4];
            vgMakeArm_A1_MOV(1, 0, g_main.config.ib[0].width, mov_r1_width);
            vgMakeArm_A1_MOV(0, 0, g_main.config.ib[0].height, mov_r0_height);

            vgPatchAddOffset("PCSB01093", SELF_EBOOT, NID_ANY, 2, 0x119AA0, 0x119AB8);
            vgPatchAddOffset("PCSE00959", SELF_EBOOT, NID_ANY, 2, 0x11D1BC, 0x11D1D4);

            vgInjectData(0, g_main.offset[0], &mov_r1_width, sizeof(mov_r1_width));
            vgInjectData(0, g_main.offset[1], &mov_r0_height, sizeof(mov_r0_height));
        }
    }
    //
    // Utawarerumono: Itsuwari no Kamen
    //
    else if (vgPatchIsGame("PCSG00617", SELF_EBOOT, NID_ANY)) { // JP [1.02]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t mov_r0_w_r1_h[8];
            vgMakeArm_A1_MOV(0, 0, g_main.config.ib[0].width, mov_r0_w_r1_h);
            vgMakeArm_A1_MOV(1, 0, g_main.config.ib[0].height, &mov_r0_w_r1_h[4]);

            vgInjectData(0, 0x119058, &mov_r0_w_r1_h, sizeof(mov_r0_w_r1_h));
            vgInjectData(0, 0x11907C, &mov_r0_w_r1_h, sizeof(mov_r0_w_r1_h));
        }
    }
    //
    // Dead or Alive 5 Plus
    //
    else if (vgPatchIsGame("PCSB00296", SELF_EBOOT, NID_ANY) || // EU [1.01]
             vgPatchIsGame("PCSE00235", SELF_EBOOT, NID_ANY) || // US [1.01]
             vgPatchIsGame("PCSG00167", SELF_EBOOT, NID_ANY)) { // JP [1.01]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r1_width[4], movs_r1_height[4];
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.ib[0].width, movs_r1_width);
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.ib[0].height, movs_r1_height);

            vgInjectData(0, 0x5B0DC4, &movs_r1_width, sizeof(movs_r1_width));
            vgInjectData(0, 0x5B0DC4 + 6, &movs_r1_height, sizeof(movs_r1_height));
            vgInjectData(0, 0x5B0DD0, &movs_r1_width, sizeof(movs_r1_width));
            vgInjectData(0, 0x5B0DD0 + 6, &movs_r1_height, sizeof(movs_r1_height));
        }
    }
    //
    // Miracle Girls Festival
    //
    else if (vgPatchIsGame("PCSG00610", SELF_EBOOT, NID_ANY)) { // JP
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r2_width[4], movs_r3_width[4], movs_r7_width[4];
            uint8_t movs_r3_height[4], movs_r4_height[4], movs_lr_height[4];
            vgMakeThumb2_T2_MOV(2, 1, g_main.config.ib[0].width, movs_r2_width);
            vgMakeThumb2_T2_MOV(3, 1, g_main.config.ib[0].height, movs_r3_height);
            vgMakeThumb2_T2_MOV(4, 1, g_main.config.ib[0].height, movs_r4_height);
            vgMakeThumb2_T2_MOV(3, 1, g_main.config.ib[0].width, movs_r3_width);
            vgMakeThumb2_T2_MOV(REGISTER_LR, 1, g_main.config.ib[0].height, movs_lr_height);
            vgMakeThumb2_T2_MOV(7, 1, g_main.config.ib[0].width, movs_r7_width);

            vgInjectData(0, 0x158EB0, &movs_r2_width, sizeof(movs_r2_width));
            vgInjectData(0, 0x158EB0 + 8, &movs_r3_height, sizeof(movs_r3_height));
            vgInjectData(0, 0x159EAE, &movs_r2_width, sizeof(movs_r2_width));
            vgInjectData(0, 0x159EAE + 8, &movs_r4_height, sizeof(movs_r4_height));
            vgInjectData(0, 0x15D32A, &movs_r3_width, sizeof(movs_r3_width));
            vgInjectData(0, 0x15D32A + 4, &movs_lr_height, sizeof(movs_lr_height));
            vgInjectData(0, 0x15D6C4, &movs_r7_width, sizeof(movs_r7_width));
            vgInjectData(0, 0x15D6C4 + 4, &movs_lr_height, sizeof(movs_lr_height));
        }
    }
    //
    // The Jak and Daxter Trilogy / Jak and Daxter Collection
    //
    else if (vgPatchIsGame("PCSF00248", SELF_EBOOT, NID_ANY) ||  // EU (Jak1)
             vgPatchIsGame("PCSF00249", SELF_EBOOT, NID_ANY) ||  // EU (Jak2)
             vgPatchIsGame("PCSF00250", SELF_EBOOT, NID_ANY) ||  // EU (Jak3)
             vgPatchIsGame("PCSF00247", "Jak1.self", 0x109d6ad5) || // EU (Jak1.self)
             vgPatchIsGame("PCSF00247", "Jak2.self", 0x15059015) || // EU (Jak2.self)
             vgPatchIsGame("PCSF00247", "Jak3.self", 0x790ebad9) || // EU (Jak3.self)
             vgPatchIsGame("PCSA00080", "Jak1.self", 0x109d6ad5) || // US (Jak1.self)
             vgPatchIsGame("PCSA00080", "Jak2.self", 0x15059015) || // US (Jak2.self)
             vgPatchIsGame("PCSA00080", "Jak3.self", 0x790ebad9)) { // US (Jak3.self)
        vgConfigSetSupported(FT_ENABLED, FT_UNSUPPORTED, FT_UNSUPPORTED);

        if (vgConfigIsFbEnabled()) {
            uint8_t movs_r0_width[4], movs_r0_height[4];
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.fb.width, movs_r0_width);
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.fb.height, movs_r0_height);

            vgPatchAddOffset("PCSF00248", SELF_EBOOT, NID_ANY, 2, 0x1B250, 0x1B260);  // EU (Jak1)
            vgPatchAddOffset("PCSF00249", SELF_EBOOT, NID_ANY, 2, 0x211F2, 0x211FA);  // EU (Jak2)
            vgPatchAddOffset("PCSF00250", SELF_EBOOT, NID_ANY, 2, 0x26096, 0x2609E);  // EU (Jak3)
            vgPatchAddOffset("PCSF00247", "Jak1.self", 0x109d6ad5, 2, 0x1B250, 0x1B260); // EU (Jak1.self)
            vgPatchAddOffset("PCSF00247", "Jak2.self", 0x15059015, 2, 0x211F2, 0x211FA); // EU (Jak2.self)
            vgPatchAddOffset("PCSF00247", "Jak3.self", 0x790ebad9, 2, 0x26096, 0x2609E); // EU (Jak3.self)
            vgPatchAddOffset("PCSA00080", "Jak1.self", 0x109d6ad5, 2, 0x1B250, 0x1B260); // US (Jak1.self)
            vgPatchAddOffset("PCSA00080", "Jak2.self", 0x15059015, 2, 0x211F2, 0x211FA); // US (Jak2.self)
            vgPatchAddOffset("PCSA00080", "Jak3.self", 0x790ebad9, 2, 0x26096, 0x2609E); // US (Jak3.self)

            vgInjectData(0, g_main.offset[0], &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, g_main.offset[1], &movs_r0_height, sizeof(movs_r0_height));
        }
    }
    //
    // Hatsune Miku: Project Diva f
    //
    else if (vgPatchIsGame("PCSB00419", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE00326", SELF_EBOOT, NID_ANY) || // US
             vgPatchIsGame("PCSG00074", SELF_EBOOT, NID_ANY)) { // JP [1.01]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_ENABLED);

        if (vgConfigIsIbEnabled()) {
            uint8_t mov_r1_width[4], mov_r2_height[4];
            uint8_t movs_r2_width[4], movs_r3_height[4];
            vgMakeThumb2_T2_MOV(1, 0, g_main.config.ib[0].width, mov_r1_width);
            vgMakeThumb2_T2_MOV(2, 0, g_main.config.ib[0].height, mov_r2_height);
            vgMakeThumb2_T2_MOV(2, 1, g_main.config.ib[0].width, movs_r2_width);
            vgMakeThumb2_T2_MOV(3, 1, g_main.config.ib[0].height, movs_r3_height);

            vgPatchAddOffset("PCSB00419", SELF_EBOOT, NID_ANY, 3, 0x2828C4, 0x2828CA, 0x28207C);
            vgPatchAddOffset("PCSE00326", SELF_EBOOT, NID_ANY, 3, 0x2828C4, 0x2828CA, 0x28207C);
            vgPatchAddOffset("PCSG00074", SELF_EBOOT, NID_ANY, 3, 0x257AC0, 0x257ABA, 0x257150);

            vgInjectData(0, g_main.offset[0], &mov_r1_width, sizeof(mov_r1_width));
            vgInjectData(0, g_main.offset[1], &mov_r2_height, sizeof(mov_r2_height));
            vgInjectData(0, g_main.offset[2], &movs_r2_width, sizeof(movs_r2_width));
            vgInjectData(0, g_main.offset[2] + 8, &movs_r3_height, sizeof(movs_r3_height));
        }
        if (vgConfigIsFpsEnabled()) {
            uint8_t movs_r0_vblank[2];
            uint8_t vmov_s1_1float[4] = {0xF7, 0xEE, 0x00, 0x0A};
            vgMakeThumb_T1_MOV(0, g_main.config.fps == FPS_60 ? 0x1 : 0x2, movs_r0_vblank);

            vgPatchAddOffset("PCSB00419", SELF_EBOOT, NID_ANY, 2, 0x136C4, 0x13948);
            vgPatchAddOffset("PCSE00326", SELF_EBOOT, NID_ANY, 2, 0x136C4, 0x13948);
            vgPatchAddOffset("PCSG00074", SELF_EBOOT, NID_ANY, 2, 0x12582, 0x12814);

            vgInjectData(0, g_main.offset[0], &movs_r0_vblank, sizeof(movs_r0_vblank));
            if (g_main.config.fps == FPS_60) // patch PV anim speed
                vgInjectData(0, g_main.offset[1], &vmov_s1_1float, sizeof(vmov_s1_1float));
        }
    }
    //
    // Hatsune Miku: Project Diva f 2nd
    //
    else if (vgPatchIsGame("PCSB00554", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE00434", SELF_EBOOT, NID_ANY) || // US
             vgPatchIsGame("PCSG00205", SELF_EBOOT, NID_ANY) || // JP [1.01]
             vgPatchIsGame("PCSH00088", SELF_EBOOT, NID_ANY)) { // ASIA
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_ENABLED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r3_width[4], movs_r4_height[4];
            uint8_t movs_r5_width[4], movs_r6_height[4];
            vgMakeThumb2_T2_MOV(3, 0, g_main.config.ib[0].width, movs_r3_width);
            vgMakeThumb2_T2_MOV(4, 0, g_main.config.ib[0].height, movs_r4_height);
            vgMakeThumb2_T2_MOV(5, 1, g_main.config.ib[0].width, movs_r5_width);
            vgMakeThumb2_T2_MOV(6, 1, g_main.config.ib[0].height, movs_r6_height);

            vgPatchAddOffset("PCSB00554", SELF_EBOOT, NID_ANY, 2, 0x25E34A, 0x25D500);
            vgPatchAddOffset("PCSE00434", SELF_EBOOT, NID_ANY, 2, 0x25E34A, 0x25D500);
            vgPatchAddOffset("PCSG00205", SELF_EBOOT, NID_ANY, 2, 0x25A6B6, 0x25986C);
            vgPatchAddOffset("PCSH00088", SELF_EBOOT, NID_ANY, 2, 0x25E4CA, 0x25D680);

            vgInjectData(0, g_main.offset[0], &movs_r3_width, sizeof(movs_r3_width));
            vgInjectData(0, g_main.offset[0] + 8, &movs_r4_height, sizeof(movs_r4_height));
            vgInjectData(0, g_main.offset[1], &movs_r5_width, sizeof(movs_r5_width));
            vgInjectData(0, g_main.offset[1] + 8, &movs_r6_height, sizeof(movs_r6_height));
        }
        if (vgConfigIsFpsEnabled()) {
            uint8_t movs_r0_vblank[2];
            uint8_t vmov_s1_1float[4] = {0xF7, 0xEE, 0x00, 0x0A};
            vgMakeThumb_T1_MOV(0, g_main.config.fps == FPS_60 ? 0x1 : 0x2, movs_r0_vblank);

            vgPatchAddOffset("PCSB00554", SELF_EBOOT, NID_ANY, 2, 0xA994, 0xAAF2);
            vgPatchAddOffset("PCSE00434", SELF_EBOOT, NID_ANY, 2, 0xA994, 0xAAF2);
            vgPatchAddOffset("PCSG00205", SELF_EBOOT, NID_ANY, 2, 0xA95C, 0xAABA);
            vgPatchAddOffset("PCSH00088", SELF_EBOOT, NID_ANY, 2, 0xA994, 0xAAF2);

            vgInjectData(0, g_main.offset[0], &movs_r0_vblank, sizeof(movs_r0_vblank));
            if (g_main.config.fps == FPS_60) // patch PV anim speed
                vgInjectData(0, g_main.offset[1], &vmov_s1_1float, sizeof(vmov_s1_1float));
        }
    }
    //
    // Hatsune Miku: Project Diva X
    //
    else if (vgPatchIsGame("PCSB01007", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE00867", SELF_EBOOT, NID_ANY) || // US
             vgPatchIsGame("PCSH00176", SELF_EBOOT, NID_ANY)) { // ASIA
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r2_width[4], movs_r3_height[4];
            uint8_t movs_lr_width[4], movs_r12_height[4];
            vgMakeThumb2_T2_MOV(2, 1, g_main.config.ib[0].width, movs_r2_width);
            vgMakeThumb2_T2_MOV(3, 1, g_main.config.ib[0].height, movs_r3_height);
            vgMakeThumb2_T2_MOV(REGISTER_LR, 1, g_main.config.ib[0].width, movs_lr_width);
            vgMakeThumb2_T2_MOV(12, 1, g_main.config.ib[0].height, movs_r12_height);

            vgPatchAddOffset("PCSB01007", SELF_EBOOT, NID_ANY, 2, 0x2643BA, 0x2653D2);
            vgPatchAddOffset("PCSE00867", SELF_EBOOT, NID_ANY, 2, 0x2643BA, 0x2653D2);
            vgPatchAddOffset("PCSH00176", SELF_EBOOT, NID_ANY, 2, 0x230D5A, 0x231A86);

            vgInjectData(0, g_main.offset[0], &movs_r2_width, sizeof(movs_r2_width));
            vgInjectData(0, g_main.offset[0] + 6, &movs_r3_height, sizeof(movs_r3_height));
            vgInjectData(0, g_main.offset[1], &movs_lr_width, sizeof(movs_lr_width));
            vgInjectData(0, g_main.offset[1] + 6, &movs_r12_height, sizeof(movs_r12_height));
        }
    }
    //
    // MotoGP 13
    //
    else if (vgPatchIsGame("PCSB00316", SELF_EBOOT, NID_ANY) || // EU [1.02]
             vgPatchIsGame("PCSE00409", SELF_EBOOT, NID_ANY)) { // US
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r0_width[4], movs_r5_height[4];
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[0].width, movs_r0_width);
            vgMakeThumb2_T2_MOV(5, 1, g_main.config.ib[0].height, movs_r5_height);

            vgPatchAddOffset("PCSB00316", SELF_EBOOT, NID_ANY, 1, 0xAA57A4);
            vgPatchAddOffset("PCSE00409", SELF_EBOOT, NID_ANY, 1, 0xAA622C);

            vgInjectData(0, g_main.offset[0], &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, g_main.offset[0] + 6, &movs_r5_height, sizeof(movs_r5_height));
        }
    }
    //
    // MotoGP 14
    //
    else if (vgPatchIsGame("PCSE00529", SELF_EBOOT, NID_ANY)) { // US
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r0_width[4], movs_r4_height[4];
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[0].width, movs_r0_width);
            vgMakeThumb2_T2_MOV(4, 1, g_main.config.ib[0].height, movs_r4_height);

            vgInjectData(0, 0x52B5EA, &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, 0x52B5EA + 6, &movs_r4_height, sizeof(movs_r4_height));
        }
    }
    //
    // WRC 5: FIA World Rally Championship
    //
    else if (vgPatchIsGame("PCSB00762", SELF_EBOOT, NID_ANY)) { // EU
        vgConfigSetSupported(FT_ENABLED, FT_UNSUPPORTED, FT_UNSUPPORTED);

        if (vgConfigIsFbEnabled()) {
            uint8_t nop[2] = {0x00, 0xBF};
            uint8_t nop_nop[4] = {0x00, 0xBF, 0x00, 0xBF};
            uint8_t movs_r0_width[4], mov_r6_height[4];
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.fb.width, movs_r0_width);
            vgMakeThumb2_T2_MOV(6, 0, g_main.config.fb.height, mov_r6_height);

            vgInjectData(0, 0x18C002, &nop_nop, sizeof(nop_nop));
            vgInjectData(0, 0x18C04E, &nop, sizeof(nop));
            vgInjectData(0, 0x18C010, &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, 0x18C022, &mov_r6_height, sizeof(mov_r6_height));
        }
    }
    //
    // Utawarerumono: Mask of Truth / Futari no Hakuoro
    //
    else if (vgPatchIsGame("PCSB01145", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE01102", SELF_EBOOT, NID_ANY) || // US
             vgPatchIsGame("PCSG00838", SELF_EBOOT, NID_ANY)) { // JP [1.04]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t mov_r0_width[4], mov_r1_height[4];
            vgMakeArm_A1_MOV(0, 0, g_main.config.ib[0].width, mov_r0_width);
            vgMakeArm_A1_MOV(1, 0, g_main.config.ib[0].height, mov_r1_height);

            vgPatchAddOffset("PCSB01145", SELF_EBOOT, NID_ANY, 1, 0x15143C);
            vgPatchAddOffset("PCSE01102", SELF_EBOOT, NID_ANY, 1, 0x154AA8);
            vgPatchAddOffset("PCSG00838", SELF_EBOOT, NID_ANY, 1, 0x152698);

            vgInjectData(0, g_main.offset[0], &mov_r0_width, sizeof(mov_r0_width));
            vgInjectData(0, g_main.offset[0] + 8, &mov_r1_height, sizeof(mov_r1_height));
        }
    }
    //
    // Dragon Quest Builders
    //
    else if (vgPatchIsGame("PCSB00981", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE00912", SELF_EBOOT, NID_ANY) || // US
             vgPatchIsGame("PCSG00697", SELF_EBOOT, NID_ANY) || // JP [1.03]
             vgPatchIsGame("PCSH00221", SELF_EBOOT, NID_ANY)) { // ASIA
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_ENABLED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r0_width[4], movs_r3_height[4];
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[0].width, movs_r0_width);
            vgMakeThumb2_T2_MOV(3, 1, g_main.config.ib[0].height, movs_r3_height);

            vgPatchAddOffset("PCSB00981", SELF_EBOOT, NID_ANY, 1, 0x271F62);
            vgPatchAddOffset("PCSE00912", SELF_EBOOT, NID_ANY, 1, 0x271F5E);
            vgPatchAddOffset("PCSG00697", SELF_EBOOT, NID_ANY, 1, 0x26AC1E);
            vgPatchAddOffset("PCSH00221", SELF_EBOOT, NID_ANY, 1, 0x26BD42);

            vgInjectData(0, g_main.offset[0], &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, g_main.offset[0] - 6, &movs_r3_height, sizeof(movs_r3_height));
        }
        if (vgConfigIsFpsEnabled()) {
            if (g_main.config.fps == FPS_30)
                vgHookFunctionImport(0x7A410B64, sceDisplaySetFrameBuf_withWait);
        }
    }
    //
    // The Amazing Spider-Man
    //
    else if (vgPatchIsGame("PCSB00428", SELF_EBOOT, NID_ANY)) { // EU
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_ENABLED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r1_w_r0_h[4];
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.ib[0].width, movs_r1_w_r0_h);
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[0].height, &movs_r1_w_r0_h[4]);

            vgInjectData(0, 0x16CD7E, &movs_r1_w_r0_h, sizeof(movs_r1_w_r0_h));
        }
        if (vgConfigIsFpsEnabled()) {
            if (g_main.config.fps == FPS_30)
                vgHookFunctionImport(0x7A410B64, sceDisplaySetFrameBuf_withWait);
        }
    }
    //
    // Sly Cooper / The Sly Trilogy
    //
    else if (vgPatchIsGame("PCSF00269", SELF_EBOOT, NID_ANY) ||     // EU (Sly Cooper and the Thievius Raccoonus)
             vgPatchIsGame("PCSF00270", SELF_EBOOT, NID_ANY) ||     // EU (Sly Cooper 2: Band of Thieves)
             vgPatchIsGame("PCSF00271", SELF_EBOOT, NID_ANY) ||     // EU (Sly Cooper 3: Honor Among Thieves)
             vgPatchIsGame("PCSF00338", "Sly1.self", 0x15bca5ba) || // EU
             vgPatchIsGame("PCSF00338", "Sly1.self", 0x7288e791) || // EU
             vgPatchIsGame("PCSA00096", SELF_EBOOT, NID_ANY) ||     // US (Sly Cooper and the Thievius Raccoonus)
             vgPatchIsGame("PCSA00097", SELF_EBOOT, NID_ANY) ||     // US (Sly Cooper 2: Band of Thieves)
             vgPatchIsGame("PCSA00098", SELF_EBOOT, NID_ANY) ||     // US (Sly Cooper 3: Honor Among Thieves)
             vgPatchIsGame("PCSA00095", "Sly1.self", 0x605d1db1) || // US
             vgPatchIsGame("PCSA00095", "Sly2.self", 0xdcd6b8bc)) { // US
        vgConfigSetSupported(FT_ENABLED, FT_UNSUPPORTED, FT_ENABLED);

        if (vgConfigIsFbEnabled()) {
            uint8_t movs_r1_width[4], movs_r1_height[4];
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.fb.width, movs_r1_width);
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.fb.height, movs_r1_height);

            vgPatchAddOffset("PCSF00269", SELF_EBOOT, NID_ANY, 1, 0xE0A40);
            vgPatchAddOffset("PCSF00270", SELF_EBOOT, NID_ANY, 1, 0x1155FC);
            vgPatchAddOffset("PCSF00271", SELF_EBOOT, NID_ANY, 1, 0x1692AC);
            vgPatchAddOffset("PCSF00338", "Sly1.self", 0x15bca5ba, 1, 0xE0A40);
            vgPatchAddOffset("PCSF00338", "Sly2.self", 0x7288e791, 1, 0x1155FC);
            vgPatchAddOffset("PCSA00096", SELF_EBOOT, NID_ANY, 1, 0xE0AF8);
            vgPatchAddOffset("PCSA00097", SELF_EBOOT, NID_ANY, 1, 0x1155F8);
            vgPatchAddOffset("PCSA00098", SELF_EBOOT, NID_ANY, 1, 0x1692A8);
            vgPatchAddOffset("PCSA00095", "Sly1.self", 0x605d1db1, 1, 0xE0AF8);
            vgPatchAddOffset("PCSA00095", "Sly2.self", 0xdcd6b8bc, 1, 0x1155F8);

            vgInjectData(0, g_main.offset[0], &movs_r1_width, sizeof(movs_r1_width));
            vgInjectData(0, g_main.offset[0] + 6, &movs_r1_height, sizeof(movs_r1_height));
        }
        if (vgConfigIsFpsEnabled()) {
            uint8_t movs_r0_vblank[2];
            vgMakeThumb_T1_MOV(0, g_main.config.fps == FPS_60 ? 1 : 2, movs_r0_vblank);

            vgPatchAddOffset("PCSF00269", SELF_EBOOT, NID_ANY, 1, 0x10C04C);
            vgPatchAddOffset("PCSF00270", SELF_EBOOT, NID_ANY, 1, 0x12E63C);
            vgPatchAddOffset("PCSF00271", SELF_EBOOT, NID_ANY, 1, 0x1822EC);
            vgPatchAddOffset("PCSF00338", "Sly1.self", 0x15bca5ba, 1, 0x10C04C);
            vgPatchAddOffset("PCSF00338", "Sly2.self", 0x7288e791, 1, 0x12E63C);
            vgPatchAddOffset("PCSA00096", SELF_EBOOT, NID_ANY, 1, 0x10C104);
            vgPatchAddOffset("PCSA00097", SELF_EBOOT, NID_ANY, 1, 0x12E638);
            vgPatchAddOffset("PCSA00098", SELF_EBOOT, NID_ANY, 1, 0x1822E8);
            vgPatchAddOffset("PCSA00095", "Sly1.self", 0x605d1db1, 1, 0x10C104);
            vgPatchAddOffset("PCSA00095", "Sly2.self", 0xdcd6b8bc, 1, 0x12E638);

            vgInjectData(0, g_main.offset[0], &movs_r0_vblank, sizeof(movs_r0_vblank));
        }
    }
    //
    // Sly Cooper: Thieves in Time
    //
    else if (vgPatchIsGame("PCSF00156", SELF_EBOOT, NID_ANY) || // EU [1.01]
             vgPatchIsGame("PCSF00206", SELF_EBOOT, NID_ANY) || // EU [1.01]
             vgPatchIsGame("PCSF00207", SELF_EBOOT, NID_ANY) || // EU [1.01]
             vgPatchIsGame("PCSF00208", SELF_EBOOT, NID_ANY) || // EU [1.01]
             vgPatchIsGame("PCSF00209", SELF_EBOOT, NID_ANY) || // EU [1.01]
             vgPatchIsGame("PCSA00068", SELF_EBOOT, NID_ANY)) { // US [1.01]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_UNSUPPORTED, FT_ENABLED);

        if (vgConfigIsFpsEnabled()) {
            uint8_t movs_r0_vblank[2];
            vgMakeThumb_T1_MOV(0, g_main.config.fps == FPS_60 ? 1 : 2, movs_r0_vblank);

            vgPatchAddOffset("PCSF00156", SELF_EBOOT, NID_ANY, 1, 0x3228AC);
            vgPatchAddOffset("PCSF00206", SELF_EBOOT, NID_ANY, 1, 0x3228AC);
            vgPatchAddOffset("PCSF00207", SELF_EBOOT, NID_ANY, 1, 0x3228AC);
            vgPatchAddOffset("PCSF00208", SELF_EBOOT, NID_ANY, 1, 0x3228AC);
            vgPatchAddOffset("PCSF00209", SELF_EBOOT, NID_ANY, 1, 0x3228AC);
            vgPatchAddOffset("PCSA00068", SELF_EBOOT, NID_ANY, 1, 0x3228E4);

            vgInjectData(0, g_main.offset[0], &movs_r0_vblank, sizeof(movs_r0_vblank));
        }
    }
    //
    // The Ratchet & Clank Trilogy / Ratchet & Clank Collection
    //
    else if (vgPatchIsGame("PCSF00484", SELF_EBOOT, NID_ANY) ||    // EU (Ratchet & Clank 1)
             vgPatchIsGame("PCSF00482", "rc1.self", 0x0a02a884) || // EU
             vgPatchIsGame("PCSA00133", "rc1.self", 0x0a02a884) || // US
             vgPatchIsGame("PCSF00485", SELF_EBOOT, NID_ANY) ||    // EU (Ratchet & Clank 2)
             vgPatchIsGame("PCSF00482", "rc2.self", 0x7a1d621c) || // EU
             vgPatchIsGame("PCSA00133", "rc2.self", 0x7a1d621c) || // US
             vgPatchIsGame("PCSF00486", SELF_EBOOT, NID_ANY) ||    // EU (Ratchet & Clank 3)
             vgPatchIsGame("PCSF00482", "rc3.self", 0xcf835e57) || // EU
             vgPatchIsGame("PCSA00133", "rc3.self", 0xcf835e57)) { // US
        vgConfigSetSupported(FT_ENABLED, FT_UNSUPPORTED, FT_UNSUPPORTED);

        if (vgConfigIsFbEnabled()) {
            uint8_t movs_r0_width[4], movs_r1_height[4];
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.fb.width, movs_r0_width);
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.fb.height, movs_r1_height);

            vgPatchAddOffset("PCSF00484", SELF_EBOOT, NID_ANY, 2, 0x1A024, 0x2D12);
            vgPatchAddOffset("PCSF00482", "rc1.self", 0x0a02a884, 2, 0x1A024, 0x2D12);
            vgPatchAddOffset("PCSA00133", "rc1.self", 0x0a02a884, 2, 0x1A024, 0x2D12);
            vgPatchAddOffset("PCSF00485", SELF_EBOOT, NID_ANY, 2, 0x19054, 0x1D62);
            vgPatchAddOffset("PCSF00482", "rc2.self", 0x7a1d621c, 2, 0x19054, 0x1D62);
            vgPatchAddOffset("PCSA00133", "rc2.self", 0x7a1d621c, 2, 0x19054, 0x1D62);
            vgPatchAddOffset("PCSF00486", SELF_EBOOT, NID_ANY, 2, 0x19F34, 0x2B98);
            vgPatchAddOffset("PCSF00482", "rc3.self", 0xcf835e57, 2, 0x19F34, 0x2B98);
            vgPatchAddOffset("PCSA00133", "rc3.self", 0xcf835e57, 2, 0x19F34, 0x2B98);

            if (g_main.config.fb.width > 720 || g_main.config.fb.height > 408) {
                uint8_t movs_lr_parambufsize[4];
                vgMakeThumb2_T2_MOV(REGISTER_LR, 1, (12 * 1024 * 1024), movs_lr_parambufsize); // 0xC00000

                vgInjectData(0, g_main.offset[0], &movs_lr_parambufsize, sizeof(movs_lr_parambufsize));
            }

            vgInjectData(0, g_main.offset[1], &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, g_main.offset[1] + 4, &movs_r1_height, sizeof(movs_r1_height));
        }
    }
    //
    // Ninja Gaiden Sigma 2 Plus
    //
    else if (vgPatchIsGame("PCSB00294", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE00233", SELF_EBOOT, NID_ANY) || // US
             vgPatchIsGame("PCSG00157", SELF_EBOOT, NID_ANY)) { // JP
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);
        vgConfigSetSupportedIbCount(2);

        if (vgConfigIsIbEnabled()) {
            uint8_t large_patch[30] = {
                0x00, 0x00, 0x00, 0x00, // MOVS.W  R0, #width1
                0x00, 0x00, 0x00, 0x00, // MOVS.W  R1, #height1
                0x0E, 0x90,             // STR     R0, [SP,#0x38]
                0x0F, 0x91,             // STR     R1, [SP,#0x3C]
                0x00, 0x00, 0x00, 0x00, // MOVS.W  R0, #width2
                0x00, 0x00, 0x00, 0x00, // MOVS.W  R1, #height2
                0x10, 0x90,             // STR     R0, [SP,#0x40]
                0x11, 0x91,             // STR     R1, [SP,#0x44]
                0x12, 0x90,             // STR     R0, [SP,#0x48]
                0x00, 0xBF,             // NOP
                0x00, 0xBF,             // NOP
                                        // STR     R1, [SP,#0x4C]
            };
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[0].width, &large_patch[0]);
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.ib[0].height, &large_patch[4]);
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[1].width, &large_patch[12]);
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.ib[1].height, &large_patch[16]);

            vgPatchAddOffset("PCSB00294", SELF_EBOOT, NID_ANY, 1, 0x54A8A);
            vgPatchAddOffset("PCSE00233", SELF_EBOOT, NID_ANY, 1, 0x54A8A);
            vgPatchAddOffset("PCSG00157", SELF_EBOOT, NID_ANY, 1, 0x5393E);

            vgInjectData(0, g_main.offset[0], &large_patch, sizeof(large_patch));
        }
    }
    //
    // Ratchet & Clank: QForce / Ratchet & Clank: Full Frontal Assault
    //
    else if (vgPatchIsGame("PCSF00191", SELF_EBOOT, NID_ANY) || // EU [1.01]
             vgPatchIsGame("PCSA00086", SELF_EBOOT, NID_ANY)) { // US [1.01]
        vgConfigSetSupported(FT_ENABLED, FT_UNSUPPORTED, FT_UNSUPPORTED);

        if (vgConfigIsFbEnabled()) {
            uint8_t movs_r3_width[4], movs_r4_height[4];
            vgMakeThumb2_T2_MOV(3, 1, g_main.config.fb.width, movs_r3_width);
            vgMakeThumb2_T2_MOV(4, 1, g_main.config.fb.height, movs_r4_height);

            vgInjectData(0, 0x5557C2, &movs_r3_width, sizeof(movs_r3_width));
            vgInjectData(0, 0x5557C2 + 8, &movs_r4_height, sizeof(movs_r4_height));
        }
    }
    //
    // Utawarerumono: Chiriyuku Mono he no Komoriuta
    //
    else if (vgPatchIsGame("PCSG01079", SELF_EBOOT, NID_ANY)) { // JP [1.02]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t mov_r0_width[4], mov_r1_height[4];
            vgMakeArm_A1_MOV(0, 0, g_main.config.ib[0].width, mov_r0_width);
            vgMakeArm_A1_MOV(1, 0, g_main.config.ib[0].height, mov_r1_height);

            vgInjectData(0, 0x137808, &mov_r0_width, sizeof(mov_r0_width));
            vgInjectData(0, 0x137808 + 8, &mov_r1_height, sizeof(mov_r1_height));
        }
    }
    //
    // Dragon Ball Z: Battle of Z
    //
    else if (vgPatchIsGame("PCSB00396", SELF_EBOOT, NID_ANY) || // EU [1.01]
             vgPatchIsGame("PCSE00305", SELF_EBOOT, NID_ANY) || // US [1.01]
             vgPatchIsGame("PCSG00213", SELF_EBOOT, NID_ANY)) { // JP [1.01]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t mov_r0_width[4], mov_r1_height[4];
            uint8_t mov_r2_width[4], mov_r3_height[4];
            uint8_t mov_r3_width[4], mov_r5_height[4], mov_r6_height[4];
            vgMakeArm_A1_MOV(0, 0, g_main.config.ib[0].width, mov_r0_width);
            vgMakeArm_A1_MOV(1, 0, g_main.config.ib[0].height, mov_r1_height);
            vgMakeArm_A1_MOV(2, 0, g_main.config.ib[0].width, mov_r2_width);
            vgMakeArm_A1_MOV(3, 0, g_main.config.ib[0].height, mov_r3_height);
            vgMakeArm_A1_MOV(3, 0, g_main.config.ib[0].width, mov_r3_width);
            vgMakeArm_A1_MOV(5, 0, g_main.config.ib[0].height, mov_r5_height);
            vgMakeArm_A1_MOV(6, 0, g_main.config.ib[0].height, mov_r6_height);

            vgPatchAddOffset("PCSB00396", SELF_EBOOT, NID_ANY, 5, 0x63E8D0, 0x63CE94, 0x63D200, 0x63DA1C, 0x63FF1C);
            vgPatchAddOffset("PCSE00305", SELF_EBOOT, NID_ANY, 5, 0x63E8A0, 0x63CE64, 0x63D1D0, 0x63D9EC, 0x63FEEC);
            vgPatchAddOffset("PCSG00213", SELF_EBOOT, NID_ANY, 5, 0x63DE3C, 0x63C400, 0x63C76C, 0x63CF88, 0x63F488);

            vgInjectData(0, g_main.offset[0], &mov_r0_width, sizeof(mov_r0_width));
            vgInjectData(0, g_main.offset[0] + 8, &mov_r1_height, sizeof(mov_r1_height));
            vgInjectData(0, g_main.offset[1], &mov_r0_width, sizeof(mov_r0_width));
            vgInjectData(0, g_main.offset[1] + 8, &mov_r1_height, sizeof(mov_r1_height));
            vgInjectData(0, g_main.offset[2], &mov_r2_width, sizeof(mov_r2_width));
            vgInjectData(0, g_main.offset[2] + 8, &mov_r3_height, sizeof(mov_r3_height));
            vgInjectData(0, g_main.offset[3], &mov_r3_width, sizeof(mov_r3_width));
            vgInjectData(0, g_main.offset[3] + 8, &mov_r5_height, sizeof(mov_r5_height));
            vgInjectData(0, g_main.offset[4], &mov_r3_width, sizeof(mov_r3_width));
            vgInjectData(0, g_main.offset[4] + 8, &mov_r6_height, sizeof(mov_r6_height));
        }
    }
    //
    // Wipeout 2048
    //
    else if (vgPatchIsGame("PCSF00007", SELF_EBOOT, NID_ANY) || // EU [1.04]
             vgPatchIsGame("PCSA00015", SELF_EBOOT, NID_ANY) || // US [1.04]
             vgPatchIsGame("PCSC00006", SELF_EBOOT, NID_ANY) || // JP [1.04]
             vgPatchIsGame("PCSD00005", SELF_EBOOT, NID_ANY)) { // ASIA [1.04]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_ENABLED);
        vgConfigSetSupportedIbCount(14);

        if (vgConfigIsIbEnabled()) {
            vgPatchAddOffset("PCSF00007", SELF_EBOOT, NID_ANY, 4, 0x2941D0, 0x294206, 0x2941F8, 0x424870);
            vgPatchAddOffset("PCSA00015", SELF_EBOOT, NID_ANY, 4, 0x2941D0, 0x294206, 0x2941F8, 0x424870);
            vgPatchAddOffset("PCSC00006", SELF_EBOOT, NID_ANY, 4, 0x2941D4, 0x29420A, 0x2941FC, 0x424870);
            vgPatchAddOffset("PCSD00005", SELF_EBOOT, NID_ANY, 4, 0x2941D4, 0x29420A, 0x2941FC, 0x424870);

            if (g_main.config.ib_count == 1) {
                uint8_t movs_r0_1[2], branch_0x36[2] = {0x19, 0xE0};
                vgMakeThumb_T1_MOV(0, 1, movs_r0_1);

                vgInjectData(0, g_main.offset[0], &branch_0x36, sizeof(branch_0x36));
                vgInjectData(0, g_main.offset[1], &movs_r0_1, sizeof(movs_r0_1));
            } else {
                uint8_t movs_r4_1[2], cmp_r4_1[2] = {0x01, 0x2C};
                vgMakeThumb_T1_MOV(4, 1, movs_r4_1);

                vgInjectData(0, g_main.offset[2], &cmp_r4_1, sizeof(cmp_r4_1));
                vgInjectData(0, g_main.offset[2] + 4, &movs_r4_1, sizeof(movs_r4_1));
            }

            uint32_t data32_w_h[14 * 2];
            for (uint8_t i = 0; i < 14; i++) {
                data32_w_h[i * 2] = g_main.config.ib[i].width;
                data32_w_h[i * 2 + 1] = g_main.config.ib[i].height;
            }
            vgInjectData(0, g_main.offset[3] + 8, &data32_w_h, sizeof(data32_w_h));
        }

        if (vgConfigIsFpsEnabled()) {
            vgPatchAddOffset("PCSF00007", SELF_EBOOT, NID_ANY, 1, 0x2F5D2E);
            vgPatchAddOffset("PCSA00015", SELF_EBOOT, NID_ANY, 1, 0x2F5D2E);
            vgPatchAddOffset("PCSC00006", SELF_EBOOT, NID_ANY, 1, 0x2F5D32);
            vgPatchAddOffset("PCSD00005", SELF_EBOOT, NID_ANY, 1, 0x2F5D32);

            if (g_main.config.fps == FPS_60) {
                uint8_t nop[2] = {0x00, 0xBF};
                vgInjectData(0, g_main.offset[0], &nop, sizeof(nop));
            }
        }
    }
    //
    // Fate/EXTELLA LINK / The Umbral Star
    //
    else if (vgPatchIsGame("PCSG01091", SELF_EBOOT, NID_ANY) || // JP [1.07]
             vgPatchIsGame("PCSH10121", SELF_EBOOT, NID_ANY)) { // ASIA [1.02]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t mov_r0_width[4], mov_r0_height[4];
            vgMakeThumb2_T2_MOV(0, 0, g_main.config.ib[0].width, mov_r0_width);
            vgMakeThumb2_T2_MOV(0, 0, g_main.config.ib[0].height, mov_r0_height);

            vgPatchAddOffset("PCSG01091", SELF_EBOOT, NID_ANY, 1, 0x6C919C);
            vgPatchAddOffset("PCSH10121", SELF_EBOOT, NID_ANY, 1, 0x6CEF98);

            vgInjectData(0, g_main.offset[0], &mov_r0_width, sizeof(mov_r0_width));
            vgInjectData(0, g_main.offset[0] + 6, &mov_r0_height, sizeof(mov_r0_height));
        }
    }
    //
    // The Legend of Heroes: Trails of Cold Steel / Eiyuu Densetsu: Sen no Kiseki
    //
    else if (vgPatchIsGame("PCSB00866", SELF_EBOOT, NID_ANY) || // EU [1.01]
             vgPatchIsGame("PCSE00786", SELF_EBOOT, NID_ANY) || // US [1.02]
             vgPatchIsGame("PCSG00195", SELF_EBOOT, NID_ANY) || // JP [1.03]
             vgPatchIsGame("PCSH00074", SELF_EBOOT, NID_ANY)) { // ASIA [1.03]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r1_w_r2_h[8], movs_r2_w_r3_h[8];
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.ib[0].width, movs_r1_w_r2_h);
            vgMakeThumb2_T2_MOV(2, 1, g_main.config.ib[0].height, &movs_r1_w_r2_h[4]);
            vgMakeThumb2_T2_MOV(2, 1, g_main.config.ib[0].width, movs_r2_w_r3_h);
            vgMakeThumb2_T2_MOV(3, 1, g_main.config.ib[0].height, &movs_r2_w_r3_h[4]);

            vgPatchAddOffset("PCSB00866", SELF_EBOOT, NID_ANY, 3, 0xF9AD6, 0xF9910, 0xF9946);
            vgPatchAddOffset("PCSE00786", SELF_EBOOT, NID_ANY, 3, 0xF99F2, 0xF982C, 0xF9862);
            vgPatchAddOffset("PCSG00195", SELF_EBOOT, NID_ANY, 3, 0xF971A, 0xF9554, 0xF958A);
            vgPatchAddOffset("PCSH00074", SELF_EBOOT, NID_ANY, 3, 0xF9B32, 0xF996C, 0xF99A2);

            vgInjectData(0, g_main.offset[0], &movs_r1_w_r2_h, sizeof(movs_r1_w_r2_h));
            vgInjectData(0, g_main.offset[1], &movs_r1_w_r2_h, sizeof(movs_r1_w_r2_h));
            vgInjectData(0, g_main.offset[2], &movs_r2_w_r3_h, sizeof(movs_r2_w_r3_h));
        }
    }
    //
    // The Legend of Heroes: Trails of Cold Steel II
    //
    else if (vgPatchIsGame("PCSB01016", SELF_EBOOT, NID_ANY) || // EU
             vgPatchIsGame("PCSE00896", SELF_EBOOT, NID_ANY) || // US [1.01]
             vgPatchIsGame("PCSG00354", SELF_EBOOT, NID_ANY) || // JP [1.03]
             vgPatchIsGame("PCSH00075", SELF_EBOOT, NID_ANY)) { // ASIA [1.03]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r1_w_r2_h[8];
            vgMakeThumb2_T2_MOV(1, 1, g_main.config.ib[0].width, movs_r1_w_r2_h);
            vgMakeThumb2_T2_MOV(2, 1, g_main.config.ib[0].height, &movs_r1_w_r2_h[4]);

            vgPatchAddOffset("PCSB01016", SELF_EBOOT, NID_ANY, 3, 0x139B8C, 0x1399EC, 0x1399AC);
            vgPatchAddOffset("PCSE00896", SELF_EBOOT, NID_ANY, 3, 0x139AA4, 0x139904, 0x1398C4);
            vgPatchAddOffset("PCSG00354", SELF_EBOOT, NID_ANY, 3, 0x1395C4, 0x139422, 0x1393DE);
            vgPatchAddOffset("PCSH00075", SELF_EBOOT, NID_ANY, 3, 0x139BE8, 0x139A48, 0x139A08);

            vgInjectData(0, g_main.offset[0], &movs_r1_w_r2_h, sizeof(movs_r1_w_r2_h));
            vgInjectData(0, g_main.offset[1], &movs_r1_w_r2_h, sizeof(movs_r1_w_r2_h));
            vgInjectData(0, g_main.offset[2], &movs_r1_w_r2_h, sizeof(movs_r1_w_r2_h));
        }
    }
    //
    // Assassin's Creed III: Liberation
    //
    else if (vgPatchIsGame("PCSB00074", SELF_EBOOT, NID_ANY) || // EU [1.02]
             vgPatchIsGame("PCSE00053", SELF_EBOOT, NID_ANY)) { // US [1.02]
        vgConfigSetSupported(FT_UNSUPPORTED, FT_ENABLED, FT_ENABLED);

        if (vgConfigIsIbEnabled()) {
            uint8_t movs_r0_width[4], movs_r0_height[4];
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[0].width, movs_r0_width);
            vgMakeThumb2_T2_MOV(0, 1, g_main.config.ib[0].height, movs_r0_height);

            if (g_main.config.ib[0].width > 720 || g_main.config.ib[0].height > 408) {
                uint8_t movs_r1_gxm_parambuf[4];
                uint32_t data32_mem_colsurf = 0x1100000 + (9 * 1024 * 1024);
                uint32_t data32_mem_dssurf  = 0x400000  + (1 * 1024 * 1024);
                uint32_t data32_mem_tex     = 0x3B00000 - (2 * 1024 * 1024);
                vgMakeThumb2_T2_MOV(1, 1, 0x1000000 - (8 * 1024 * 1024), movs_r1_gxm_parambuf);

                vgInjectData(0, 0xBB4C, &movs_r1_gxm_parambuf, sizeof(movs_r1_gxm_parambuf));
                vgInjectData(1, 0x32C, &data32_mem_colsurf, sizeof(data32_mem_colsurf));
                vgInjectData(1, 0x32C + 16, &data32_mem_dssurf, sizeof(data32_mem_dssurf));
                vgInjectData(1, 0x32C + 96, &data32_mem_tex, sizeof(data32_mem_tex));
            }

            vgInjectData(0, 0xE9E8, &movs_r0_width, sizeof(movs_r0_width));
            vgInjectData(0, 0xE9E8 + 6, &movs_r0_height, sizeof(movs_r0_height));
        }

        if (vgConfigIsFpsEnabled()) {
            uint8_t movs_r0_vblank[4];
            vgMakeThumb_T1_MOV(0, g_main.config.fps == FPS_60 ? 1 : 2, movs_r0_vblank);

            vgInjectData(0, 0xCBC2, &movs_r0_vblank, sizeof(movs_r0_vblank));
        }
    }
}
