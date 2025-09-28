/*
 * Gearlynx - Lynx Emulator
 * Copyright (C) 2025  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#ifndef MIKEY_DEFINES_H
#define MIKEY_DEFINES_H

//#define GLYNX_DEBUG_MIKEY

#if defined(GLYNX_DEBUG_MIKEY)
    #define DebugMikey(msg, ...) Debug("· MIKEY [PC=%04X]: " msg, m_m6502->GetState()->PC.GetValue(), ##__VA_ARGS__)
#else
    #define DebugMikey(msg, ...)
#endif

#define MIKEY_TIM0BKUP      0xFD00
#define MIKEY_TIM0CTLA      0xFD01
#define MIKEY_TIM0CNT       0xFD02
#define MIKEY_TIM0CTLB      0xFD03
#define MIKEY_TIM1BKUP      0xFD04
#define MIKEY_TIM1CTLA      0xFD05
#define MIKEY_TIM1CNT       0xFD06
#define MIKEY_TIM1CTLB      0xFD07
#define MIKEY_TIM2BKUP      0xFD08
#define MIKEY_TIM2CTLA      0xFD09
#define MIKEY_TIM2CNT       0xFD0A
#define MIKEY_TIM2CTLB      0xFD0B
#define MIKEY_TIM3BKUP      0xFD0C
#define MIKEY_TIM3CTLA      0xFD0D
#define MIKEY_TIM3CNT       0xFD0E
#define MIKEY_TIM3CTLB      0xFD0F
#define MIKEY_TIM4BKUP      0xFD10
#define MIKEY_TIM4CTLA      0xFD11
#define MIKEY_TIM4CNT       0xFD12
#define MIKEY_TIM4CTLB      0xFD13
#define MIKEY_TIM5BKUP      0xFD14
#define MIKEY_TIM5CTLA      0xFD15
#define MIKEY_TIM5CNT       0xFD16
#define MIKEY_TIM5CTLB      0xFD17
#define MIKEY_TIM6BKUP      0xFD18
#define MIKEY_TIM6CTLA      0xFD19
#define MIKEY_TIM6CNT       0xFD1A
#define MIKEY_TIM6CTLB      0xFD1B
#define MIKEY_TIM7BKUP      0xFD1C
#define MIKEY_TIM7CTLA      0xFD1D
#define MIKEY_TIM7CNT       0xFD1E
#define MIKEY_TIM7CTLB      0xFD1F

#define MIKEY_AUD0VOL       0xFD20
#define MIKEY_AUD0SHFTFB    0xFD21
#define MIKEY_AUD0OUTVAL    0xFD22
#define MIKEY_AUD0L8SHFT    0xFD23
#define MIKEY_AUD0TBACK     0xFD24
#define MIKEY_AUD0CTL       0xFD25
#define MIKEY_AUD0COUNT     0xFD26
#define MIKEY_AUD0MISC      0xFD27

#define MIKEY_AUD1VOL       0xFD28
#define MIKEY_AUD1SHFTFB    0xFD29
#define MIKEY_AUD1OUTVAL    0xFD2A
#define MIKEY_AUD1L8SHFT    0xFD2B
#define MIKEY_AUD1TBACK     0xFD2C
#define MIKEY_AUD1CTL       0xFD2D
#define MIKEY_AUD1COUNT     0xFD2E
#define MIKEY_AUD1MISC      0xFD2F

#define MIKEY_AUD2VOL       0xFD30
#define MIKEY_AUD2SHFTFB    0xFD31
#define MIKEY_AUD2OUTVAL    0xFD32
#define MIKEY_AUD2L8SHFT    0xFD33
#define MIKEY_AUD2TBACK     0xFD34
#define MIKEY_AUD2CTL       0xFD35
#define MIKEY_AUD2COUNT     0xFD36
#define MIKEY_AUD2MISC      0xFD37

#define MIKEY_AUD3VOL       0xFD38
#define MIKEY_AUD3SHFTFB    0xFD39
#define MIKEY_AUD3OUTVAL    0xFD3A
#define MIKEY_AUD3L8SHFT    0xFD3B
#define MIKEY_AUD3TBACK     0xFD3C
#define MIKEY_AUD3CTL       0xFD3D
#define MIKEY_AUD3COUNT     0xFD3E
#define MIKEY_AUD3MISC      0xFD3F

#define MIKEY_ATTEN_A       0xFD40
#define MIKEY_ATTEN_B       0xFD41
#define MIKEY_ATTEN_C       0xFD42
#define MIKEY_ATTEN_D       0xFD43
#define MIKEY_MPAN          0xFD44
#define MIKEY_MSTEREO       0xFD50
#define MIKEY_INTRST        0xFD80
#define MIKEY_INTSET        0xFD81
#define MIKEY_MAGRDY0       0xFD84
#define MIKEY_MAGRDY1       0xFD85
#define MIKEY_AUDIN         0xFD86
#define MIKEY_SYSCTL1       0xFD87
#define MIKEY_MIKEYHREV     0xFD88
#define MIKEY_MIKEYSREV     0xFD89
#define MIKEY_IODIR         0xFD8A
#define MIKEY_IODAT         0xFD8B
#define MIKEY_SERCTL        0xFD8C
#define MIKEY_SERDAT        0xFD8D
#define MIKEY_SDONEACK      0xFD90
#define MIKEY_CPUSLEEP      0xFD91
#define MIKEY_DISPCTL       0xFD92
#define MIKEY_PBKUP         0xFD93
#define MIKEY_DISPADRL      0xFD94
#define MIKEY_DISPADRH      0xFD95
#define MIKEY_MTEST0        0xFD9C
#define MIKEY_MTEST1        0xFD9D
#define MIKEY_MTEST2        0xFD9E
#define MIKEY_GREEN0        0xFDA0
#define MIKEY_GREEN1        0xFDA1
#define MIKEY_GREEN2        0xFDA2
#define MIKEY_GREEN3        0xFDA3
#define MIKEY_GREEN4        0xFDA4
#define MIKEY_GREEN5        0xFDA5
#define MIKEY_GREEN6        0xFDA6
#define MIKEY_GREEN7        0xFDA7
#define MIKEY_GREEN8        0xFDA8
#define MIKEY_GREEN9        0xFDA9
#define MIKEY_GREENA        0xFDAA
#define MIKEY_GREENB        0xFDAB
#define MIKEY_GREENC        0xFDAC
#define MIKEY_GREEND        0xFDAD
#define MIKEY_GREENE        0xFDAE
#define MIKEY_GREENF        0xFDAF
#define MIKEY_BLUERED0      0xFDB0
#define MIKEY_BLUERED1      0xFDB1
#define MIKEY_BLUERED2      0xFDB2
#define MIKEY_BLUERED3      0xFDB3
#define MIKEY_BLUERED4      0xFDB4
#define MIKEY_BLUERED5      0xFDB5
#define MIKEY_BLUERED6      0xFDB6
#define MIKEY_BLUERED7      0xFDB7
#define MIKEY_BLUERED8      0xFDB8
#define MIKEY_BLUERED9      0xFDB9
#define MIKEY_BLUEREDA      0xFDBA
#define MIKEY_BLUEREDB      0xFDBB
#define MIKEY_BLUEREDC      0xFDBC
#define MIKEY_BLUEREDD      0xFDBD
#define MIKEY_BLUEREDE      0xFDBE
#define MIKEY_BLUEREDF      0xFDBF

#endif /* SUZY_DEFINES_H */
