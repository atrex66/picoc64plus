# C64+ Memory-Mapped Peripherals (Pico 2 / RP2350)

---
## Sprites - Terminal sprites

** 32 piece of terminal sprites (2pixel on 1 Glyph) max 256x256 px**


| Register             | Hex    | Dec   | Size  | Description                               |
|-------------------------------------------------------------------------------------------|
|Sprite 0 Width        | $A000  | 40960 | 1 B   | Width of the sprite                       |
|Sprite 0 Height       | $A001  | 40961 | 1 B   | Height of the sprite                      |
|Sprite 0 Enabled      | $A002  | 40962 | 1 B   | 0 = sprite disabled others enable         |
|Sprite 0 X Pos        | $A003  | 40963 | 2 B   | 16bit signed short X position             |
|Sprite 0 Y Pos        | $A005  | 40965 | 2 B   | 16bit signed short Y position             |
|Sprite 0 Transparency | $A007  | 40967 | 1 B   | Color code of the transparent pixels      |
|Sprite 0 Address      | $A009  | 40968 | 2 B   | Starting address of the pixel data        |
|......                | .....  | ..... | ..... | repeating to Sprite 31                    |
|-------------------------------------------------------------------------------------------|


All other functions removed from the memory mapping because i want to use other roms (not c64 ones)
also for play with, and the memory mapping is hard to maintain, there a new opcode i called SysCall()
troug the SysCall assembly macro you can call native arm fuctions from assembly or use the implemented
basic commandd, you found small programs for the functions in the examples directory.