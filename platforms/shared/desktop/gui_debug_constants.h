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

#ifndef GUI_DEBUG_CONSTANTS_H
#define GUI_DEBUG_CONSTANTS_H

#include "imgui.h"
#include "gearlynx.h"

static const ImVec4 cyan =          ImVec4(0.10f, 0.90f, 0.90f, 1.0f);
static const ImVec4 dark_cyan =     ImVec4(0.00f, 0.30f, 0.30f, 1.0f);
static const ImVec4 magenta =       ImVec4(1.00f, 0.50f, 0.96f, 1.0f);
static const ImVec4 dark_magenta =  ImVec4(0.30f, 0.18f, 0.27f, 1.0f);
static const ImVec4 yellow =        ImVec4(1.00f, 0.90f, 0.05f, 1.0f);
static const ImVec4 dark_yellow =   ImVec4(0.30f, 0.25f, 0.00f, 1.0f);
static const ImVec4 orange =        ImVec4(1.00f, 0.50f, 0.00f, 1.0f);
static const ImVec4 dark_orange =   ImVec4(0.60f, 0.20f, 0.00f, 1.0f);
static const ImVec4 red =           ImVec4(0.98f, 0.15f, 0.45f, 1.0f);
static const ImVec4 dark_red =      ImVec4(0.30f, 0.04f, 0.16f, 1.0f);
static const ImVec4 green =         ImVec4(0.10f, 0.90f, 0.10f, 1.0f);
static const ImVec4 dark_green =    ImVec4(0.03f, 0.20f, 0.02f, 1.0f);
static const ImVec4 violet =        ImVec4(0.68f, 0.51f, 1.00f, 1.0f);
static const ImVec4 dark_violet =   ImVec4(0.24f, 0.15f, 0.30f, 1.0f);
static const ImVec4 blue =          ImVec4(0.20f, 0.40f, 1.00f, 1.0f);
static const ImVec4 dark_blue =     ImVec4(0.07f, 0.10f, 0.30f, 1.0f);
static const ImVec4 white =         ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
static const ImVec4 gray =          ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
static const ImVec4 mid_gray =      ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
static const ImVec4 dark_gray =     ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
static const ImVec4 black =         ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
static const ImVec4 brown =         ImVec4(0.68f, 0.50f, 0.36f, 1.0f);
static const ImVec4 dark_brown =    ImVec4(0.38f, 0.20f, 0.06f, 1.0f);

static const char* const c_cyan = "{19E6E6}";
static const char* const c_dark_cyan = "{004C4C}";
static const char* const c_magenta = "{FF80F5}";
static const char* const c_dark_magenta = "{4C2E45}";
static const char* const c_yellow = "{FFE60D}";
static const char* const c_dark_yellow = "{4C4000}";
static const char* const c_orange = "{FF8000}";
static const char* const c_dark_orange = "{993300}";
static const char* const c_red = "{FA2673}";
static const char* const c_dark_red = "{4C0A29}";
static const char* const c_green = "{19E619}";
static const char* const c_dark_green = "{083305}";
static const char* const c_violet = "{AD82FF}";
static const char* const c_dark_violet = "{3D274D}";
static const char* const c_blue = "{3366FF}";
static const char* const c_dark_blue = "{12194D}";
static const char* const c_white = "{FFFFFF}";
static const char* const c_gray = "{808080}";
static const char* const c_mid_gray = "{666666}";
static const char* const c_dark_gray = "{1A1A1A}";
static const char* const c_black = "{000000}";
static const char* const c_brown = "{AD805C}";
static const char* const c_dark_brown = "{61330F}";

struct stDebugLabel
{
    u16 address;
    const char* label;
};

