# 3. Software Related Hardware Perniciousness

(or why some software people hate some hardware people)

There are certain things that the software ought not to do to the hardware. While these things are not physically destructive on their own, they will cause the hardware to act unpredictably strange, which may cause the user to be physically destructive.

While we certainly could have protected the system from many of the following problems, I doubt that we would have found them all. In addition, the act of software doing one of these things means that there is a problem in the software anyway and the intended function would probably not happen. Additionally, this is only a toy. If this unit were a bio-medical device I would have found a safer solution.

The things that software MUST NOT do and the precautions software MUST take are either intuitive nor obvious, so I will detail them here.

## 3.1 Don't Do These Things

If you do any of the things that I tell you not to do, the hardware will act unpredictably and will not be expected to recover without a complete system initialization. So don't do them.

### 3.1.1 Suzy SCB Register Accesses

All of the registers in the Suzy SCB are unsafe to touch while the sprite engine is in operation. This is true for BOTH read and write accesses. Prior to accessing any of those registers (they are identified in the hardware address appendix as 'unsafe') you must first check to see if the sprite engine is running (either for sprites or math). Those status bits are in SPRSYS.

### 3.1.2 Sequential Memory Accesses

CPU instructions that result in a Suzy access followed immediately by another Suzy access will probably break Suzy. Please do not do them.

### 3.1.3 Compiler Stuff

Some compilers do stupid things when they convert complex statements. For example, "A = B = C = 0" may get converted to: store 0 to C, read C and write it to B, read B and write it to A. This will break if you try it on any of my hardware, so unless you are real sure that your compiler will never do it, don't write your code that way.

### 3.1.4 Writes to the Game Cartridge

Writes to Suzy are blind, but will complete before any other CPU function can disturb them EXCEPT in the case of writes to the game cart. For 12 ticks after a write to the game cart is started, the CPU MUST NOT access Suzy (read or write). If it does, the write to the cart will be trashed.

### 3.1.5 Palettes and Page Breaks

This one is an actual hardware bug of mine. The hardware signal that ends the loading process for any SCB element comes 2 bytes prior to the actual end of the element. If a page break comes at the same time as the 'end' signal, then that end signal needs to be delayed by one cycle. If not, the loading process will end and not start up again at the end of the page break. Well, I screwed up the logic for the pen index palette loader. Therefore if a pen index palette starts at address xxFA, a page break will occur at the same time as the 'end' signal and this page break will be ignored by the palette loader. The last 2 bytes of this 8 byte palette will not be loaded from RAM, and the values loaded by the previous SCB will still be in the hardware. Pen index numbers C,D,E, and F will be incorrectly unchanged. Sometimes I am such a jerk.

## 3.2 Please Don't Do These Things

There are also things that software people do that merely annoy the hardware people rather than annoy the end user. This includes seemingly harmless activities like sub-decoding bit varies from byte descriptions (sprite types, for instance), or assuming address continuity between areas of hardware (like SPRCTLO follows the Sprite Control Block). Please don't do them.

It would be difficult to list all of the possibilities, so I will list the general considerations and ask you to be sensible about them. In addition, please feel free to submit a request for an exemption on a specific case. If it is granted, we will change the specification so as to make the special case forever legal. The price will be small.

The list: Do not decode any bits within any byte definitions. Do not assume any address continuity between hardware sections. Do not assume that any hardware addresses that are multiply mapped will remain so. Do not use un-defined bits within a byte for anything.

If you notice an unspecified but clever interaction between two or more sections of hardware, please come and tell me about it. It is more likely that it is a mistake rather than an omission from the spec.

I will try to list the approved exemptions in the spec.

Thank You

## 3.3 Do It This Way

Some of the hardware functions, as designed by us mindless brutes, require special handling. As we discover these requirements, I will list them here.

### 3.3.1 Timer Reload Disable

If a particular timer reload bit is set to disable, the timer ought to count down to zero, stop counting, and set its 'timer done' bit. However, if the 'timer done' bit was already set, the timer will not even do its first count. Therefore, when the setting of 'timer reload' is 'disabled', you must clear the 'timer done' bit in order for the timer to count at all.

This 'clear' can be accomplished by setting the 'reset timer done' bit, but that can cause a new and exciting problem. The 'clear' can also be accomplished by writing directly to the byte that contains the 'timer done' bit. The answer of choice is to write a 0 to bit 3 of the control byte (TIMnCTLB). Since the other bits are adjusted by hardware, I recommend writing a full byte of 0 to that register when you need to do the 'clear' function.

### 3.3.2 Reset Timer Done

The 'reset timer done' bit in the hardware registers was mistakenly designed as a 'level' signal rather than a 'pulse' signal. Therefore, to use it correctly, you must 'pulse' it in software. This means first setting it high, and then setting it low. The problem with just leaving it high is that the dumb hardware may (depending on other conditions in that particular timer) send a continuous interrupt to the interrupt logic. In addition, since a lucky interrupt might actually slip in during this 'software pulse', the recommended process is to clear the interrupt enable bit at the same time that the reset timer done bit is set. Happily, both bits are in the same register. Please remember to restore the 'enable' bit as you release the 'reset' bit.
