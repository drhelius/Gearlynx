# 4. CPU/ROM

The CPU is a 65C02 cell imbedded in the Mikey IC. The specifics of its instruction set and register operations are found in the VT1 specification.

## 4.1 CPU Cycle Timing

While the number of CPU cycles for each instruction are defined in the VTI spec, the number of system 'ticks' used by each CPU cycle are defined here. Some cycles will require a fixed number of ticks, some will require a variable number of ticks based on the instruction that preceded it, and some will require a variable (and sometimes non-deterministic) number of ticks based on the functioning of other pieces of hardware in the system. In addition, the CPU can be paused by the video and refresh accesses, and it can be interrupted by the timers and the serial port. The point of listing all of these variances to the CPU timing numbers is to advise the programmer that CPU cycles can not be used as the timing elements in small software timers. Under certain specific circumstances, the environment will be sufficiently controlled to allow for small software timers (initial mag tape reading), but in general, that practice should be avoided.

### 4.1.1 RAM Page Mode

The RAMs used in the system have a page mode operation in which one of the control signals (RAS) is not repeated for each memory access. This allows the cycle to be shorter than a normal access cycle. The requirement for using a page mode cycle is that the current access is in the same 256 address page of memory as the previous access. While comparing current and previous addresses is certainly a valid method of determining if a page mode cycle can be used, it usually takes the same or more time than just repeating the control signal. As a result, page mode is often not used by many CPU designers.

We use the method of decoding the current op-code to see if the next cycle could be a page mode cycle and then observing the other pertinent states of the system to see if some reason exists to NOT allow a page mode cycle. While this method is not 100% efficient in allowing all possible page mode opportunities, its silicon requirements are small and when properly designed it does not permit false page mode cycles. The CPU makes use of the page mode circuitry in its op-code reads. All writes and all data reads are done in normal memory cycles. A page mode op-code read takes 4 ticks, a normal read or write to RAM takes 5 ticks.

### 4.1.2 Hardware Accesses, DTACK

CPU accesses to hardware require an acknowledge signal (DTACK) from that hardware in order for the CPU cycle to proceed. These CPU accesses fall into 2 classes. One is for hardware that is always available and the other is for hardware that becomes available at a time not related to the CPU.

For always available hardware, the DTACK can be generated from the address decode and not cause the cycle to be longer than a RAM read or write cycle (5 ticks). Writes to Suzy are handled as 'always available' whether or not Suzy is actually available.

For eventually available hardware, (some of Suzy and some of Mikey), the DTACK is generated as a combination of the address decode, sometimes the column strobe, and the particular hardware being accessed. This cycle has a minimum requirement of 5 ticks and a maximum requirement of 128 ticks (probable actual maximum of 40). The maximum is not related to the CPU, it is required to prevent video and refresh underflows. All writes to Suzy are 'Blind' in that the write cycle is always 5 ticks long and does not get a DTACK. Suzy will accept the data immediately and then place it internally as required. The maximum time required for this internal placement is 6 ticks (except for writes to the game cart). This is less than the fastest turnaround time for 2 sequential writes to Suzy, so no collisions will occur. Poof for unsafe addresses. Some of the hardware in Mikey is constructed of Dual Ported RAMs addressed in a cyclical manner. These DPRAMs will have a maximum latency equal to their cycle time (1.25 us).

Some of the hardware in Suzy is also constructed of Dual Ported RAMs. These RAMs are not cycling, but their latency is still slightly variable (+1 tick, -0) due to clock synchronization.

The CPU accessible addresses in both Mikey and Suzy are not all readable and writable. See the hardware address appendix (appx 2) for the specifics.

### 4.1.3 CPU Cycle Tick Summary

| Cycle | Min | Max |
|-------|-----|-----|
| Page Mode RAM(read) | 4 | 4 |
| Normal RAM(r/w) | 5 | 5 |
| Page Mode ROM | 4 | 4 |
| Normal ROM | 5 | 5 |
| Available Hardware(r/w) | 5 | 5 |
| Mikey audio DPRAM(r/w) | 5 | 20 |
| Mikey color palette DPRAM(r/w) | 5 | 5 |
| Suzy Hardware(write) | 5 | 5 |
| Suzy Hardware(read) | 9 | 15 |

### 4.1.4 CPU NMI Latency

The NMI signal path from the pin of Mikey to the pin of the internal CPU contains clocked delays. This will make the usability of the pin questionable in the debug environment. Fortunately for us, Howard and Craig arranged the diagnostic hardware to still use it effectively. See the Howard board spec for details.

### 4.1.5 Suzy Bus Request-Bus Grant Latency

The maximum allowable latency at Suzy is constrained by the needs of the video DMA circuit. As a compromise between bigger FIFOs in the Mikey video DMA and reduced performance in Suzy, we are setting the maximum latency from Bus Request to Bus Grant at 2.5 us.

The time between Mikey requesting the bus and Suzy releasing it is dependant on the state of the currently running process inside of Suzy. The longest process is 30 ticks. Adding the overhead of accepting the bus request and releasing the bus grant brings the total to 40 ticks.

## 4.2 ROM

The system ROM is imbedded in Mikey. Its size is 512 bytes. Its main and perhaps only function is to read in the initial data from the data input system and then execute that data. In the case of the magnetic tape system, some error correction will be performed.

## 4.3 CPU Sleep

BIG NOTE: Sleep is broken in Mikey. The CPU will NOT remain asleep unless Suzy is using the bus. There is no point in putting the CPU to sleep unless you expect Suzy to take the bus. We will figure out how to save power some other way.

I will still keep the following paragraphs pertaining to sleep in this spec.

The CPU can put itself to sleep by writing to the CPU disable address. The CPU wants to sleep for two reasons. The first is that it is done with all of its work and we would like to conserve battery power. In this case, the CPU would pre-arrange for an interrupt to wake it up at the next time that work needs to be done (probably vertical restart). When awaken by an interrupt, normal interrupt vectoring takes place. Note that if no interrupt occurs, the CPU sleeps forever. An interrupt will wake you up even if they are disabled inside the CPU. The second reason to sleep is that Suzy has been requested (by the CPU) to do work. In that case, it is up to Suzy to awaken the CPU when it is done. When awaken by Suzy, processing continues from where it left off. The CPU cannot fall asleep unless it puts on its pajamas. This is accomplished by disabling all interrupts (register in Mikey) that you do not want to wake you up, then clearing them (in case they came in while you were disabling them), and then writing to the Suzy Wakeup Acknowledge address IF you were woken up by Suzy (or if this is the first time you are going to sleep). The purpose of the Suzy Wakeup Acknowledge address is to prevent missing the wakeup request from Suzy should it occur exactly at the same time as an interrupt.
