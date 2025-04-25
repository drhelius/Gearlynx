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
    u32 file_size;
    u32 bank0_size;
    u32 bank1_size;
    u32 flags;
};

const GLYNX_Game_DB_Entry k_game_database[] =
{
    { 0x9D09BC4C, "8-Bit Slicks (World) (Aftermarket) (Unl)", 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x427D0E97, "A Bug's Trip Redux (World) (Aftermarket) (Unl)", 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x540E9BB7, "Alien vs Predator (USA) (Proto) (1993-12-17)", 262144, 0, 0, 0 },
    { 0x95A1EA09, "Alpine Games (World) (Aftermarket) (Unl)", 524288, GLYNX_DB_SIZE_C256K, 0, GLYNX_DB_FLAG_AUDIN | GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0xF6FB48FB, "A.P.B. (USA, Europe)", 262144, 0, 0, 0 },
    { 0x0483CD2A, "Awesome Golf (USA, Europe)", 262144, 0, 0, 0 },
    { 0x3943C116, "Baseball Heroes (USA, Europe)", 262144, 0, 0, 0 },
    { 0x4161BB4A, "Basketbrawl (USA, Europe)", 131072, 0, 0, 0 },
    { 0x277F82C2, "Batman Returns (USA, Europe)", 262144, 0, 0, 0 },
    { 0x779FAECE, "Battle Wheels (USA, Europe)", 131072, 0, 0, 0 },
    { 0x30FEE726, "Battlezone 2000 (USA, Europe)", 262144, 0, 0, 0 },
    { 0x143A313E, "Bill & Ted's Excellent Adventure (USA, Europe)", 262144, 0, 0, 0 },
    { 0x3CD75DF3, "Block Out (USA, Europe)", 131072, 0, 0, 0 },
    { 0xDAF587B1, "Blue Lightning (USA, Europe)", 131072, 0, 0, 0 },
    { 0xBFE36525, "Blue Lightning (USA, Europe) (Demo)", 131072, 0, 0, 0 },
    { 0x333DAECE, "Bubble Trouble (USA, Europe)", 262144, 0, 0, 0 },
    { 0xA08F0B59, "California Games (USA, Europe)", 131072, 0, 0, 0 },
    { 0x97501709, "Centipede (USA) (Proto)", 131072, 0, 0, GLYNX_DB_FLAG_ROTATE_RIGHT },
    { 0x19C5A7A5, "Checkered Flag (USA, Europe)", 262144, 0, 0, 0 },
    { 0x6A5F53ED, "Chip's Challenge (USA, Europe)", 131072, 0, 0, 0 },
    { 0xAEC474C8, "Crystal Mines II (USA, Europe)", 131072, 0, 0, 0 },
    { 0x99729395, "Daemonsgate (USA) (Proto)", 262144, 0, 0, 0 },
    { 0xB9AC1FE5, "Desert Strike - Return to the Gulf (USA, Europe)", 262144, 0, 0, 0 },
    { 0x50386CFA, "Dinolympics (USA, Europe)", 262144, 0, 0, 0 },
    { 0xD565FBB7, "Dirty Larry - Renegade Cop (USA, Europe)", 262144, 0, 0, 0 },
    { 0xFBFC0F05, "Double Dragon (USA, Europe)", 262144, 0, 0, 0 },
    { 0x33BB74C7, "Dracula the Undead (USA, Europe)", 262144, 0, 0, 0 },
    { 0xBD97116B, "Electrocop (USA, Europe)", 131072, 0, 0, 0 },
    { 0xF83397F9, "European Soccer Challenge (USA, Europe)", 131072, 0, 0, 0 },
    { 0x6BCEAA9C, "Eye of the Beholder (USA) (Proto)", 131072, 0, 0, 0 },
    { 0xAE8C70F0, "Eye of the Beholder (USA) (Proto)", 524288, 0, 0, 0 },
    { 0x9034EE27, "Fat Bobby (USA, Europe)", 262144, 0, 0, 0 },
    { 0x7E4B5945, "Fidelity Ultimate Chess Challenge, The (USA, Europe)", 131072, 0, 0, 0 },
    { 0x494CC568, "Gates of Zendocon (USA, Europe)", 131072, 0, 0, 0 },
    { 0xAC564BAA, "Gauntlet - The Third Encounter (1990) [o1]", 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_LEFT },
    { 0x7F0EC7AD, "Gauntlet - The Third Encounter (USA, Europe)", 131072, 0, 0, GLYNX_DB_FLAG_ROTATE_LEFT },
    { 0xDCD723E3, "Gauntlet - The Third Encounter (USA, Europe) (Beta) (1990-06-04)", 131072, GLYNX_DB_SIZE_C128K, 0, GLYNX_DB_FLAG_ROTATE_LEFT },
    { 0xD20A85FC, "Gordo 106 (USA, Europe)", 262144, 0, 0, 0 },
    { 0x6DF63834, "Hard Drivin' (USA, Europe)", 131072, 0, 0, 0 },
    { 0xE8B45707, "Hockey (USA, Europe)", 262144, 0, 0, 0 },
    { 0xE3041C6C, "Hydra (USA, Europe)", 262144, 0, 0, 0 },
    { 0x5CF8BBF0, "Ishido - The Way of Stones (USA, Europe)", 131072, 0, 0, 0 },
    { 0x2455B6CF, "Jimmy Connors' Tennis (USA, Europe)", 524288, 0, 0, 0 },
    { 0x5DBA792A, "Joust (USA, Europe)", 131072, 0, 0, 0 },
    { 0xA53649F1, "Klax (USA, Europe)", 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_LEFT },
    { 0x4D5D94F4, "Klax (USA, Europe) (Beta)", 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_LEFT },
    { 0xBED5BA2B, "Krazy Ace - Miniature Golf (USA, Europe)", 262144, 0, 0, 0 },
    { 0xCD1BD405, "Kung Food (USA, Europe)", 262144, 0, 0, 0 },
    { 0x39B9B8CC, "Lemmings (USA, Europe)", 262144, 0, 0, 0 },
    { 0x0271B6E9, "Lexis (USA)", 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_RIGHT },
    { 0, "Lode Runner (USA) (Proto)", 262144, 0, 0, 0 },
    { 0xB1C25EF1, "Loopz (USA) (Proto)", 262144, 0, 0, 0 },
    { 0x1091A268, "Lynx Casino (USA, Europe)", 262144, 0, 0, 0 },
    { 0x28ADA019, "Lynx II Production Test Program (USA) (v0.02) (Proto)", 262144, 0, 0, 0 },
    { 0xABA6DA3D, "Malibu Bikini Volleyball (USA, Europe)", 262144, 0, 0, 0 },
    { 0, "Malibu Bikini Volleyball (USA, Europe) (Beta)", 262144, 0, 0, 0 },
    { 0xC3FA0D4D, "Marlboro Go! (Europe) (Proto)", 262144, 0, 0, 0 },
    { 0xCA7CF30B, "MegaPak 1 (World) (Aftermarket) (Unl)", 262144, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x7DE3783A, "Ms. Pac-Man (USA, Europe)", 131072, 0, 0, 0 },
    { 0x006FD398, "NFL Football (USA, Europe)", 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_RIGHT },
    { 0xF3E3F811, "Ninja Gaiden III - The Ancient Ship of Doom (USA, Europe)", 524288, 0, 0, 0 },
    { 0x22D47D51, "Ninja Gaiden (USA, Europe)", 262144, 0, 0, 0 },
    { 0xAA50DD22, "Pac-Land (USA, Europe)", 131072, 0, 0, 0 },
    { 0x4CDFBD57, "Paperboy (USA, Europe)", 131072, 0, 0, 0 },
    { 0x14D38CA7, "Pinball Jam (USA, Europe)", 262144, 0, 0, 0 },
    { 0x2393135F, "Pit-Fighter (USA, Europe)", 524288, 0, 0, 0 },
    { 0xBB27E6F0, "Ponx (World) (Aftermarket) (Unl)", 131072, GLYNX_DB_SIZE_C256K, 0, 0 },
    { 0x99C42034, "Power Factor (USA, Europe)", 262144, 0, 0, 0 },
    { 0xB9881423, "QIX (USA, Europe)", 131072, 0, 0, 0 },
    { 0xBCD10C3A, "Raiden (USA) (v3.0) (Beta)", 262144, 0, 0, GLYNX_DB_FLAG_ROTATE_RIGHT },
    { 0x689F31A4, "Raiden (World) (Aftermarket) (Unl)", 524288, 0, 0, GLYNX_DB_FLAG_ROTATE_RIGHT },
    { 0xB10B7C8E, "Rampage (USA, Europe)", 262144, 0, 0, 0 },
    { 0x139F301D, "Rampart (USA, Europe)", 262144, 0, 0, 0 },
    { 0x6867E80C, "RoadBlasters (USA, Europe)", 262144, 0, 0, 0 },
    { 0, "Road Riot 4WD (USA) (Proto 1)", 262144, 0, 0, 0 },
    { 0, "Road Riot 4WD (USA) (Proto 2)", 262144, 0, 0, 0 },
    { 0x69959A3B, "Road Riot 4WD (USA) (Proto 3)", 262144, 0, 0, 0 },
    { 0xD1DFF2B2, "Robo-Squash (USA, Europe)", 131072, 0, 0, 0 },
    { 0x7A6049B5, "Robotron 2084 (USA, Europe)", 131072, 0, 0, 0 },
    { 0x67E5BDBA, "Rygar (USA, Europe)", 262144, 0, 0, 0 },
    { 0xBE166F3B, "Scrapyard Dog (USA, Europe)", 262144, 0, 0, 0 },
    { 0xEB78BAA3, "Shadow of the Beast (USA, Europe)", 262144, 0, 0, 0 },
    { 0x192BCD04, "Shanghai (USA, Europe)", 131072, 0, 0, 0 },
    { 0xB8879506, "Sky Raider Redux (World) (Aftermarket) (Unl)", 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x5B2308ED, "Steel Talons (USA, Europe)", 262144, 0, 0, 0 },
    { 0x8595C40B, "S.T.U.N. Runner (USA, Europe)", 262144, 0, 0, 0 },
    { 0x2DA7E2A8, "Super Asteroids, Missile Command (USA, Europe)", 131072, 0, 0, 0 },
    { 0x690CAEB0, "Super Off-Road (USA, Europe)", 262144, 0, 0, 0 },
    { 0xDFA61571, "Super Skweek (USA, Europe)", 262144, 0, 0, 0 },
    { 0x13657705, "Switchblade II (USA, Europe)", 262144, 0, 0, 0 },
    { 0xAE267E29, "Todd's Adventures in Slime World (USA, Europe)", 131072, 0, 0, 0 },
    { 0x156A4A4C, "Toki (USA, Europe)", 262144, 0, 0, 0 },
    { 0x0590A9E3, "Tournament Cyberball (USA, Europe)", 262144, 0, 0, 0 },
    { 0xA4B924D6, "Turbo Sub (USA, Europe)", 131072, 0, 0, 0 },
    { 0xB2FA93D3, "Unnamed (World) (Aftermarket) (Unl)", 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x8D56828B, "Viking Child (USA, Europe)", 262144, 0, 0, 0 },
    { 0x50B0575A, "Vikings Saga - Protect The Love (World) (Aftermarket) (Unl)", 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x18B006A9, "Vikings Saga - Save The Love (World) (Aftermarket) (Unl)", 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0xB946BA49, "Warbirds (USA, Europe)", 131072, 0, 0, 0 },
    { 0x91233794, "World Class Soccer (USA, Europe)", 262144, 0, 0, 0 },
    { 0x7F8B5EFA, "Wyvern Tales (World) (Aftermarket) (Unl)", 524288, 0, 0, GLYNX_DB_FLAG_EEPROM_93C46 },
    { 0x9BED736D, "Xenophobe (USA, Europe)", 131072, 0, 0, 0 },
    { 0x89E2A595, "Xybots (USA, Europe)", 262144, 0, 0, 0 },
    {0 ,"Zaku (USA) (Unl)", 0, 0, 0, 0 },
    {0 ,"Zaku (USA) (Unl) (Beta)", 0, 0, 0, 0 },
    { 0xCB27199D, "Zarlor Mercenary (USA, Europe)", 131072, 0, 0, 0 },

    {0, 0, 0, 0, 0, 0}
};

const uint32_t k_crc32_tab[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static u32 CalculateCRC32(u32 crc, const u8 *buf, int size)
{
    const u8 *p;
    p = buf;
    crc = crc ^ ~0U;

    while (size--)
    {
        crc = k_crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ ~0U;
}

#endif	/* GAME_DB_H */