# 12. Other Hardware Features

1. Hardware Multiply, Accumulate and Divide
2. Upward Compatibility
3. Parallel Port
4. Qbert Root
5. Everon
6. Unsafe Access
7. Howie Notification
8. General I/O Port

## 12.1 Hardware Multiply, Accumulate and Divide

We have a 16 by 16 to 32 unsigned and signed multiply with accumulate and a 32 / 16 into 32 unsigned divide. The results of a multiply are a 32 bit product, a 32 bit accumulate and an accumulator overflow bit. The results of a divide are a 32 bit dividend, a 16 bit remainder, and a flag bit indicating a divide by zero. The number in the dividend as a result of a divide by zero is 'FFFFFFFF' (BigNum). The accumulator is 32 bits and accumulates the result of multiply operations.

The basic method of performing one of these math operations is to write the starting values into Suzy registers (all of the addresses are different) and then polling for completion prior to reading the results. The act of writing to the last register starts the math process.

The functions of 'sign/unsign' and 'accumulate' are optional. The value for SPRINIT is currently unchanged for the multiply and divide operations. The following picture shows the operations available.

Each letter represents a different byte address. These addresses are identified in the hardware address Appx 2. Each grouping represents the kind of math operation available.

```
AB      EFGH
*CD     /NP
EFGH    ABCD
Accumulate in JKLM     Remainder in (JK)LM
```

Some Rules:

Writing to B,D,F,H,K, or M will force a '0' to be written to A,C,E,G,J, or L, respectfully. Therefore, if you only have 8 bits in a particular number, there is no need to write the upper byte to '0'. (except for signed multiplies)

Writing to A will start a 16 bit multiply.

Writing to E will start a 16 bit divide.

The actual steps required to perform some of the functions are:

**16 x 16 multiply:**
- Write LSB to D, MSB to C
- Write LSB to B, MSB to A
- Poll MATHSTAT until done (or just wait for 54 ticks)
- Read answer (LSB->MSB) from H,G,F,E

**Accumulate:**
To initialize the accumulator, write a '0' to K and M (This will put 0 in J and L). The write to 'M' will clear the accumulator overflow bit. Note that you can actually initialize the accumulator to any value by writing to all 4 bytes (J,K,L,M).

**32 / 16 divide:**
- Write LSB to P, MSB to N,
- Write LSB -> MSB to H,G,F,E
- Poll MATHSTAT until done (or just wait 400 ticks max)
- Read answer (LSB->MSB) from D,C,B,A
- Read remainder (LSB->MSB) from M,L

As a courtesy, the hardware will set J,K to zero so that the software can treat the remainder as a 32 bit number.

### 12.1.1 Signed Multiplies

When signed multiply is enabled, the hardware will convert the number provided by the CPU into a positive number and save the sign of the original number. The resultant positive number is placed in the same Suzy location as the original number and therefore the original number is lost. At the end of a multiply, the signs of the original numbers are examined and if required, the multiply result is converted to a negative number.

The conversion that is performed on the CPU provided starting numbers is done when the upper byte is sent by the CPU. Therefore, while writing to the lower byte will set the upper byte to zero, it WILL NOT change its sign to positive. Therefore, when using signed multiply, you MUST write both bytes of a number.

### 12.1.2 Repetitive Multiplies and Divides

Since none of the input values are disturbed, repetitive multiplies and divides are easily done by only changing the required bytes and the 'starting' byte. There are several restrictions in the utilization of hardware multiply and divide:

- Suzy is not processing sprites.
- The steps to perform the function must be performed exactly as specified. Failure to do so will result in botched data.
- The hardware is not re-entrant. This means that once started, a multiply or divide can not be messed with.

There is an interrupt consideration. Although the hardware is not re-entrant, some clever interrupt routine may wish to wait for an operation to complete, save the answer, perform its own operation, get its own answer, and then restore the previous values. While this works for the simple singular operation, it will disturb the input values which may still be required by the original software that is performing repetitive operations. If the interrupt software wants to save and restore the original input values, it will start a new math operation when it writes them back to the hardware. The various implications of this act have not yet been explored, so be cautious.

### 12.1.3 Math Timing

Multiplies without sign or accumulate take 44 ticks to complete.

Multiplies with sign and accumulate take 54 ticks to complete.

Divides take 176 + 14*N ticks where N is the number of most significant zeros in the divisor.

### 12.1.4 Bugs in MathLand

**BIG NOTE:** There are several bugs in the multiply and divide hardware. Lets hope that we do not get a chance to fix them.

- In signed multiply, the hardware thinks that 8000 is a positive number.
- In divide, the remainder will have 2 possible errors, depending on its actual value. No point in explaining the errors here, just don't use it. Thank You VTI.
- In signed multiply, the hardware thinks that 0 is a negative number. This is not an immediate problem for a multiply by zero, since the answer will be re-negated to the correct polarity of zero. However, since it will set the sign flag, you can not depend on the sign flag to be correct if you just load the lower byte after a multiply by zero.
- The overflow bit is not permanently saved in the hardware. You must read the overflow bit before doing anything to Suzy that might disturb it. Basically, read the overflow bit before you access the SCB registers.

