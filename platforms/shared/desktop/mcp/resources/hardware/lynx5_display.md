# 5. Display

## 5.1 Frame Rate

We have a programmable frame rate to accommodate either 50 or 60 Hz lighting systems. We may also want to vary the frame rate for 'game' reasons. While no vertical blanking is actually required by the LCD, the system likes to have some non-display time in which to cleanly change the display characteristics. In addition, multiple player games need some dead time between frames to re-synchronize to the master. Since reloading the color palette could take 150 us, at least 1 and perhaps 2 scan lines of time should be allocated as vertical blank time. The current method of driving the LCD requires 3 scan lines of vertical blank.

During vertical blank time, none of the display lines are allowed to be on. If they were, that line would be noticeably 'brighter' than the others. Additionally, the magic 'P' counter has to be set to match the LCD scan rate. The formula is:

```
INT((((line time - .5us) / 15) * 4) -1)
```

Don't forget to set the display control bits. Normal 4 bit color is:
'DISPCTL' (FD92) = x'0D'

Some frame rate choices are:

### 60Hz

159 us x 105 lines 16.695 ms (59.90 Hz), 3 lines of Vertical Blank

For normal 60Hz video operation, set the relevant timers as follows:
- Timer 0: clock=1us, backup=158. FD00=x'9E', FD01=x'18'
- Timer 2: clock=linking, backup=104. FD08=x'68', FD09=x'1F'
- Pcount: 'PBKUP' = 41. FD93=x'29'

### 50Hz

190 us x 105 lines = 19.950 ms (50.13 Hz), 3 lines of Vertical Blank.

For 50Hz video operation, set the relevant timers as follows:
- Timer 0: clock=1us, backup=189. FD00=x'BD', FD01=x'18'
- Timer 2: clock=linking, backup=104. FD08=x'68', FD09=x'1F'
- Pcount: 'PBKUP' = 49. FD93=x'31'

### 75Hz

127 us x 105 lines = 13.335 ms (74.99 Hz), 3 lines of Vertical Blank

For 75Hz video operation, set the relevant timers as follows:
- Timer 0: clock=1us, backup=126. FD00=x'7E', FD01=x'18'
- Timer 2: clock=linking, backup=104. FD08=x'68', FD09=x'1F'
- Pcount: 'PBKUP' = 32. FD93=x'20'

The maximum frame rate occurs when Hcount is 121 (backup = 120) which results in a vertical frequency of 78.7 Hz. 75 Hz (a useful rate) is achieved by setting the H backup value to 126 (x'7E'). The vertical values do not change.

We notice that 50Hz operation causes a massive flicker that is probably due to the speed of the LCD itself. 50 Hz may not be usable.

## 5.2 Pen Numbers/Color Palette

The display system has 4 bits of pen number per pixel in each display buffer. The color value of a particular pixel is converted from its pen number by the color palette in Mikey. There is only one color palette in the system. The color palette can be written to at any time (it is 'always available hardware'). The color display has rectangular shaped pixels (resulting in square triads) arranged in the following grid.

```
------------------------------------------------------
| R | G | B || R | G | B || R | G | B || R | G | B || .... | R | G | B |
------------------------------------------------------
------------------------------------------------------
| R | G | B || R | G | B || R | G | B || R | G | B || .... | R | G | B |
------------------------------------------------------
```

### 5.2.1 Flipped Color Palette

When the screen is flipped, the data is read from RAM starting at the bottom of the screen. This means that any screen related color palette changing must take 'flip' into account and reverse its order accordingly.

### 5.2.2 Monochrome Display

A long time ago, in a galaxy far away, monochrome display was considered. The chips still have some monochrome logic in them, and certain bits still need to be set in order to insure color operation. I have removed all non-pertinent monochrome information from the specifications. Please let me know if I missed any.

## 5.3 Display World/Suzy Painting Buffer/Mikey Display Buffer

The display world is an imaginary 64K pixel by 64K line space. Sprites may be positioned anywhere within the world by use of their 16 bit horizontal and 16 bit vertical positions. The origin of this world (0H, 0V) is the upper left corner.

The 160 pixel by 102 line Suzy painting buffer can be positioned anywhere in that world by setting a horizontal and vertical offset. The numbers sent to the Suzy hardware will be the distance from the upper left corner of the display world to the upper left corner of the Suzy painting buffer. Portions of sprites outside the Suzy painting buffer will be 'clipped' by the hardware and therefore not drawn into any RAM. Since the offset numbers are used in the placement of a sprite in real RAM, changing the numbers will affect previously drawn sprites differently from sprites yet to be drawn. Any sprites drawn prior to a change in the offset numbers will remain unmoved in the Suzy painting buffer. Any sprites drawn after the change in the offset numbers will be positioned in accordance with those new numbers.

The Mikey Display buffer is the 8160 byte (102 lines x 80 bytes) area of RAM that is displayed on the LCD.

### 5.3.1 Display Buffer Address Register

The software can elect to have any number of display buffers. Two will allow for normal double buffering of the screen.

The value in the register is the start (upper left corner) of the display buffer in normal mode and the end (lower right corner) of the display buffer in FLIP mode. The address of the upper left corner of any display buffer must have '00' in the bottom 2 bits (only the upper 14 bits of the address are used). The hardware registers in Suzy are loaded with the start (upper left corner) of the painting or collision buffer, regardless of FLIP mode.

### 5.3.2 Display Buffer Address Register

The hardware address in Mikey (DISPADDR) is the backup value for the actual address counter. The backup value is transferred to the address counter at the very start of the third line of vertical blanking. In addition, the actual address register can be changed in real time by writing a 1 into the bit 'VBLANKEF' of Mikey register 'MTEST2'.

## 5.4 External Power Plug Detector

The presence of a plug in the 'external power' jack can be detected by the hardware. Whether or not any power is provided by the inserted plug is not actually discernable, since inserting the plug will disconnect the batteries. Therefore, the detection of an inserted plug ought to mean that the unit is operating on externally supplied power. Please note that this external power might be a large battery pack in addition to being an AC adaptor. The main purpose of providing this information to the software is so that it can select the appropriate time-out values for the auto power down modes.

Since the I/O pin we are using used to be an output (backlight control), it is set to 'output' by the Mikey ROM. While this will not cause any damage to the parts, the pin will not function correctly until the software has set it to 'input'.
