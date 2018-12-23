# VitaGrafix
VitaGrafix is a taiHEN plugin that allows you to change resolution and FPS cap of PS Vita games (to get better visuals, higher FPS or longer battery life).

**Official patchlist:** [HERE](https://github.com/Electry/VitaGrafixPatchlist)

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
5. Download patchlist.txt from [HERE](https://github.com/Electry/VitaGrafixPatchlist)
6. Create and open *ux0:data/VitaGrafix/config.txt* file
7. Add games to the config you wish to apply patches for (refer to your patchlist.txt)

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