## 12.2 Upward Compatibility

Hardware changes can be desired for many reasons. We will probably not implement desired hardware changes for many other reasons. However, we may introduce product upgrades for which software compatibility is important.

To support the upgrades, a read only hardware identifier register and a write only software identifier register in both Mikey and Suzy (4 registers total) will be provided. These registers will have 'bit' significance in that the individual bits will have individual revision or capability significance.

In the first Mikey and Suzy, the hardware identifier value will be '01'. The software identifier register will not actually exist, only its address is allocated.

## 12.3 Parallel Port

Some of the prototypes will have a parallel port for debug purposes. The port uses 2 addresses, one for status and one for data. Both are read/write. See the address appendix for the details. Basically, you write to the status address to set the port for either input or output, and to wiggle the 'Paper Out' pin. You read the status port to see if the port has data available, is ready for data, or read a the 'Busy' pin. The hardware handshake timing on the parallel port itself is totally handled in hardware. Of special note is that changing the direction of the data port will clear the data available bit.

Additionally, the 'SEL' line in the parallel cable is connected to the NMI/ line of the CPU. Pulling it low will cause an NMI to happen in the handy CPU. Don't forget to set it high again before the handy CPU finishes the NMI software routine. If you are wiggling it from an Amiga, you can pull it high as soon as you want to. The Amiga hardware will not let its duration be too short.

## 12.4 Qbert Root

As a compromise between the square root and qube root desires of the software people, and the schedule desires of the management, we have decided to incorporate the function of QbertRoot. The required steps are:

- Start a 16 by 16 multiply.
- Immediately write to 'E' which will try to start a divide.
- Read the result from 'D,C,B,A'.

## 12.5 Everon

At a particular point in the life of a sprite, it may move in such a manner that it no longer appears on the screen. It may also have started its life off-screen. It appears to be useful to the software to know that a sprite is completely off-screen. The function used to discover this truth is called 'Everon' and is enabled by the 'Everonoff' bit. The function is enabled when the Sprite Engine is started with an 05 instead of an 01 at 'SPRGO'. This function will cause the 'Everon' bit to reflect the off-screen situation of a particular sprite. This bit is returned to each SCB in bit 7 of the collision depository. The bit will be a '0' if Everon is disabled or if the sprite is ever on the screen. The bit is a '1' only when Everon is enabled (Everonoff is on) and the sprite is never on the screen.

## 12.6 Unsafe Access

As expressed earlier in this document, there are times when certain addresses are unsafe to access. If, by some unknown means, the software does an unsafe access, the unsafe access bit in SPRSYS will be set. This bit will remain set until it is cleared as explained in the hardware address appendix.

**BIG NOTE:** Unsafe access is broken for math operations. Please reset it after every math operation or it will not be useful for sprite operations.

## 12.7 Howie Notification

For purposes of diagnostics, there is a Suzy address that is used to notify the Howard board of a desired communication. See the hardware address spec.

## 12.8 General I/O Port

In the beginning, there was a general purpose 8 bit I/O port. As pins on Mikey became unavailable, the number of bits was reduced. Now all we have are the 5 bits of IODAT and they are not even pure read/write.

The direction of the pins (in or out) still needs to be set even though all but one are forced in the PCB to be either an in or an out but not both. The function of the bits are not apparent from the description in the address appendix, so I will explain them here.

### External Power

This bit detects the presence of a powered plug. The ROM sets it to an output, so the system code must set it to an input.

### Cart Address Data

This bit must be set to an output. It has 2 functions. One is that it is the data pin for the shifter that holds the cartridge address. The other is that it controls power to the cartridge. Power is on when the bit is low, power is off when the bit is high.

### Noexp

This bit must be set to an input. It detects the presence of a plug in the expansion connector.

### Rest

This bit must be set to an output. In addition, the data value of this bit must be set to 1. This bit controls the rest period of the LCD display. The actual Rest signal is a high except during the last 2 scan lines of vertical blank and the first scan line after vertical blank. It is generated by the vertical timing chain in Mikey but its output on this pin can be disabled by the incorrect setting of this bit. In addition, when reading this bit, you will get the actual state of the Rest signal 'anded' with the value of the bit set in IODAT. Yes, we know that the polarity of the name is wrong, but that is the way it came to us.

### Audin

This bit can be an input or an output. In its current use, it is the write enable line for writeable elements in the cartridge. It can also be used as an input from the cartridge such as 'audio in' for 'talking-listening' games. Whether it is set to input or output, the value read on this pin will depend on the electronics in the cartridge that is driving it.

### The other bits

The other 3 bits in the byte are not connected to anything specific. Don't depend on them being any particular value.

# 13. Expansion Connector

The expansion connector is a 3 wire stereo earphone jack. The pin assignments are:

- Shield: Ground
- Tip: VCC
- Center: Data

There are certain considerations for data transfer, more later.

Note that when using a wire, any unit that is powered off and connected to the link will disable communications for all of the units in the link.
