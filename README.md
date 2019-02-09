# VitaGrafix
VitaGrafix is a taiHEN plugin that allows you to change resolution and FPS cap of PS Vita games (to get better visuals, higher FPS or longer battery life).

**Official patchlist:** [HERE](https://github.com/Electry/VitaGrafixPatchlist)

## Installation
1. Download latest *VitaGrafix.suprx* from the [Releases](https://github.com/Electry/VitaGrafix/releases) section and *ioPlus.skprx*  **(v0.1)** from [PSVita-RE-tools repo](https://github.com/CelesteBlue-dev/PSVita-RE-tools/tree/master/ioPlus/ioPlus-0.1/release)
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
3. Otherwise (or if you are using SD2VITA mounted as ux0)
    1. Copy *VitaGrafix.suprx* and *ioPlus.skprx* to *ur0:tai/* directory
    2. Open *ur0:tai/config.txt* in a text editor
    3. Add following lines to the bottom
```
*KERNEL
ur0:tai/ioPlus.skprx
*ALL
ur0:tai/VitaGrafix.suprx
```
4. Create *ux0:data/VitaGrafix/* folder or start any game (the folder will be created automatically)
5. Download patchlist.txt from [HERE](https://github.com/Electry/VitaGrafixPatchlist)
6. Create and open *ux0:data/VitaGrafix/config.txt* file
7. Add games to the config you wish to apply patches for (refer to your patchlist.txt)

## Configuration
You can configure every game separately using unified configuration file.

### [MAIN] section
- This section applies to all games, unless specific entry for a game exists. Available options are listed below.
```
[MAIN]
# Same options as in GAME section are permitted here. To save space, they are listed only once, below.
```

### GAME section
- This section applies to a single game. These options override the [MAIN] section (for that game). Each game supports different options! Refer to the compatibility table of your patchlist.txt
```
[PCSB00245]    <- TITLE ID of your game (if you're not sure, run your game and check log.txt using VitaShell)
ENABLED=1      <- Setting this to 0 disables all game modifications.
                  (default = 1)
                  Valid options:
                    1
                    0
OSD=1          <- Setting this to 0 disables in game on-screen display
                  (shown during few seconds at the beginning).
                  (default = 1)
                  Valid options:
                    1
                    0
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
MSAA=4x        <- Multisample anti-aliasing. Setting this to OFF disables this feature.
                  (default = OFF)
                  Valid options:
                    4x
                    2x
                    1x
                    OFF
```

### Example config.txt
```
# This is a comment, comments have to be on a separate line and start with # char

[MAIN]
# Set default options for all games
FB=960x544
IB=960x544
FPS=60
MSAA=4x

# Persona 4 Golden [EU], disable osd
# Notice that IB=960x544 is redundant here as it is
#  the same resolution as the default one in [MAIN] section above.
#  This of course doesn't cause any harm.
[PCSB00245]
OSD=0
IB=960x544

# Ninja Gaiden Sigma 2 Plus, uses dynamic resolution scaling
#  and switches between two specified IB resolutions when patched
#  (based on framerate)
# Override the [MAIN] IB resolution(s) for this game.
[PCSB00294]
IB=960x544,720x408

# LittleBigPlanet res. patch introduces some glitches,
#  let's disable it here.
[PCSA00017]
ENABLED=0

# GoW won't reach 60 FPS @ 960x544 very often, so let's
#  keep it locked to 30 (default FPS cap).
[PCSF00438]
FPS=OFF

# I think you get what this does by now.
[PCSB00204]
IB=OFF
```
NOTE: If some options are left out, the plugin will use their default values.


### Syntax for patchlist.txt
```
#
# Syntax:
#
#
# Game section:
#
#  [titleid,self_path,self_nid]
#    titleid    = Game's TITLEID, e.g. "PCSF00001"
#    self_path  = Part of SELF path, e.g. "eboot.bin" or "GOW1.self" (optional, but recommended)
#    self_nid   = NID of SELF written as hex integer, e.g. 0x12345678 (optional, but recommended)
#
#
# Patch type section:
#
#  @FB    = only patched if FB resolution is set in config.txt
#  @IB    = only patched if IB resolution is set in config.txt
#  @FPS   = only patched if FPS cap is set in config.txt
#  @MSAA  = only patched if MSAA mode is set in config.txt
#
#
# Patch:
#
#  segment : offset  generator( arguments )  repeat[optional]
#        1 : 0x123      uint32( <ib_w> )     *2
#
#
# NOTE: No extra spaces are allowed, spaces in the example
#        above are only for readability. Check valid examples below.
#
#
# Plain value generators:
#
#  uint32(x)    x = macro or decimal/hex integer (0 <= x < 2^32)
#  uint16(x)    x = macro or decimal/hex integer (0 <= x < 2^16)
#  fl32(x)      x = macro or decimal/hex integer -> will be converted to IEEE-754 32-bit float
#  bytes(x)     x = 1 or more 8-bit hex values without the 0x prefix (e.g. "01 02" or "DEADBEEF")
#
#
# Instruction encoders:
#
#  t1_mov(register,value)            - MOV{S} <Rd>,#<imm8>     (Thumb instr. set)
#    register   = destination, decimal integer (0 <= r <= 14), 0 denotes R0 or A1
#    value      = macro or decimal/hex integer (0 <= v < 256)
#
#  t2_mov(setflags,register,value)   - MOV{S}.W <Rd>,#<const>  (Thumb2 instr. set)
#    setflags   = 1 for MOVS.W, 0 for MOV.W
#    register   = destination, decimal integer (0 <= r <= 14), 0 denotes R0 or A1
#    value      = macro or decimal/hex integer (0 <= v < 2^32)
#
#  t3_mov(register,value)            - MOVW <Rd>,#<imm16>      (Thumb2 instr. set)
#    register   = destination, decimal integer (0 <= r <= 14), 0 denotes R0 or A1
#    value      = macro or decimal/hex integer (0 <= v < 2^16)
#
#  t1_movt(register,value)           - MOVT <Rd>,#<imm16>      (Thumb2 instr. set)
#    register   = destination, decimal integer (0 <= r <= 14), 0 denotes R0 or A1
#    value      = macro or decimal/hex integer (0 <= v < 2^16)
#
#  a1_mov(setflags,register,value)   - MOV{S} <Rd>,#<const>    (Arm instr. set)
#    setflags   = 1 for MOVS, 0 for MOV
#    register   = destination, decimal integer (0 <= r <= 14), 0 denotes R0 or A1
#    value      = macro or decimal/hex integer (0 <= v < 2^32)
#
#  a2_mov(register,value)            - MOVW <Rd>,#<imm16>      (Arm instr. set)
#    register   = destination, decimal integer (0 <= r <= 14), 0 denotes R0 or A1
#    value      = macro or decimal/hex integer (0 <= v < 2^16)
#
#  nop()                             - NOP     (00 BF)
#  bkpt()                            - BKPT    (00 BE)
#
#
# General macros:   - a, b, c and d = another macro or decimal/hex integer
#
#  <+,a,b>          = a + b
#  <-,a,b>          = a - b
#  <*,a,b>          = a * b
#  </,a,b>          = a / b
#  <&,a,b>          = a & b (bitwise)
#  <|,a,b>          = a | b (bitwise)
#  <l,a,b>          = a << b (bitwise)
#  <r,a,b>          = a >> b (bitwise)
#  <min,a,b>        = if a < b then a else b
#  <max,a,b>        = if a > b then a else b
#  <if_eq,a,b,c,d>  = if a == b then c else d
#  <if_gt,a,b,c,d>  = if a > b then c else d
#  <if_lt,a,b,c,d>  = if a < b then c else d
#  <if_ge,a,b,c,d>  = if a >= b then c else d
#  <if_le,a,b,c,d>  = if a <= b then c else d
#  <to_fl,a>        = a converted to IEEE-754 32-bit float value
#
#
# Config-based macros:  - these return values based on config.txt entry for that particular titleid
#
#  <fb_w>           = framebuffer width
#  <fb_h>           = framebuffer height
#  <ib_w>           = backbuffer width (1st) i=0
#  <ib_h>           = backbuffer height (1st) i=0
#  <ib_w,n>         = backbuffer width (n-th) 0 <= n <= MAX_IB_RES_COUNT
#  <ib_h,n>         = backbuffer height, same as above
#  <vblank>         = FPS as vblank wait period (60FPS = 1, 30FPS = 2, 20FPS = 3, etc...)
#  <msaa>           = MSAA as multisample mode (1x = 0, 2x = 1, 4x = 2)
#  <msaa_enabled>   = 1 if MSAA != OFF, 0 if MSAA=OFF or not specified
#
#
#
#
#
# Valid examples (syntactically, in reality they don't make any logical sense):
#
[PCSF00001,eboot.bin,0x12345678]
@FB
0:0x1 uint16(960)
0:0x1 uint16(0x3C0)
0:0x2 uint32(<fb_h>)
0:0x3 bytes(C0 03 00 00)
0:0x4 bytes(01020304)
0:0x5 fl32(960)
0:0x6 a1_mov(1,4,</,<fb_w>,2>)
1:0x7 a1_mov(0,5,840)
1:0x8 t3_mov(2,<if_eq,<msaa_mode>,1,<msaa>,0>)
1:0x9 t2_mov(1,2,960)
1:0xA t2_mov(0,3,840)
1:0xB nop()
1:0xC bkpt()
@FPS
0:0x1234 t1_mov(0,<vblank>)
@MSAA
0:0x3123 t1_mov(1,<msaa>)
```

## Credits
- Rinnegatamante - for [vitaRescale](https://github.com/Rinnegatamante/vitaRescale)
- dots_tb - for [ioPlus](https://github.com/CelesteBlue-dev/PSVita-RE-tools/tree/master/ioPlus/ioPlus-0.1)
- "the scene" - for giving us a chance to do these things in the first place
