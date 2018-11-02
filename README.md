# VitaGrafix
VitaGrafix is a taiHEN plugin that allows you to change resolution and FPS cap of PS Vita games (to get better visuals, higher FPS or longer battery life).

## Credits
- Rinnegatamante - for [vitaRescale](https://github.com/Rinnegatamante/vitaRescale)
- dots_tb - for [ioPlus](https://github.com/CelesteBlue-dev/PSVita-RE-tools/tree/master/ioPlus/ioPlus-0.1)
- InquisitionImplied - for finding patches of some of the games included in here
- "the scene" - for giving us a chance to do these things in the first place

## Installation
1. Download latest *VitaGrafix.suprx* and *ioPlus.skprx* from the [Releases](https://github.com/Electry/VitaGrafix/releases) section
2. If *ux0:tai/config.txt* file does exist
    1. Copy *VitaGrafix.suprx* and *ioPlus.skprx* to *ux0:tai/* directory
    2. Open *ux0:tai/config.txt* in a text editor
    3. Add following lines to the bottom
```
*KERNEL
ux0:tai/ioPlus.skprx
*ALL
ux0:tai/VitaGrafix.suprx
```
3. Otherwise
    1. Copy *VitaGrafix.suprx* and *ioPlus.skprx* to *ur0:tai/* directory
    2. Open *ur0:tai/config.txt* in a text editor
    3. Add following lines to the bottom
```
*KERNEL
ur0:tai/ioPlus.skprx
*ALL
ur0:tai/VitaGrafix.suprx
```
4. Create *ux0:data/VitaGrafix* folder or start any game (the folder will be created automatically)
5. Create and open *ux0:data/VitaGrafix/config.txt* file
6. Add games you wish to apply patches for (refer to the configuration section and compatibility list below)

## Configuration
You can configure every game separately using unified configuration file.

### [MAIN] section
- This section applies to all games and overrides their individual options.
```
[MAIN]
ENABLED=1      <- Setting this to 0 disables all game modifications.
                  (default = 1)
OSD=1          <- Setting this to 0 disables in game OSD (during few seconds at the beginning).
                  (default = 1)
```

### GAME section
- This section applies to a single game. Each game supports different options! Refer to the compatibility table below.
```
[PCSB00245]    <- TITLE ID of your game
ENABLED=1      <- Setting this to 0 disables all game modifications.
                  (default = 1)
OSD=1          <- Setting this to 0 disables in game OSD (during few seconds at the beginning).
                  (default = 1)
FB=960x544     <- Framebuffer resolution. Setting this to OFF disables this feature.
                  (default = OFF)
                  Valid options:
                    960x544
                    720x408
                    640x368
                    OFF
IB=960x544     <- Internal buffer resolution. Setting this to OFF disables this feature.
                  Changing this generally does not impact resolution of UI elements.
                  (default = OFF)
                  Valid options:
                    WxH (where 0 < W <= 960 and 0 < H <= 544)
                    WxH,WxH,... (ONLY if the game supports multiple IB res. options)
                    OFF
FPS=60         <- FPS cap. Setting this to OFF disables this feature.
                  (default = OFF)
                  Valid options:
                    60
                    30
                    OFF
```

### Example config.txt
```
[MAIN]
ENABLED=1

# This is a comment, comments have to be on a separate line and start with # char
[PCSB00245]
OSD=0
IB=960x544

# Ninja Gaiden Sigma 2 Plus uses dynamic resolution scaling,
# and switches between two specified IB resolutions when patched
# (based on framerate)
[PCSB00294]
IB=960x544,720x408

[PCSE00411]
IB=864x492

[PCSB00204]
IB=OFF

[PCSF00438]
FB=720x408
FPS=30

[PCSB00204]
ENABLED=0
```
NOTE: If some options are left out, the plugin will use their default values.

## Supported games
|   | Game          | Title ID(s)   | Supported features | Game defaults | Notes.............. |
| - | ------------- | ------------- | ------------------ | ------------- | ------------------- |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Assassin's Creed III: Liberation | PCSB00074 <br/> PCSE00053 | Internal res. <br/> FPS cap. | 720x408 <br/> 30 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Asphalt: Injection | PCSB00040 <br/> PCSE00007 | Internal res. | 720x408 | |
| ![](https://placehold.it/20/eeee22/000000?text=+) | Borderlands 2 | PCSF00570 <br/> PCSF00576 <br/> PCSE00383 | Framebuffer | 960x544 | [#119](https://github.com/Electry/VitaGrafix/issues/119) |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Dead or Alive 5 Plus | PCSB00296 <br/> PCSE00235 <br/> PCSG00167 | Internal res. | 720x408 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Dragon Ball Z: Battle of Z | PCSB00396 <br/> PCSE00305 <br/> PCSG00213 | Internal res. | 704x448 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Dragon Quest Builders | PCSB00981 <br/> PCSE00912 <br/> PCSG00697 <br/> PCSH00221 | Internal res. <br/> FPS cap | 720x408 <br/> 60 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Fate/EXTELLA LINK (The Umbral Star) | PCSG01091 <br/> PCSH10121 | Internal res. | 720x408 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | F1 2011 | PCSB00027 | Internal res. | 640x384 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | God of War Collection | PCSF00438 <br/> PCSA00126 <br/> PCSC00059 | Framebuffer <br/> FPS cap | 720x408 <br/> 30 | |
| ![](https://placehold.it/20/eeee22/000000?text=+) | Hatsune Miku: Project Diva f | PCSB00419 <br/> PCSE00326 <br/> PCSG00074 | Internal res. <br/> FPS cap | 640x352 <br/> 30 | [#120](https://github.com/Electry/VitaGrafix/issues/120), [#81](https://github.com/Electry/VitaGrafix/issues/81) |
| ![](https://placehold.it/20/eeee22/000000?text=+) | Hatsune Miku: Project Diva f 2nd | PCSB00554 <br/> PCSE00434 <br/> PCSG00205 <br/> PCSH00088 | Internal res. <br/> FPS cap | 720x408 <br/> 30 | [#120](https://github.com/Electry/VitaGrafix/issues/120), [#80](https://github.com/Electry/VitaGrafix/issues/80) |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Hatsune Miku: Project Diva X | PCSB01007 <br/> PCSE00867 <br/> PCSH00176 | Internal res. | 720x408 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Jak and Daxter Collection | PCSF00247 <br/> PCSF00248 <br/> PCSF00249 <br/> PCSF00250 <br/> PCSA00080 | Framebuffer | 720x408 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Killzone: Mercenary | PCSF00243 <br/> PCSF00403 <br/> PCSA00107 <br/> PCSC00045 <br/> PCSD00071 | Internal res. <br/> FPS cap | Dynamic <br/> 30 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | LEGO Star Wars: The Force Awakens | PCSB00877 <br/> PCSE00791 | Internal res. | 640x368 | |
| ![](https://placehold.it/20/eeee22/000000?text=+) | LittleBigPlanet | PCSF00021 <br/> PCSA00017 <br/> PCSC00013 <br/> PCSD00006 | Internal res. | 720x408 | [#24](https://github.com/Electry/VitaGrafix/issues/24) |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Miracle Girls Festival | PCSG00610 | Internal res. | 720x408 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | MotoGP 13 | PCSB00316 <br/> PCSE00409 | Internal res. | 704x448 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | MotoGP 14 | PCSE00529 | Internal res. | 704x448 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | MUD - FIM Motocross World Championship | PCSB00182 | Internal res. | 704x448 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | MXGP: The Official Motocross Videogame | PCSB00470 <br/> PCSE00530 | Internal res. | 704x448 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Ninja Gaiden Sigma 2 Plus | PCSB00294 <br/> PCSE00233 <br/> PCSG00157 | 2x Internal res. | 864x480, 640x416 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Persona 4 Golden | PCSB00245 <br/> PCSE00120 <br/> PCSG00004 <br/> PCSG00563 <br/> PCSH00021 | Internal res. | 840x476 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Ratchet & Clank | PCSF00484 <br/> PCSF00482 <br/> PCSA00133 | Framebuffer | 720x408 | |
| ![](https://placehold.it/20/eeee22/000000?text=+) | Ratchet & Clank 2: Locked and Loaded / Going Commando | PCSF00485 <br/> PCSF00482 <br/> PCSA00133 | Framebuffer | 720x408 | [#121](https://github.com/Electry/VitaGrafix/issues/121) |
| ![](https://placehold.it/20/eeee22/000000?text=+) | Ratchet & Clank 3 / Up Your Arsenal | PCSF00486 <br/> PCSF00482 <br/> PCSA00133 | Framebuffer | 720x408 | [#121](https://github.com/Electry/VitaGrafix/issues/121) |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Ratchet & Clank: QForce / Full Frontal Assault | PCSF00191 <br/> PCSA00086 | Framebuffer | 720x408 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Ridge Racer | PCSB00048 <br/> PCSE00001 <br/> PCSG00001 | Internal res. | 720x408 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Sly Cooper and the Thievius Raccoonus | PCSF00269 <br/> PCSF00338 <br/> PCSA00095 <br/> PCSA00096 <br/> | Framebuffer <br/> FPS cap | 720x408 <br/> 30 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Sly Cooper 2: Band of Thieves | PCSF00270 <br/> PCSF00338 <br/> PCSA00095 <br/> PCSA00097 <br/> | Framebuffer <br/> FPS cap | 960x544 <br/> 30 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Sly Cooper 3: Honor Among Thieves | PCSF00271 <br/> PCSF00338 <br/> PCSA00095 <br/> PCSA00098 <br/> | Framebuffer <br/> FPS cap | 960x544 <br/> 30 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Sly Cooper: Thieves in Time | PCSF00156 <br/> PCSF00206 <br/> PCSF00207 <br/> PCSF00208 <br/> PCSF00209 <br/> PCSA00068 | FPS cap | 30 | |
| ![](https://placehold.it/20/eeee22/000000?text=+) | The Amazing Spider-Man | PCSB00428 | Internal res. <br/> FPS cap | 704x400 <br/> 60 | [#122](https://github.com/Electry/VitaGrafix/issues/122) |
| ![](https://placehold.it/20/22bb22/000000?text=+) | The Legend of Heroes: Trails of Cold Steel <br/> Eiyuu Densetsu: Sen no Kiseki | PCSB00866 <br/> PCSE00786 <br/> PCSG00195 <br/> PCSH00074 | Internal res. | 720x408 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | The Legend of Heroes: Trails of Cold Steel II <br/> Eiyuu Densetsu: Sen no Kiseki II | PCSB01016 <br/> PCSE00896 <br/> PCSG00354 <br/> PCSH00075 | Internal res. | 720x408 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Utawarerumono: Mask of Deception / Itsuwari no Kamen | PCSB01093 <br/> PCSE00959 <br/> PCSG00617 | Internal res. | 672x384 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Utawarerumono: Mask of Truth / Futari no Hakuoro | PCSB01145 <br/> PCSE01102 <br/> PCSG00838 | Internal res. | 672x384 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Utawarerumono: Chiriyuku Mono he no Komoriuta | PCSG01079 | Internal res. | 672x384 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | Wipeout 2048 | PCSF00007 <br/> PCSA00015 <br/> PCSC00006 <br/> PCSD00005 | 14x Internal res. <br/> FPS cap | up to 960x544 <br/> 30 | |
| ![](https://placehold.it/20/eeee22/000000?text=+) | World of Final Fantasy | PCSB00951 <br/> PCSE00880 <br/> PCSH00223 <br/> PCSG00709 | Internal res. | 640x384 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | WRC 3: FIA World Rally Championship | PCSB00204 | Internal res. | 704x448 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | WRC 4: FIA World Rally Championship | PCSB00345 <br/> PCSE00411 | Internal res. | 704x448 | |
| ![](https://placehold.it/20/22bb22/000000?text=+) | WRC 5: FIA World Rally Championship | PCSB00762 | Framebuffer | 960x544 | |

* ![](https://placehold.it/10x10/22bb22/000000?text=+) no known issues
* ![](https://placehold.it/10x10/eeee22/000000?text=+) playable, but with some minor issues (check *[Issues](https://github.com/Electry/VitaGrafix/issues)* section)

Adding support for each and every game requires manual disassembly of game's binary to find addresses in the game code where the resolution is set. Some are easy to patch, others plainly impossible.