static const int k_debug_label_count = 204;
static const stDebugLabel k_debug_labels[k_debug_label_count] = 
{
    { 0xFC00, "SUZY_TMPADRL" },
    { 0xFC01, "SUZY_TMPADRH" },
    { 0xFC02, "SUZY_TILTACUML" },
    { 0xFC03, "SUZY_TILTACUMH" },
    { 0xFC04, "SUZY_HOFFL" },
    { 0xFC05, "SUZY_HOFFH" },
    { 0xFC06, "SUZY_VOFFL" },
    { 0xFC07, "SUZY_VOFFH" },
    { 0xFC08, "SUZY_VIDBASL" },
    { 0xFC09, "SUZY_VIDBASH" },
    { 0xFC0A, "SUZY_COLLBASL" },
    { 0xFC0B, "SUZY_COLLBASH" },
    { 0xFC0C, "SUZY_VIDADRL" },
    { 0xFC0D, "SUZY_VIDADRH" },
    { 0xFC0E, "SUZY_COLLADRL" },
    { 0xFC0F, "SUZY_COLLADRH" },
    { 0xFC10, "SUZY_SCBNEXTL" },
    { 0xFC11, "SUZY_SCBNEXTH" },
    { 0xFC12, "SUZY_SPRDLINEL" },
    { 0xFC13, "SUZY_SPRDLINEH" },
    { 0xFC14, "SUZY_HPOSSTRTL" },
    { 0xFC15, "SUZY_HPOSSTRTH" },
    { 0xFC16, "SUZY_VPOSSTRTL" },
    { 0xFC17, "SUZY_VPOSSTRTH" },
    { 0xFC18, "SUZY_SPRHSIZL" },
    { 0xFC19, "SUZY_SPRHSIZH" },
    { 0xFC1A, "SUZY_SPRVSIZL" },
    { 0xFC1B, "SUZY_SPRVSIZH" },
    { 0xFC1C, "SUZY_STRETCHL" },
    { 0xFC1D, "SUZY_STRETCHH" },
    { 0xFC1E, "SUZY_TILTL" },
    { 0xFC1F, "SUZY_TILTH" },
    { 0xFC20, "SUZY_SPRDOFFL" },
    { 0xFC21, "SUZY_SPRDOFFH" },
    { 0xFC22, "SUZY_SPRVPOSL" },
    { 0xFC23, "SUZY_SPRVPOSH" },
    { 0xFC24, "SUZY_COLLOFFL" },
    { 0xFC25, "SUZY_COLLOFFH" },
    { 0xFC26, "SUZY_VSIZACUML" },
    { 0xFC27, "SUZY_VSIZACUMH" },
    { 0xFC28, "SUZY_HSIZOFFL" },
    { 0xFC29, "SUZY_HSIZOFFH" },
    { 0xFC2A, "SUZY_VSIZOFFL" },
    { 0xFC2B, "SUZY_VSIZOFFH" },
    { 0xFC2C, "SUZY_SCBADRL" },
    { 0xFC2D, "SUZY_SCBADRH" },
    { 0xFC2E, "SUZY_PROCADRL" },
    { 0xFC2F, "SUZY_PROCADRH" },
    { 0xFC52, "SUZY_MATHD" },
    { 0xFC53, "SUZY_MATHC" },
    { 0xFC54, "SUZY_MATHB" },
    { 0xFC55, "SUZY_MATHA" },
    { 0xFC56, "SUZY_MATHP" },
    { 0xFC57, "SUZY_MATHN" },
    { 0xFC60, "SUZY_MATHH" },
    { 0xFC61, "SUZY_MATHG" },
    { 0xFC62, "SUZY_MATHF" },
    { 0xFC63, "SUZY_MATHE" },
    { 0xFC6C, "SUZY_MATHM" },
    { 0xFC6D, "SUZY_MATHL" },
    { 0xFC6E, "SUZY_MATHK" },
    { 0xFC6F, "SUZY_MATHJ" },
    { 0xFC80, "SUZY_SPRCTL0" },
    { 0xFC81, "SUZY_SPRCTL1" },
    { 0xFC82, "SUZY_SPRCOLL" },
    { 0xFC83, "SUZY_SPRINIT" },
    { 0xFC88, "SUZY_HREV" },
    { 0xFC89, "SUZY_SREV" },
    { 0xFC90, "SUZY_BUSEN" },
    { 0xFC91, "SUZY_SPRGO" },
    { 0xFC92, "SUZY_SPRSYS" },
    { 0xFCB0, "SUZY_JOYSTICK" },
    { 0xFCB1, "SUZY_SWITCHES" },
    { 0xFCB2, "SUZY_RCART0" },
    { 0xFCB3, "SUZY_RCART1" },
    { 0xFCC0, "SUZY_LEDS" },
    { 0xFCC2, "SUZY_PPORTSTAT" },
    { 0xFCC3, "SUZY_PPORTDATA" },
    { 0xFCC4, "SUZY_HOWIE" },
    { 0xFD00, "MIKEY_TIM0BKUP" },
    { 0xFD01, "MIKEY_TIM0CTLA" },
    { 0xFD02, "MIKEY_TIM0CNT" },
    { 0xFD03, "MIKEY_TIM0CTLB" },
    { 0xFD04, "MIKEY_TIM1BKUP" },
    { 0xFD05, "MIKEY_TIM1CTLA" },
    { 0xFD06, "MIKEY_TIM1CNT" },
    { 0xFD07, "MIKEY_TIM1CTLB" },
    { 0xFD08, "MIKEY_TIM2BKUP" },
    { 0xFD09, "MIKEY_TIM2CTLA" },
    { 0xFD0A, "MIKEY_TIM2CNT" },
    { 0xFD0B, "MIKEY_TIM2CTLB" },
    { 0xFD0C, "MIKEY_TIM3BKUP" },
    { 0xFD0D, "MIKEY_TIM3CTLA" },
    { 0xFD0E, "MIKEY_TIM3CNT" },
    { 0xFD0F, "MIKEY_TIM3CTLB" },
    { 0xFD10, "MIKEY_TIM4BKUP" },
    { 0xFD11, "MIKEY_TIM4CTLA" },
    { 0xFD12, "MIKEY_TIM4CNT" },
    { 0xFD13, "MIKEY_TIM4CTLB" },
    { 0xFD14, "MIKEY_TIM5BKUP" },
    { 0xFD15, "MIKEY_TIM5CTLA" },
    { 0xFD16, "MIKEY_TIM5CNT" },
    { 0xFD17, "MIKEY_TIM5CTLB" },
    { 0xFD18, "MIKEY_TIM6BKUP" },
    { 0xFD19, "MIKEY_TIM6CTLA" },
    { 0xFD1A, "MIKEY_TIM6CNT" },
    { 0xFD1B, "MIKEY_TIM6CTLB" },
    { 0xFD1C, "MIKEY_TIM7BKUP" },
    { 0xFD1D, "MIKEY_TIM7CTLA" },
    { 0xFD1E, "MIKEY_TIM7CNT" },
    { 0xFD1F, "MIKEY_TIM7CTLB" },
    { 0xFD20, "MIKEY_AUD0VOL" },
    { 0xFD21, "MIKEY_AUD0SHFTFB" },
    { 0xFD22, "MIKEY_AUD0OUTVAL" },
    { 0xFD23, "MIKEY_AUD0L8SHFT" },
    { 0xFD24, "MIKEY_AUD0TBACK" },
    { 0xFD25, "MIKEY_AUD0CTL" },
    { 0xFD26, "MIKEY_AUD0COUNT" },
    { 0xFD27, "MIKEY_AUD0MISC" },
    { 0xFD28, "MIKEY_AUD1VOL" },
    { 0xFD29, "MIKEY_AUD1SHFTFB" },
    { 0xFD2A, "MIKEY_AUD1OUTVAL" },
    { 0xFD2B, "MIKEY_AUD1L8SHFT" },
    { 0xFD2C, "MIKEY_AUD1TBACK" },
    { 0xFD2D, "MIKEY_AUD1CTL" },
    { 0xFD2E, "MIKEY_AUD1COUNT" },
    { 0xFD2F, "MIKEY_AUD1MISC" },
    { 0xFD30, "MIKEY_AUD2VOL" },
    { 0xFD31, "MIKEY_AUD2SHFTFB" },
    { 0xFD32, "MIKEY_AUD2OUTVAL" },
    { 0xFD33, "MIKEY_AUD2L8SHFT" },
    { 0xFD34, "MIKEY_AUD2TBACK" },
    { 0xFD35, "MIKEY_AUD2CTL" },
    { 0xFD36, "MIKEY_AUD2COUNT" },
    { 0xFD37, "MIKEY_AUD2MISC" },
    { 0xFD38, "MIKEY_AUD3VOL" },
    { 0xFD39, "MIKEY_AUD3SHFTFB" },
    { 0xFD3A, "MIKEY_AUD3OUTVAL" },
    { 0xFD3B, "MIKEY_AUD3L8SHFT" },
    { 0xFD3C, "MIKEY_AUD3TBACK" },
    { 0xFD3D, "MIKEY_AUD3CTL" },
    { 0xFD3E, "MIKEY_AUD3COUNT" },
    { 0xFD3F, "MIKEY_AUD3MISC" },
    { 0xFD40, "MIKEY_ATTEN_A" },
    { 0xFD41, "MIKEY_ATTEN_B" },
    { 0xFD42, "MIKEY_ATTEN_C" },
    { 0xFD43, "MIKEY_ATTEN_D" },
    { 0xFD44, "MIKEY_MPAN" },
    { 0xFD50, "MIKEY_MSTEREO" },
    { 0xFD80, "MIKEY_INTRST" },
    { 0xFD81, "MIKEY_INTSET" },
    { 0xFD84, "MIKEY_MAGRDY0" },
    { 0xFD85, "MIKEY_MAGRDY1" },
    { 0xFD86, "MIKEY_AUDIN" },
    { 0xFD87, "MIKEY_SYSCTL1" },
    { 0xFD88, "MIKEY_HREV" },
    { 0xFD89, "MIKEY_SREV" },
    { 0xFD8A, "MIKEY_IODIR" },
    { 0xFD8B, "MIKEY_IODAT" },
    { 0xFD8C, "MIKEY_SERCTL" },
    { 0xFD8D, "MIKEY_SERDAT" },
    { 0xFD90, "MIKEY_SDONEACK" },
    { 0xFD91, "MIKEY_CPUSLEEP" },
    { 0xFD92, "MIKEY_DISPCTL" },
    { 0xFD93, "MIKEY_PBKUP" },
    { 0xFD94, "MIKEY_DISPADR" },
    { 0xFD94, "MIKEY_DISPADRL" },
    { 0xFD95, "MIKEY_DISPADRH" },
    { 0xFD9C, "MIKEY_MTEST0" },
    { 0xFD9D, "MIKEY_MTEST1" },
    { 0xFD9E, "MIKEY_MTEST2" },
    { 0xFDA0, "MIKEY_GREEN0" },
    { 0xFDA1, "MIKEY_GREEN1" },
    { 0xFDA2, "MIKEY_GREEN2" },
    { 0xFDA3, "MIKEY_GREEN3" },
    { 0xFDA4, "MIKEY_GREEN4" },
    { 0xFDA5, "MIKEY_GREEN5" },
    { 0xFDA6, "MIKEY_GREEN6" },
    { 0xFDA7, "MIKEY_GREEN7" },
    { 0xFDA8, "MIKEY_GREEN8" },
    { 0xFDA9, "MIKEY_GREEN9" },
    { 0xFDAA, "MIKEY_GREENA" },
    { 0xFDAB, "MIKEY_GREENB" },
    { 0xFDAC, "MIKEY_GREENC" },
    { 0xFDAD, "MIKEY_GREEND" },
    { 0xFDAE, "MIKEY_GREENE" },
    { 0xFDAF, "MIKEY_GREENF" },
    { 0xFDB0, "MIKEY_BLUERED0" },
    { 0xFDB1, "MIKEY_BLUERED1" },
    { 0xFDB2, "MIKEY_BLUERED2" },
    { 0xFDB3, "MIKEY_BLUERED3" },
    { 0xFDB4, "MIKEY_BLUERED4" },
    { 0xFDB5, "MIKEY_BLUERED5" },
    { 0xFDB6, "MIKEY_BLUERED6" },
    { 0xFDB7, "MIKEY_BLUERED7" },
    { 0xFDB8, "MIKEY_BLUERED8" },
    { 0xFDB9, "MIKEY_BLUERED9" },
    { 0xFDBA, "MIKEY_BLUEREDA" },
    { 0xFDBB, "MIKEY_BLUEREDB" },
    { 0xFDBC, "MIKEY_BLUEREDC" },
    { 0xFDBD, "MIKEY_BLUEREDD" },
    { 0xFDBE, "MIKEY_BLUEREDE" },
    { 0xFDBF, "MIKEY_BLUEREDF" },
    { 0xFFF9, "MAPCTL" }
};

#endif /* GUI_DEBUG_CONSTANTS_H */
