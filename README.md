# Acolyte Handheld

The Acolyte Handheld is a PIC32MZ based game console.  It is programmed with my own NES emulator, as well as the <a href="https://github.com/deltabeard/Peanut-GB">Peanut-GB Emulator</a>.  Most of the code is derived from my previous project <a href="https://github.com/stevenchadburrow/AcolyteHandPICd32">here</a>.<br>

<b>Features</b>

The Acolyte Handheld game console is switchable between VGA and LCD displays without needing to reboot.  It also supports four players simultaneously through the use of additional controllers.  The built-in NES emulator has been tested with over 120+ games at this point, including but not limited to the list <a href="https://github.com/stevenchadburrow/AcolyteHandPICd32/tree/main/NES">here</a>.  Most games run at full speed when at 20 FPS.  As a benchmark, Super Mario Bros 3 runs about 5% too slow.  On the other hand, the imported GB emulator is very accurate and runs games at full speed.<br>

<b>Microcontroller</b>

PIC32MZ2048EFH144 running at 260 MHz (slightly overclocked)<br>
Internal memory of 512KB of RAM and 2MB of Flash ROM<br>

<b>Video</b>

VGA 1024x768 resolution at 60 Hz with 256 colors or 65K colors<br>
ILI9341 LCD 320x240 resolution with 65K colors<br>
Effective resolution of NES emulator at 256x240, 3x integer scaled on VGA<br>
Effective resolution of GB emulator at 160x144, 3x integer scaled on VGA<br>

<b>Audio</b>

Dual 6-bit audio channels<br>
Headphone jack and internal speaker<br>

<b>HID</b>

2x Sega Genesis controller ports<br>
USB port for keyboard, mouse, or Xbox-360 controller<br>
14 built-in mappable buttons on device<br>

<b>File System</b>

Elm-Chan's <a href="https://elm-chan.org/fsw/ff/">FatFS Generic FAT Filesystem Module</a> for Micro TF Cards<br>
(use command 'sudo mkfs.vfat /dev/sdX' to format card)<br>

<b>UART</b>

FT232RL USB-to-UART adapter<br>
(use command 'sudo picocom /dev/ttyUSB0' for default 9600 baud connection)<br>

<b>NES Emulator</b>

My own NES Emulator, supporting NROM, UxROM, CxROM, AxROM, MMC1, and MMC3 mappers.<br>
Programmed with speed over accuracy in mind, runs most games at full speed without alteration.<br>
Supports settings adjustments, game save files, and save-states.<br>

<b>GB Emulator</b>

Mahyar Koshkouei's <a href="https://github.com/deltabeard/Peanut-GB">Peanut-GB (with MiniGB-APU) Emulator</a><br>
Supports GBC palettes and game save files.<br>

<b>Links</b>

<a href="http://forum.6502.org/">http://forum.6502.org/</a> for the 6502 Forum, an incredible resource for everything 6502 related.<br>
<a href="https://www.nesdev.org/wiki/Nesdev_Wiki">https://www.nesdev.org/wiki/Nesdev_Wiki</a> for the very best NES programming guidance.<br> 
<a href="https://www.aidanmocke.com/">https://www.aidanmocke.com/</a> for a bunch of PIC32MZ projects (without Harmony), including USB.<br>
<a href="http://elm-chan.org/">http://elm-chan.org/</a> for a bunch of projects, including MMC (aka TF Card) and FatFs.<br>
<a href="https://github.com/deltabeard/">https://github.com/deltabeard/</a> for a bunch of projects, including Peanut-GB and MiniGB-APU projects.<br>


<img src="HandheldPIC32-Image1.jpg">
<img src="HandheldPIC32-Image2.jpg">
<img src="HandheldPIC32-Image3.jpg">
<img src="HandheldPIC32-Image4.jpg">
<img src="HandheldPIC32-Image5.jpg">
<img src="HandheldPIC32-Image6.jpg">
<img src="HandheldPIC32-Image7.jpg">


