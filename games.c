#include <psp2/types.h>
#include <libk/string.h>
#include <taihen.h>

#include "tools.h"
#include "config.h"
#include "main.h"
#include "games.h"

uint8_t patch_game(const char *titleid, tai_module_info_t *eboot_info, VG_Config *config) {
	if (!strncmp(titleid, "PCSF00243", 9) || // Killzone Mercenary [EUR] [1.12]
			!strncmp(titleid, "PCSF00403", 9) || // Killzone Mercenary [EUR] [1.12]
			!strncmp(titleid, "PCSA00107", 9) || // Killzone Mercenary [USA] [1.12]
			!strncmp(titleid, "PCSC00045", 9) || // Killzone Mercenary [JPN] [1.12]
			!strncmp(titleid, "PCSD00071", 9)) { // Killzone Mercenary [ASA] [1.12]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_ENABLED, config);
		config_set_default(FT_DISABLED, FT_DISABLED, FT_DISABLED, config);

		if (config_is_ib_enabled(config)) {
			uint32_t data32_w_h_w_h[4] = {
				config->ib_width, config->ib_height, config->ib_width, config->ib_height
			};
			uint8_t nop_nop_nop_nop[8] = {
				0x00, 0xBF, 0x00, 0xBF, 0x00, 0xBF, 0x00, 0xBF
			};

			// seg000:8115A5C8  BL  sub_8104DFC6
			// seg000:8115A5CC  BL  sub_8104F55E
			injectData(eboot_info->modid, 0, 0x15A5C8, &nop_nop_nop_nop, sizeof(nop_nop_nop_nop));

			// dword_81AA1728  DCD 0x3C0  ; DATA XREF: sub_8104F55E
			// dword_81AA172C  DCD 0x220  ; DATA XREF: sub_8104F55E
			// dword_81AA1730  DCD 0x3C0  ; DATA XREF: sub_8104DFC6
			// dword_81AA1734  DCD 0x220  ; DATA XREF: sub_8104DFC6
			injectData(eboot_info->modid, 1, 0xD728, &data32_w_h_w_h, sizeof(data32_w_h_w_h));
		}
		if (config_is_fps_enabled(config)) {
			uint32_t data32_vblank = config->fps == FPS_60 ? 0x1 : 0x2;

			// dword_819706A4  DCD 2  ; DATA XREF: seg000:8104F722
			injectData(eboot_info->modid, 0, 0x9706A4, &data32_vblank, sizeof(data32_vblank));
		}
		
		return 1;
	}
	else if (!strncmp(titleid, "PCSB00245", 9) || // Persona 4 Golden [EUR]
			!strncmp(titleid, "PCSE00120", 9) || // Persona 4 Golden [USA]
			!strncmp(titleid, "PCSG00004", 9) || // Persona 4 Golden [JPN] [1.01]
			!strncmp(titleid, "PCSG00563", 9) || // Persona 4 Golden [JPN]
			!strncmp(titleid, "PCSH00021", 9)) { // Persona 4 Golden [ASA]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED, config);
		config_set_default(FT_DISABLED, FT_ENABLED, FT_DISABLED, config);

		if (config_is_ib_enabled(config)) {
			float float_w_h[2] = {
				(float)config->ib_width, (float)config->ib_height
			};
			uint32_t offset_w_h = 0;

			if (!strncmp(titleid, "PCSB00245", 9)) {
				offset_w_h = 0xDBCFC;
			} else if (!strncmp(titleid, "PCSE00120", 9)) {
				offset_w_h = 0xDBCEC;
			} else if (!strncmp(titleid, "PCSG00004", 9) ||
					!strncmp(titleid, "PCSG00563", 9)) {
				offset_w_h = 0xDBD9C;
			} else if (!strncmp(titleid, "PCSH00021", 9)) {
				offset_w_h = 0xF1C50;
			}

			injectData(eboot_info->modid, 1, offset_w_h, &float_w_h, sizeof(float_w_h));
		}
		
		return 1;
	}
	else if (!strncmp(titleid, "PCSB00204", 9)) { // WRC 3: FIA World Rally Championship [EUR] [1.01]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED, config);
		config_set_default(FT_DISABLED, FT_ENABLED, FT_DISABLED, config);

		if (config_is_ib_enabled(config)) {
			uint8_t movs_r5_width[4], movs_r6_height[4];
			make_thumb2_t2_mov(5, 1, config->ib_width, movs_r5_width);
			make_thumb2_t2_mov(6, 1, config->ib_height, movs_r6_height);

			injectData(eboot_info->modid, 0, 0xAC430A, &movs_r5_width, sizeof(movs_r5_width));
			injectData(eboot_info->modid, 0, 0xAC4310, &movs_r6_height, sizeof(movs_r6_height));
		}
		
		return 1;
	}
	else if (!strncmp(titleid, "PCSB00345", 9) || // WRC 4: FIA World Rally Championship [EUR] [1.01]
			!strncmp(titleid, "PCSE00411", 9)) { // WRC 4: FIA World Rally Championship [USA]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED, config);
		config_set_default(FT_DISABLED, FT_ENABLED, FT_DISABLED, config);

		if (config_is_ib_enabled(config)) {
			uint8_t movs_r0_width[4], movs_r4_height[4];
			uint32_t offset_w = 0, offset_h = 0;
			make_thumb2_t2_mov(0, 1, config->ib_width, movs_r0_width);
			make_thumb2_t2_mov(4, 1, config->ib_height, movs_r4_height);

			if (!strncmp(titleid, "PCSB00345", 9)) {
				offset_w = 0xAC297C; offset_h = 0xAC2982;
			} else if (!strncmp(titleid, "PCSE00411", 9)) {
				offset_w = 0xAC46C4; offset_h = 0xAC46CA;
			}

			injectData(eboot_info->modid, 0, offset_w, &movs_r0_width, sizeof(movs_r0_width));
			injectData(eboot_info->modid, 0, offset_h, &movs_r4_height, sizeof(movs_r4_height));
		}
		
		return 1;
	}
	else if ((!strncmp(titleid, "PCSF00438", 9) && // God of War Collection [EUR]
			(eboot_info->module_nid == 0x8638ffed ||  // gow1.self
			eboot_info->module_nid == 0x6531f96a)) || // gow2.self
			(!strncmp(titleid, "PCSA00126", 9) && // God of War Collection [USA]
			(eboot_info->module_nid == 0x126f65c5 ||  // gow1.self
			eboot_info->module_nid == 0x64ec7e)) ||   // gow2.self
			(!strncmp(titleid, "PCSC00059", 9) && // God of War Collection [JPN]
			(eboot_info->module_nid == 0x990f8128 ||  // gow1.self
			eboot_info->module_nid == 0x395a00f6))) { // gow2.self
		config_set_unsupported(FT_ENABLED, FT_DISABLED, FT_ENABLED, config);
		config_set_default(FT_ENABLED, FT_DISABLED, FT_ENABLED, config);

		if (config_is_fb_enabled(config)) {
			uint8_t movs_r0_width[4], movs_r4_width[4], movs_r7_width[4];
			uint8_t movs_r1_height[4], movs_r2_height[4], movs_lr_height[4];
			uint32_t offset_w_h_1 = 0, offset_w_h_2 = 0;
			uint32_t offset_w_h_3 = 0, offset_w_h_4 = 0;
			uint32_t offset_h = 0;
			make_thumb2_t2_mov(0, 1, config->fb_width, movs_r0_width);
			make_thumb2_t2_mov(4, 1, config->fb_width, movs_r4_width);
			make_thumb2_t2_mov(7, 1, config->fb_width, movs_r7_width);
			make_thumb2_t2_mov(1, 1, config->fb_height, movs_r1_height);
			make_thumb2_t2_mov(2, 1, config->fb_height, movs_r2_height);
			make_thumb2_t2_mov(REGISTER_LR, 1, config->fb_height, movs_lr_height);

			if (!strncmp(titleid, "PCSF00438", 9)) {
				if (eboot_info->module_nid == 0x8638ffed) { // gow1.self
					offset_w_h_1 = 0x9E212; offset_w_h_2 = 0x9F0F0; offset_w_h_3 = 0xA31C6;
					offset_w_h_4 = 0xCEF06; offset_h = 0xA1098;
				} else if (eboot_info->module_nid == 0x6531f96a) { // gow2.self
					offset_w_h_1 = 0xCDAE6; offset_w_h_2 = 0xCE9C4; offset_w_h_3 = 0xD2DBA;
					offset_w_h_4 = 0xFF782; offset_h = 0xD0C8C;
				}
			} else if (!strncmp(titleid, "PCSA00126", 9)) {
				if (eboot_info->module_nid == 0x126f65c5) { // gow1.self
					offset_w_h_1 = 0x9E36E; offset_w_h_2 = 0x9F24C; offset_w_h_3 = 0xA3322;
					offset_w_h_4 = 0xCF062; offset_h = 0xA11F4;
				} else if (eboot_info->module_nid == 0x64ec7e) { // gow2.self
					offset_w_h_1 = 0xCD9AE; offset_w_h_2 = 0xCE88C; offset_w_h_3 = 0xD2C82;
					offset_w_h_4 = 0xFF64A; offset_h = 0xD0B54;
				}
			} else if (!strncmp(titleid, "PCSC00059", 9)) {
				if (eboot_info->module_nid == 0x990f8128) { // gow1.self
					offset_w_h_1 = 0x9E1E6; offset_w_h_2 = 0x9F0C4; offset_w_h_3 = 0xA319A;
					offset_w_h_4 = 0xCEEDA; offset_h = 0xA106C;
				} else if (eboot_info->module_nid == 0x395a00f6) { // gow2.self
					offset_w_h_1 = 0xCD7DA; offset_w_h_2 = 0xCE6B8; offset_w_h_3 = 0xD2AAE;
					offset_w_h_4 = 0xFF476; offset_h = 0xD0980;
				}
			}

			injectData(eboot_info->modid, 0, offset_w_h_1, &movs_r4_width, sizeof(movs_r4_width));
			injectData(eboot_info->modid, 0, offset_w_h_1 + 0x8, &movs_r2_height, sizeof(movs_r2_height));
			injectData(eboot_info->modid, 0, offset_w_h_2, &movs_r0_width, sizeof(movs_r0_width));
			injectData(eboot_info->modid, 0, offset_w_h_2 + 0x8, &movs_r1_height, sizeof(movs_r1_height));
			injectData(eboot_info->modid, 0, offset_w_h_3, &movs_r7_width, sizeof(movs_r7_width));
			injectData(eboot_info->modid, 0, offset_w_h_3 + 0x6, &movs_r1_height, sizeof(movs_r1_height));
			injectData(eboot_info->modid, 0, offset_w_h_4, &movs_r0_width, sizeof(movs_r0_width));
			injectData(eboot_info->modid, 0, offset_w_h_4 + 0x8, &movs_r2_height, sizeof(movs_r2_height));
			injectData(eboot_info->modid, 0, offset_h, &movs_lr_height, sizeof(movs_lr_height));
		}
		if (config_is_fps_enabled(config)) {
			uint8_t mov_r0_vblank[2];
			uint32_t offset_vblank = 0;
			make_thumb_t1_mov(0, config->fps == FPS_60 ? 0x1 : 0x2, mov_r0_vblank);

			if (!strncmp(titleid, "PCSF00438", 9)) {
				if (eboot_info->module_nid == 0x8638ffed) { // gow1.self
					offset_vblank = 0x9E228;
				} else if (eboot_info->module_nid == 0x6531f96a) { // gow2.self
					offset_vblank = 0xCDAFC;
				}
			} else if (!strncmp(titleid, "PCSA00126", 9)) {
				if (eboot_info->module_nid == 0x126f65c5) { // gow1.self
					offset_vblank = 0x9E384;
				} else if (eboot_info->module_nid == 0x64ec7e) { // gow2.self
					offset_vblank = 0xCD9C4;
				}
			} else if (!strncmp(titleid, "PCSC00059", 9)) {
				if (eboot_info->module_nid == 0x990f8128) { // gow1.self
					offset_vblank = 0x9E1FC;
				} else if (eboot_info->module_nid == 0x395a00f6) { // gow2.self
					offset_vblank = 0xCD7F0;
				}
			}

			// sceDisplayWaitVblankStartMulti
			injectData(eboot_info->modid, 0, offset_vblank, &mov_r0_vblank, sizeof(mov_r0_vblank));
		}

		return 1;
	}
	else if (!strncmp(titleid, "PCSB00182", 9)) { // MUD - FIM Motocross World Championship [EUR]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED, config);
		config_set_default(FT_DISABLED, FT_ENABLED, FT_DISABLED, config);

		if (config_is_ib_enabled(config)) {
			uint8_t movs_r5_width[4], movs_r6_height[4];
			make_thumb2_t2_mov(5, 1, config->ib_width, movs_r5_width);
			make_thumb2_t2_mov(6, 1, config->ib_height, movs_r6_height);

			injectData(eboot_info->modid, 0, 0x9B8B52, &movs_r5_width, sizeof(movs_r5_width));
			injectData(eboot_info->modid, 0, 0x9B8B58, &movs_r6_height, sizeof(movs_r6_height));
		}

		return 1;
	}
	else if (!strncmp(titleid, "PCSB00470", 9) || // MXGP: The Official Motocross Videogame [EUR]
			!strncmp(titleid, "PCSE00530", 9)) {  // MXGP: The Official Motocross Videogame [USA]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED, config);
		config_set_default(FT_DISABLED, FT_ENABLED, FT_DISABLED, config);

		if (config_is_ib_enabled(config)) {
			uint8_t movs_r0_width[4], movs_r5_height[4];
			uint32_t offset_w_h = 0;
			make_thumb2_t2_mov(0, 1, config->ib_width, movs_r0_width);
			make_thumb2_t2_mov(5, 1, config->ib_height, movs_r5_height);

			if (!strncmp(titleid, "PCSB00470", 9)) {
				offset_w_h = 0xB1D47A;
			} else if (!strncmp(titleid, "PCSE00530", 9)) {
				offset_w_h = 0xB1D36A;
			}

			injectData(eboot_info->modid, 0, offset_w_h, &movs_r0_width, sizeof(movs_r0_width));
			injectData(eboot_info->modid, 0, offset_w_h + 0x6, &movs_r5_height, sizeof(movs_r5_height));
		}

		return 1;
	}
	else if (!strncmp(titleid, "PCSB00027", 9)) { // F1 2011 [EUR]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED, config);
		config_set_default(FT_DISABLED, FT_ENABLED, FT_DISABLED, config);

		if (config_is_ib_enabled(config)) {
			uint8_t movs_r1_width_r2_height[8];
			make_thumb2_t2_mov(1, 1, config->ib_width, movs_r1_width_r2_height);
			make_thumb2_t2_mov(2, 1, config->ib_height, &movs_r1_width_r2_height[4]);

			injectData(eboot_info->modid, 0, 0x10F0AA, &movs_r1_width_r2_height, sizeof(movs_r1_width_r2_height));
		}

		return 1;
	}
	else if (!strncmp(titleid, "PCSF00021", 9) || // LittleBigPlanet [EUR] [1.22]
			!strncmp(titleid, "PCSA00017", 9) || // LittleBigPlanet [USA] [1.22]
			!strncmp(titleid, "PCSC00013", 9) || // LittleBigPlanet [JPN] [1.22]
			!strncmp(titleid, "PCSD00006", 9)) { // LittleBigPlanet [ASA] [1.22]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED, config);
		config_set_default(FT_DISABLED, FT_ENABLED, FT_DISABLED, config);

		if (config_is_ib_enabled(config)) {
			uint8_t movs_r1_width[4], movs_r2_height[4];
			make_thumb2_t2_mov(1, 1, config->ib_width, movs_r1_width);
			make_thumb2_t2_mov(2, 1, config->ib_height, movs_r2_height);

			injectData(eboot_info->modid, 0, 0x168546, &movs_r1_width, sizeof(movs_r1_width));
			injectData(eboot_info->modid, 0, 0x168546 + 0x4, &movs_r2_height, sizeof(movs_r2_height));
			injectData(eboot_info->modid, 0, 0x16856A, &movs_r1_width, sizeof(movs_r1_width));
			injectData(eboot_info->modid, 0, 0x16856A + 0x4, &movs_r2_height, sizeof(movs_r2_height));
			injectData(eboot_info->modid, 0, 0x168582, &movs_r1_width, sizeof(movs_r1_width));
			injectData(eboot_info->modid, 0, 0x168582 + 0x8, &movs_r2_height, sizeof(movs_r2_height));
			injectData(eboot_info->modid, 0, 0x1685B0, &movs_r1_width, sizeof(movs_r1_width));
			injectData(eboot_info->modid, 0, 0x1685B0 + 0x4, &movs_r2_height, sizeof(movs_r2_height));
		}

		return 1;
	}
	else if (!strncmp(titleid, "PCSF00570", 9) || // Borderlands 2 [EUR] [1.07]
			!strncmp(titleid, "PCSE00383", 9)) { // Borderlands 2 [USA] [1.09]
		config_set_unsupported(FT_ENABLED, FT_UNSUPPORTED, FT_UNSUPPORTED, config);
		config_set_default(FT_DISABLED, FT_DISABLED, FT_DISABLED, config);

		if (config_is_fb_enabled(config)) {
			uint32_t data32_w_h[2] = {config->fb_width, config->fb_height};
			injectData(eboot_info->modid, 1, 0x24A94, &data32_w_h, sizeof(data32_w_h));
		}

		return 1;
	}
	else if (!strncmp(titleid, "PCSB00040", 9) || // Asphalt: Injection [EUR]
			!strncmp(titleid, "PCSE00007", 9)) { // Asphalt: Injection [USA]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED, config);
		config_set_default(FT_DISABLED, FT_ENABLED, FT_DISABLED, config);

		if (config_is_ib_enabled(config)) {
			uint32_t data32_w_h[2] = {config->ib_width, config->ib_height};
			injectData(eboot_info->modid, 1, 0x5A2C, &data32_w_h, sizeof(data32_w_h));
		}

		return 1;
	}
	else if (!strncmp(titleid, "PCSB00877", 9) || // LEGO Star Wars: The Force Awakens [EUR]
			!strncmp(titleid, "PCSE00791", 9)) { // LEGO Star Wars: The Force Awakens [USA]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED, config);
		config_set_default(FT_DISABLED, FT_ENABLED, FT_DISABLED, config);

		if (config_is_ib_enabled(config)) {
			uint8_t movs_r4_width[4], movs_r5_height[4];
			uint32_t data32_w_h[2] = {config->ib_width, config->ib_height};
			uint32_t offset_w_h = 0, offset_data_w_h = 0;
			make_thumb2_t2_mov(4, 1, config->ib_width, movs_r4_width);
			make_thumb2_t2_mov(5, 1, config->ib_height, movs_r5_height);

			if (!strncmp(titleid, "PCSB00877", 9)) {
				offset_w_h = 0x1F313E; offset_data_w_h = 0x4650;
			} else if (!strncmp(titleid, "PCSE00791", 9)) {
				offset_w_h = 0x1F315E; offset_data_w_h = 0x4508;
			}

			injectData(eboot_info->modid, 0, offset_w_h, &movs_r4_width, sizeof(movs_r4_width));
			injectData(eboot_info->modid, 0, offset_w_h + 0x6, &movs_r5_height, sizeof(movs_r5_height));
			injectData(eboot_info->modid, 1, offset_data_w_h, &data32_w_h, sizeof(data32_w_h));
		}

		return 1;
	}
	else if (!strncmp(titleid, "PCSB00951", 9) || // World of Final Fantasy [EUR] [1.03]
			!strncmp(titleid, "PCSE00880", 9) || // World of Final Fantasy [USA] [1.03]
			!strncmp(titleid, "PCSH00223", 9)) { // World of Final Fantasy [ASA] [1.03]
		config_set_unsupported(FT_UNSUPPORTED, FT_ENABLED, FT_UNSUPPORTED, config);
		config_set_default_params(FT_DISABLED, 960, 544, FT_ENABLED, 800, 480, FT_DISABLED, FPS_30, config);

		if (config_is_ib_enabled(config)) {
			uint8_t movs_r5_width[4], movs_r0_height[4];
			uint32_t offset_w_h = 0;
			make_thumb2_t2_mov(5, 1, config->ib_width, movs_r5_width);
			make_thumb2_t2_mov(0, 1, config->ib_height, movs_r0_height);

			if (!strncmp(titleid, "PCSB00951", 9)) {
				offset_w_h = 0x22C9E6;
			} else if (!strncmp(titleid, "PCSE00880", 9)) {
				offset_w_h = 0x22C9FE;
			} else if (!strncmp(titleid, "PCSH00223", 9)) {
				offset_w_h = 0x22CA16;
			}

			injectData(eboot_info->modid, 0, offset_w_h, &movs_r5_width, sizeof(movs_r5_width));
			injectData(eboot_info->modid, 0, offset_w_h + 0x6, &movs_r0_height, sizeof(movs_r0_height));
		}

		return 1;
	}

	return 0;
}
