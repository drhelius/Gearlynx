# 2. Hardware Overview

(see the Handy Block Diagram, appx 1)

The system hardware consists of:
- Mikey and Suzy custom digital ICs.
- A 16 MHz crystal.
- Two 64k by 4 DRAMs.
- A 2 inch speaker with an earphone jack and volume control.
- An LCD and its related drivers, backlight, and contrast control.
- A data input system. (Tape or ROM)
- Batteries, power supply, and external power jack.
- An expansion port.
- A joystick, 2 fire buttons, and other switches.

The division of circuitry between the 2 digital ICs is that Mikey contains all of the non-sprite hardware and Suzy is only a sprite generation engine. Some non-sprite functions (the switch readers and the ROM reader) are in Suzy due to pin limitations. In addition the math functions are part of Suzys sprite engine.

The crystal is the only source of timing information in the system. The basic timing tick of the system is 62.5 ns. Let us now define the term tick to be 62.5 ns.

The system RAM is 64K bytes. This RAM houses the video buffer(s) and collision buffer (total maximum of 24K bytes) in addition to the game software (worst case minimum of 40K bytes). The RAMs have a 120ns RAS access time and 60ns page mode CAS access time. This allows us to have a 125ns (8MHz) page mode memory access rate and a 250ns (4MHz) normal memory access rate.

The speaker is a 2 inch diameter 8 ohm speaker. The volume control range includes zero. The earphone jack is the standard stereo 'Walkman' style (only mono sound, however).

The LCD has a resolution of 480 horizontal pixels by 102 vertical pixels. Three pixels, one of each color, form a square triad with a resultant screen resolution of 160 triads by 102 lines. The column drivers can generate 16 levels of intensity for each pixel, resulting in a palette of 4096 colors. The LCD circuitry includes the power generation for the LCD driver ICs, the decoding of the Mikey strobes for the LCD driver ICs, and the power generation for the backlight.

The data input system is either a ROM cartridge or a magnetic tape reader. The system hardware will support both, but units will be made with either one or the other. The data input systems are explained elsewhere.

The power system provides raw power to the regulator, and if used, the motor. The regulator has a soft on/off function so that the system can power itself off when not in use. This is required so as to avoid the customer frustration of expensive battery replacements which could then cause loss of software revenue. The motor is separately powered so that its load is not part of the regulators problem. The soft off function also disconnects it from the power source.

The expansion port has a bi-directional serial port operating asynchronously at a programmable speed with a maximum of 62500 baud. This is approximately 104 bytes per 60 Hz frame. By allowing all of the games in a multiple player game to operate one frame behind their control functions, up to 104 bytes of data can be communicated. Using an example maximum of 8 players in a group and for some overhead, 12 bytes per player are available.

The human input controls consist of a 4 switch (8 position) joy stick, two sets of 2 independent fire buttons, game pause button, 2 flablode buttons, power on, and power off. The two sets of fire buttons are wired together, there are only 2 'fire' signals. Two sets are available to allow for left and right handed operation. Flablode is a Jovian word meaning a device or function that we know is required or desired but for which we don't have an actual definition (noun: flabloden, verb: to flablode).
