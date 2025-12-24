# 10. Timers/Interrupts

## 10.1 Timers

There are 8 independent timers. Each has the same construction as an audio channel, a 3 bit source period selector and an 8 bit down counter with a backup register. This gives a timer range of 1 us to 16384 us. Timers can be set to stop when they reach a count of 0 or to reload from their backup register. In addition, they can be linked, with the reload of one timer clocking the next timer. The linking order is as follows:

**Group A:**
Timer 0 -> Timer 2 -> Timer 4.

**Group B:**
Timer 1 -> Timer 3 -> Timer 5 -> Timer 7 -> Audio 0 -> Audio 1 -> Audio 2 -> Audio 3 -> Timer 1.

As with the audio channels, a count of 0 is valid for 1 full cycle of the selected clock. A backup value of 5 will result in 6 units of time. Actually, due to hardware limitations, the first utilization of that timer after it has been set will result in a time value between 5 and 6 units. Subsequent utilizations (reload is 'on') will result in a value of 6 units.

## 10.2 Timer Utilization

Two of the timers will be used for the video frame rate generator. One (timer 0) is set to the length of a display line and the second (timer 2) is set to the number of lines. One of the timers (timer 4) will be used as the baud rate generator for the serial expansion port (UART).

Note that the hardware actually uses the bits in these timers and it would be dangerous to arbitrarily fiddle with them in software. The other 5 timers are for general software use. POOF. See the mag tape description.

## 10.3 Interrupts

7 of the 8 timers can interrupt the CPU when it underflows. Each interrupt can be masked. The value of the interrupt bit can be polled independent of its mask condition. The interrupt bit for timer 4 (UART baud rate) is driven by receiver or transmitter ready bit of the UART.

If an interrupt occurs while the CPU is asleep, it will wake up the CPU. Since one of the timers is the vertical line counter, the useful 'end of frame' interrupt can be generated there.

The interrupt signal comes from the timer when the timer value is zero AND the timer is attempting to perform a 'borrow'. Based on the control bits, the borrow may not actually occur, but the interrupt signal will. This signal then request the bus control circuit to give the bus to the CPU. If the CPU already has the bus, then this function causes no delays. If Suzy has the bus, then the maximum Suzy latency time could be incurred. Then, the interrupt signal waits for the end of the current CPU cycle before actually interrupting the CPU.
