# 14. System Reset/Power Up

The state of all of the control bits during and immediately after system reset is given in appendix 2. During reset, the main clocks are still running and are used to propagate the reset condition thru the logic. Reset must be long enough to insure a stable and consistent start up. The process that takes place upon release from system reset and the steps required to initialize the hardware are described below.

## 14.1 Suzy Reset Recovery

At release from reset, all of Suzy is disabled and remains so until enabled by the CPU. During reset, the bus handshake logic has stabilized with bus grant asserted. Any incidental internal activity will be listed when the logic design is completed. Suzy is not expected to require any software initialization until a specific function of Suzy is desired.

## 14.2 Mikey Reset Recovery

At release from system reset, the video bus request is disabled and will remain so until enabled by the CPU. The CPU is requesting the bus. During reset, the bus handshake logic has stabilized and is ready to give the bus to the CPU. The ROM overlay is enabled and the CPU will fetch its reset vector from ROM at the normal 6502 address of FFFC and FFFD.

Since there are some pieces of hardware that are running at rates slower than the main system clock, there is the possibility that there will not be a deterministic relationship between the phases of these slower clocks (eg. CPU and audio/timer). To prevent possible peculiarities of operation (due to hardware bugs) and to assist in truthful software emulation, it is suggested that these individual hardware chunks get synchronized at system start. It may be expensive to do it all in silicon, so some of them may need to be done by software. At this time, Glenn says that the audio section is done in silicon. There are other hardware registers that must be initialized correctly in order to let the system come up consistently. They are stated in appendix 2 (hardware addresses).

The current process that must be followed at system initialization is:

1. Disable interrupts
2. Clear decimal mode
3. Read SUZYHREV
4. If =0, jump to test code
5. Read 8 locations in RAM (each must cause a new RAS to occur)

(more ??)

# 15. Definitions and Terminology

The polarity of signals and their effect on the circuitry is not always apparent from the name of the signals. For this document and its related documents and appendices, the following conventions will be used:

**Set = On = Asserted = Active = Occurs**

These mean that the signal is in the state that causes or allows its named function to occur. For example, if bus grant is set, we are granting a bus. If an interrupt mask bit is set, that interrupt is masked.

**Reset = Off = Cleared = De-asserted = Inactive = Dropped**

These mean that the signal is in the state that does not allow its named function to occur. For example, when bus request is dropped, we are no longer requesting the bus.

The terms 'hi' and 'low' are well applied to actual schematic referenced signals, but should be avoided in generally descriptive text.

**Acquired = Taken**

These relate to tri-state busses and indicate that the bus is now being driven by the 'acquirer'.

**Released = Let Go**

These also relate to tri-state busses and indicate that the 'releaser' is no longer driving the bus.

**SimWait = Boil**

The act of waiting for garbage collection on the VTI tools

# 16. Known Variances From Anticipated Optimums

This is a list of the known bugs in the hardware.

## 16.1 Mikey

- Sleep does not work if Suzy does not have the bus.
- The UART interrupt is not edge sensitive.
- The UART TXD signal powers up in TTL HIGH, instead of open collector.
- The lower nybble of the audio out byte is processed incorrectly.
- The Reset Timer Done bit is not a pulse.
- The Timer Done bit requires clearing in order to count.
- The Mikey ROM sets the External Power Detect pin to output.
- The REST pin on Mikey is initialized as an input and its data is set to 0. Both are wrong. You must set it to an output with a data value of 1.
- The IODAT register is not really a R/W register.

## 16.2 Suzy

- Remainder does not correctly handle the upper bit in two ways.
- Signed multiply thinks 8000 is a positive number.
- Auto clear of the upper byte of an SCB word does not also auto clear the sign flag.
- The page break signal does not delay the end of the pen index palette loading.
- The polarity of the 'shadow' attribute is inverted.
- Signed multiply thinks that 0 is a negative number. (delayed effect)
- The circuit that detects a '0' in the SCB NEXT field of an SCB only looks at the upper byte. Therefore, we can't have scabs in page 0. I'm sorry.
- A data packet header of '00000' can be used to indicate end of data. There appears to be a bug with it. I don't understand it yet. Beat me. Kick me.

# 17. Approved Exemptions

1. Only the upper byte of the 'NEXT' word in the SCB needs to be set to zero in order to indicate that this is the last SCB in the list. The lower byte of that word can then be used for any other function since the hardware will ignore it if the upper byte is zero.

2. Some of the sprite values (H size, V size, etc) are re-usable. The normal way to reuse them is to have the first sprite in the local list actually initialize the values, and then have the remaining sprites in the local list re-use them. One of the difficulties in doing it this way is that it is not always reasonable to arrange the list and the SCBs appropriately. One of the ways to simplify the problem is to use an initializing sprite with the desired numbers in the reusable registers.

   I have been asked to provide an exemption that would allow the software to write directly to the hardware registers in the Suzy SCB and thus avoid all of the overhead of arranging the lists or creating null sprites. Since this section of hardware is firmly tied to the sprite software process, I believe that it will be OK to write directly to the hardware registers. I could be wrong, so I am going to approve the following conditional exemption.

   It will be OK to write to the twenty-four 16 bit registers of the Suzy SCB PROVIDING that you only do it via the MACRO PROVIDED TO YOU BY RJ MICAL. In addition, you must understand that the contents of this macro may change if future revisions of the hardware so require. In addition, you must understand that future hardware may make the process not work and therefore the macro will be changed to be a warning that you can't use it anymore.

   Don't cheat.

# 18. Common Errors

There are errors that many first time programmers and some experienced programmers make with this hardware. Some of these errors may be difficult to identify due to the complexity of the ICs. I will list some of them here to aid in the debugging of 'Mystery Bugs'.

1. The presence of an interrupt in Mikey, regardless of the state of the CPU enable interrupt bit, will prevent the CPU from going to sleep, and thus prevent Suzy from functioning. So if sprites stop working, unintentional interrupt bits can be the hidden cause.

2. The Suzy Done Acknowledge address must be written to prior to running the sprite engine. This is required even prior to the first time the sprite engine is activated. If it is not written to in the appropriate sequences, the CPU will not go to sleep when so requested. In addition, if some software accidentally allows a Suzy operation to complete without then following that completion with a write to SDONEACK, the CPU will not sleep. So if sprites stop working, something may have gone wrong with your SDONEACK software.

3. Writes to the cartridge are blind. If you accidentally access Suzy before the required delay period, you will modify some internal bus in Suzy. The results are not definable.

# 19. Savegame-Option

(c) by Bastian Schick

It's possible to save high-scores or whatever info on a EEPROM provided on some carts. For best results use the EEPROM routines provided with the BLL dev.kit.
