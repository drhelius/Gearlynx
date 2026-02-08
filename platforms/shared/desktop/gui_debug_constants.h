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
static const ImVec4 orange =        ImVec4(1.00f, 0.55f, 0.00f, 1.0f);
static const ImVec4 dark_orange =   ImVec4(0.60f, 0.20f, 0.00f, 1.0f);
static const ImVec4 red =           ImVec4(0.98f, 0.15f, 0.45f, 1.0f);
static const ImVec4 dark_red =      ImVec4(0.30f, 0.04f, 0.16f, 1.0f);
static const ImVec4 green =         ImVec4(0.10f, 0.90f, 0.10f, 1.0f);
static const ImVec4 dim_green =     ImVec4(0.05f, 0.40f, 0.05f, 1.0f);
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
static const char* const c_orange = "{FF8C00}";
static const char* const c_dark_orange = "{993300}";
static const char* const c_red = "{FA2673}";
static const char* const c_dark_red = "{4C0A29}";
static const char* const c_green = "{19E619}";
static const char* const c_dim_green = "{0D660D}";
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
    { 0xFC00, "TMPADRL" },
    { 0xFC01, "TMPADRH" },
    { 0xFC02, "TILTACUML" },
    { 0xFC03, "TILTACUMH" },
    { 0xFC04, "HOFFL" },
    { 0xFC05, "HOFFH" },
    { 0xFC06, "VOFFL" },
    { 0xFC07, "VOFFH" },
    { 0xFC08, "VIDBASL" },
    { 0xFC09, "VIDBASH" },
    { 0xFC0A, "COLLBASL" },
    { 0xFC0B, "COLLBASH" },
    { 0xFC0C, "VIDADRL" },
    { 0xFC0D, "VIDADRH" },
    { 0xFC0E, "COLLADRL" },
    { 0xFC0F, "COLLADRH" },
    { 0xFC10, "SCBNEXTL" },
    { 0xFC11, "SCBNEXTH" },
    { 0xFC12, "SPRDLINEL" },
    { 0xFC13, "SPRDLINEH" },
    { 0xFC14, "HPOSSTRTL" },
    { 0xFC15, "HPOSSTRTH" },
    { 0xFC16, "VPOSSTRTL" },
    { 0xFC17, "VPOSSTRTH" },
    { 0xFC18, "SPRHSIZL" },
    { 0xFC19, "SPRHSIZH" },
    { 0xFC1A, "SPRVSIZL" },
    { 0xFC1B, "SPRVSIZH" },
    { 0xFC1C, "STRETCHL" },
    { 0xFC1D, "STRETCHH" },
    { 0xFC1E, "TILTL" },
    { 0xFC1F, "TILTH" },
    { 0xFC20, "SPRDOFFL" },
    { 0xFC21, "SPRDOFFH" },
    { 0xFC22, "SPRVPOSL" },
    { 0xFC23, "SPRVPOSH" },
    { 0xFC24, "COLLOFFL" },
    { 0xFC25, "COLLOFFH" },
    { 0xFC26, "VSIZACUML" },
    { 0xFC27, "VSIZACUMH" },
    { 0xFC28, "HSIZOFFL" },
    { 0xFC29, "HSIZOFFH" },
    { 0xFC2A, "VSIZOFFL" },
    { 0xFC2B, "VSIZOFFH" },
    { 0xFC2C, "SCBADRL" },
    { 0xFC2D, "SCBADRH" },
    { 0xFC2E, "PROCADRL" },
    { 0xFC2F, "PROCADRH" },
    { 0xFC52, "MATHD" },
    { 0xFC53, "MATHC" },
    { 0xFC54, "MATHB" },
    { 0xFC55, "MATHA" },
    { 0xFC56, "MATHP" },
    { 0xFC57, "MATHN" },
    { 0xFC60, "MATHH" },
    { 0xFC61, "MATHG" },
    { 0xFC62, "MATHF" },
    { 0xFC63, "MATHE" },
    { 0xFC6C, "MATHM" },
    { 0xFC6D, "MATHL" },
    { 0xFC6E, "MATHK" },
    { 0xFC6F, "MATHJ" },
    { 0xFC80, "SPRCTL0" },
    { 0xFC81, "SPRCTL1" },
    { 0xFC82, "SPRCOLL" },
    { 0xFC83, "SPRINIT" },
    { 0xFC88, "SUZYHREV" },
    { 0xFC89, "SUZYSREV" },
    { 0xFC90, "SUZYBUSEN" },
    { 0xFC91, "SPRGO" },
    { 0xFC92, "SPRSYS" },
    { 0xFCB0, "JOYSTICK" },
    { 0xFCB1, "SWITCHES" },
    { 0xFCB2, "RCART0" },
    { 0xFCB3, "RCART1" },
    { 0xFCC0, "LEDS" },
    { 0xFCC2, "PPORTSTAT" },
    { 0xFCC3, "PPORTDATA" },
    { 0xFCC4, "HOWIE" },
    { 0xFD00, "TIM0BKUP" },
    { 0xFD01, "TIM0CTLA" },
    { 0xFD02, "TIM0CNT" },
    { 0xFD03, "TIM0CTLB" },
    { 0xFD04, "TIM1BKUP" },
    { 0xFD05, "TIM1CTLA" },
    { 0xFD06, "TIM1CNT" },
    { 0xFD07, "TIM1CTLB" },
    { 0xFD08, "TIM2BKUP" },
    { 0xFD09, "TIM2CTLA" },
    { 0xFD0A, "TIM2CNT" },
    { 0xFD0B, "TIM2CTLB" },
    { 0xFD0C, "TIM3BKUP" },
    { 0xFD0D, "TIM3CTLA" },
    { 0xFD0E, "TIM3CNT" },
    { 0xFD0F, "TIM3CTLB" },
    { 0xFD10, "TIM4BKUP" },
    { 0xFD11, "TIM4CTLA" },
    { 0xFD12, "TIM4CNT" },
    { 0xFD13, "TIM4CTLB" },
    { 0xFD14, "TIM5BKUP" },
    { 0xFD15, "TIM5CTLA" },
    { 0xFD16, "TIM5CNT" },
    { 0xFD17, "TIM5CTLB" },
    { 0xFD18, "TIM6BKUP" },
    { 0xFD19, "TIM6CTLA" },
    { 0xFD1A, "TIM6CNT" },
    { 0xFD1B, "TIM6CTLB" },
    { 0xFD1C, "TIM7BKUP" },
    { 0xFD1D, "TIM7CTLA" },
    { 0xFD1E, "TIM7CNT" },
    { 0xFD1F, "TIM7CTLB" },
    { 0xFD20, "AUD0VOL" },
    { 0xFD21, "AUD0SHFTFB" },
    { 0xFD22, "AUD0OUTVAL" },
    { 0xFD23, "AUD0L8SHFT" },
    { 0xFD24, "AUD0TBACK" },
    { 0xFD25, "AUD0CTL" },
    { 0xFD26, "AUD0COUNT" },
    { 0xFD27, "AUD0MISC" },
    { 0xFD28, "AUD1VOL" },
    { 0xFD29, "AUD1SHFTFB" },
    { 0xFD2A, "AUD1OUTVAL" },
    { 0xFD2B, "AUD1L8SHFT" },
    { 0xFD2C, "AUD1TBACK" },
    { 0xFD2D, "AUD1CTL" },
    { 0xFD2E, "AUD1COUNT" },
    { 0xFD2F, "AUD1MISC" },
    { 0xFD30, "AUD2VOL" },
    { 0xFD31, "AUD2SHFTFB" },
    { 0xFD32, "AUD2OUTVAL" },
    { 0xFD33, "AUD2L8SHFT" },
    { 0xFD34, "AUD2TBACK" },
    { 0xFD35, "AUD2CTL" },
    { 0xFD36, "AUD2COUNT" },
    { 0xFD37, "AUD2MISC" },
    { 0xFD38, "AUD3VOL" },
    { 0xFD39, "AUD3SHFTFB" },
    { 0xFD3A, "AUD3OUTVAL" },
    { 0xFD3B, "AUD3L8SHFT" },
    { 0xFD3C, "AUD3TBACK" },
    { 0xFD3D, "AUD3CTL" },
    { 0xFD3E, "AUD3COUNT" },
    { 0xFD3F, "AUD3MISC" },
    { 0xFD40, "ATTEN_A" },
    { 0xFD41, "ATTEN_B" },
    { 0xFD42, "ATTEN_C" },
    { 0xFD43, "ATTEN_D" },
    { 0xFD44, "MPAN" },
    { 0xFD50, "MSTEREO" },
    { 0xFD80, "INTRST" },
    { 0xFD81, "INTSET" },
    { 0xFD84, "MAGRDY0" },
    { 0xFD85, "MAGRDY1" },
    { 0xFD86, "AUDIN" },
    { 0xFD87, "SYSCTL1" },
    { 0xFD88, "MIKEYHREV" },
    { 0xFD89, "MIKEYSREV" },
    { 0xFD8A, "IODIR" },
    { 0xFD8B, "IODAT" },
    { 0xFD8C, "SERCTL" },
    { 0xFD8D, "SERDAT" },
    { 0xFD90, "SDONEACK" },
    { 0xFD91, "CPUSLEEP" },
    { 0xFD92, "DISPCTL" },
    { 0xFD93, "PBKUP" },
    { 0xFD94, "DISPADRL" },
    { 0xFD95, "DISPADRH" },
    { 0xFD9C, "MTEST0" },
    { 0xFD9D, "MTEST1" },
    { 0xFD9E, "MTEST2" },
    { 0xFDA0, "GREEN0" },
    { 0xFDA1, "GREEN1" },
    { 0xFDA2, "GREEN2" },
    { 0xFDA3, "GREEN3" },
    { 0xFDA4, "GREEN4" },
    { 0xFDA5, "GREEN5" },
    { 0xFDA6, "GREEN6" },
    { 0xFDA7, "GREEN7" },
    { 0xFDA8, "GREEN8" },
    { 0xFDA9, "GREEN9" },
    { 0xFDAA, "GREENA" },
    { 0xFDAB, "GREENB" },
    { 0xFDAC, "GREENC" },
    { 0xFDAD, "GREEND" },
    { 0xFDAE, "GREENE" },
    { 0xFDAF, "GREENF" },
    { 0xFDB0, "BLUERED0" },
    { 0xFDB1, "BLUERED1" },
    { 0xFDB2, "BLUERED2" },
    { 0xFDB3, "BLUERED3" },
    { 0xFDB4, "BLUERED4" },
    { 0xFDB5, "BLUERED5" },
    { 0xFDB6, "BLUERED6" },
    { 0xFDB7, "BLUERED7" },
    { 0xFDB8, "BLUERED8" },
    { 0xFDB9, "BLUERED9" },
    { 0xFDBA, "BLUEREDA" },
    { 0xFDBB, "BLUEREDB" },
    { 0xFDBC, "BLUEREDC" },
    { 0xFDBD, "BLUEREDD" },
    { 0xFDBE, "BLUEREDE" },
    { 0xFDBF, "BLUEREDF" },
    { 0xFFF8, "RESERVED" },
    { 0xFFF9, "MAPCTL" }
};

#endif /* GUI_DEBUG_CONSTANTS_H */
