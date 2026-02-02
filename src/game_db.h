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

#ifndef GAME_DB_H
#define	GAME_DB_H

#include "common.h"

#define GLYNX_DB_FLAG_NONE          0x00
#define GLYNX_DB_FLAG_ROTATE_LEFT   0x01
#define GLYNX_DB_FLAG_ROTATE_RIGHT  0x02
#define GLYNX_DB_FLAG_AUDIN         0x04
#define GLYNX_DB_FLAG_EEPROM_93C46  0x08
#define GLYNX_DB_FLAG_NVRAM_8KB     0x10

#define GLYNX_DB_SIZE_C64K          0x100
#define GLYNX_DB_SIZE_C128K         0x200
#define GLYNX_DB_SIZE_C256K         0x400
#define GLYNX_DB_SIZE_C512K         0x800
#define GLYNX_DB_SIZE_C1024K        0x1000

#define GLYNX_DB_BIOS_CRC           0x0D973C9D

struct GLYNX_Game_DB_Entry
{
    u32 crc;
    const char* title;
    GLYNX_Console_Type console_type;
    u32 file_size;
    u32 bank0_page_size;
    u32 bank1_page_size;
    u32 flags;
};

const GLYNX_Game_DB_Entry k_game_database[] =
{
    { 0x9D09BC4C, "8-Bit Slicks (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x427D0E97, "A Bug's Trip Redux (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x540E9BB7, "Alien vs Predator (USA) (Proto) (1993-12-17)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x95A1EA09, "Alpine Games (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 524288, GLYNX_DB_SIZE_C256K, 0, GLYNX_DB_FLAG_AUDIN | GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x6BE0E860, "Alpine Games (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 524288, GLYNX_DB_SIZE_C256K, 0, GLYNX_DB_FLAG_AUDIN | GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0xF6FB48FB, "A.P.B. (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x0483CD2A, "Awesome Golf (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x3943C116, "Baseball Heroes (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x4161BB4A, "Basketbrawl (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x277F82C2, "Batman Returns (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x779FAECE, "Battle Wheels (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x30FEE726, "Battlezone 2000 (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x143A313E, "Bill & Ted's Excellent Adventure (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x3CD75DF3, "Block Out (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0xDAF587B1, "Blue Lightning (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0xBFE36525, "Blue Lightning (USA, Europe) (Demo)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x333DAECE, "Bubble Trouble (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xA08F0B59, "California Games (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x97501709, "Centipede (USA) (Proto)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, GLYNX_DB_FLAG_ROTATE_LEFT },
    { 0x19C5A7A5, "Checkered Flag (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x6A5F53ED, "Chip's Challenge (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0xAEC474C8, "Crystal Mines II (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x99729395, "Daemonsgate (USA) (Proto)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xB9AC1FE5, "Desert Strike - Return to the Gulf (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x50386CFA, "Dinolympics (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xD565FBB7, "Dirty Larry - Renegade Cop (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xFBFC0F05, "Double Dragon (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x33BB74C7, "Dracula the Undead (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xBD97116B, "Electrocop (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0xF83397F9, "European Soccer Challenge (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x6BCEAA9C, "Eye of the Beholder (USA) (Proto)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, GLYNX_DB_FLAG_NVRAM_8KB },
    { 0xAE8C70F0, "Eye of the Beholder (USA) (Proto)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, GLYNX_DB_FLAG_NVRAM_8KB },
    { 0xF1B307CB, "Eye of the Beholder (USA)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, GLYNX_DB_FLAG_NVRAM_8KB },
    { 0x9034EE27, "Fat Bobby (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x7E4B5945, "Fidelity Ultimate Chess Challenge, The (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x494CC568, "Gates of Zendocon (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0xAC564BAA, "Gauntlet - The Third Encounter (1990) [o1]", GLYNX_CONSOLE_AUTO, 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_RIGHT },
    { 0x7F0EC7AD, "Gauntlet - The Third Encounter (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, GLYNX_DB_FLAG_ROTATE_RIGHT },
    { 0xDCD723E3, "Gauntlet - The Third Encounter (USA, Europe) (Beta) (1990-06-04)", GLYNX_CONSOLE_AUTO, 131072, GLYNX_DB_SIZE_C128K, 0, GLYNX_DB_FLAG_ROTATE_RIGHT },
    { 0xD20A85FC, "Gordo 106 (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x6DF63834, "Hard Drivin' (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0xE8B45707, "Hockey (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xE3041C6C, "Hydra (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x5CF8BBF0, "Ishido - The Way of Stones (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x2455B6CF, "Jimmy Connors' Tennis (USA, Europe)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, 0 },
    { 0x5DBA792A, "Joust (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0xA53649F1, "Klax (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_RIGHT },
    { 0x4D5D94F4, "Klax (USA, Europe) (Beta)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_RIGHT },
    { 0xBED5BA2B, "Krazy Ace - Miniature Golf (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xCD1BD405, "Kung Food (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x39B9B8CC, "Lemmings (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x0271B6E9, "Lexis (USA)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_LEFT },
    { 0, "Lode Runner (USA) (Proto)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xB1C25EF1, "Loopz (USA) (Proto)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x1091A268, "Lynx Casino (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x28ADA019, "Lynx II Production Test Program (USA) (v0.02) (Proto)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xABA6DA3D, "Malibu Bikini Volleyball (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0, "Malibu Bikini Volleyball (USA, Europe) (Beta)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xC3FA0D4D, "Marlboro Go! (Europe) (Proto)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xCA7CF30B, "MegaPak 1 (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x7DE3783A, "Ms. Pac-Man (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x006FD398, "NFL Football (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_LEFT },
    { 0xF3E3F811, "Ninja Gaiden III - The Ancient Ship of Doom (USA, Europe)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, 0 },
    { 0x22D47D51, "Ninja Gaiden (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xAA50DD22, "Pac-Land (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x4CDFBD57, "Paperboy (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x14D38CA7, "Pinball Jam (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x2393135F, "Pit-Fighter (USA, Europe)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, 0 },
    { 0xBB27E6F0, "Ponx (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 131072, GLYNX_DB_SIZE_C256K, 0, 0 },
    { 0x99C42034, "Power Factor (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xB9881423, "QIX (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0xBCD10C3A, "Raiden (USA) (v3.0) (Beta)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_LEFT },
    { 0x689F31A4, "Raiden (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, GLYNX_DB_FLAG_ROTATE_LEFT },
    { 0xB10B7C8E, "Rampage (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x139F301D, "Rampart (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x6867E80C, "RoadBlasters (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0, "Road Riot 4WD (USA) (Proto 1)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0, "Road Riot 4WD (USA) (Proto 2)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x69959A3B, "Road Riot 4WD (USA) (Proto 3)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xD1DFF2B2, "Robo-Squash (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x7A6049B5, "Robotron 2084 (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x67E5BDBA, "Rygar (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xBE166F3B, "Scrapyard Dog (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xEB78BAA3, "Shadow of the Beast (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x192BCD04, "Shanghai (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0xB8879506, "Sky Raider Redux (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x5B2308ED, "Steel Talons (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x8595C40B, "S.T.U.N. Runner (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x2DA7E2A8, "Super Asteroids, Missile Command (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x690CAEB0, "Super Off-Road (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xDFA61571, "Super Skweek (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x13657705, "Switchblade II (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xAE267E29, "Todd's Adventures in Slime World (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x156A4A4C, "Toki (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x0590A9E3, "Tournament Cyberball (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0xA4B924D6, "Turbo Sub (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0xB2FA93D3, "Unnamed (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x8D56828B, "Viking Child (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x50B0575A, "Vikings Saga - Protect The Love (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x18B006A9, "Vikings Saga - Save The Love (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0xB946BA49, "Warbirds (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x91233794, "World Class Soccer (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0x7F8B5EFA, "Wyvern Tales (World) (Aftermarket) (Unl)", GLYNX_CONSOLE_AUTO, 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x9BED736D, "Xenophobe (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },
    { 0x89E2A595, "Xybots (USA, Europe)", GLYNX_CONSOLE_AUTO, 262144, 0, 0, 0 },
    { 0, "Zaku (USA) (Unl)", GLYNX_CONSOLE_AUTO, 0, 0, 0, 0 },
    { 0, "Zaku (USA) (Unl) (Beta)", GLYNX_CONSOLE_AUTO, 0, 0, 0, 0 },
    { 0xCB27199D, "Zarlor Mercenary (USA, Europe)", GLYNX_CONSOLE_AUTO, 131072, 0, 0, 0 },

    { 0, 0, GLYNX_CONSOLE_AUTO, 0, 0, 0, 0 }
};

#endif	/* GAME_DB_H */
