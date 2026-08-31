/*
 * Gearlynx - Atari Lynx Emulator
 * Copyright (C) 2025 Ignacio Sanchez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#import "GearlynxEmulator.h"

#import <AVFAudio/AVFAudio.h>

#include <string.h>
#include <strings.h>

#include "IOSAudioQueue.h"

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.h"
#undef MINIZ_NO_ZLIB_COMPATIBLE_NAMES

#undef MIN
#undef MAX
#include "../../../src/gearlynx.h"

bool g_mcp_stdio_mode = false;

static NSString* const GearlynxEmulatorErrorDomain = @"me.ignaciosanchez.gearlynx.emulator";

static bool IsROMArchiveEntry(const char* filename)
{
    const char* extension = strrchr(filename, '.');
    if (!extension)
        return false;

    return (strcasecmp(extension, ".lnx") == 0) ||
           (strcasecmp(extension, ".lyx") == 0) ||
           (strcasecmp(extension, ".o") == 0) ||
           (strcasecmp(extension, ".bin") == 0);
}

static NSString* ROMCRCInArchive(NSURL* url)
{
    if (!url.isFileURL)
        return nil;

    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if (!mz_zip_reader_init_file(&archive, url.fileSystemRepresentation, 0))
        return nil;

    NSString* result = nil;
    mz_uint fileCount = mz_zip_reader_get_num_files(&archive);
    for (mz_uint index = 0; index < fileCount; index++)
    {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&archive, index, &fileStat))
            break;
        if (!IsROMArchiveEntry(fileStat.m_filename))
            continue;

        size_t size = 0;
        void* data = mz_zip_reader_extract_to_heap(&archive, index, &size, 0);
        if (!data)
            break;

        mz_ulong checksum = mz_crc32(MZ_CRC32_INIT, (const unsigned char*)data, size);
        free(data);
        result = [NSString stringWithFormat:@"%08X", (unsigned int)checksum];
        break;
    }

    mz_zip_reader_end(&archive);
    return result;
}

static GLYNX_Console_Type ConsoleForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return GLYNX_CONSOLE_MODEL_I;
        case 2:
            return GLYNX_CONSOLE_MODEL_II;
        default:
            return GLYNX_CONSOLE_AUTO;
    }
}

static GLYNX_Rotation RotationForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return GLYNX_ROTATION_LEFT;
        case 2:
            return GLYNX_ROTATION_RIGHT;
        case 3:
            return GLYNX_ROTATION_DISABLED;
        case 4:
            return GLYNX_ROTATION_180;
        default:
            return GLYNX_ROTATION_AUTO;
    }
}

static GLYNX_EEPROM EEPROMForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return GLYNX_EEPROM_NONE;
        case 2:
            return GLYNX_EEPROM_93C46;
        case 3:
            return (GLYNX_EEPROM)(GLYNX_EEPROM_93C46 | GLYNX_EEPROM_8BIT);
        case 4:
            return GLYNX_EEPROM_93C56;
        case 5:
            return (GLYNX_EEPROM)(GLYNX_EEPROM_93C56 | GLYNX_EEPROM_8BIT);
        case 6:
            return GLYNX_EEPROM_93C66;
        case 7:
            return (GLYNX_EEPROM)(GLYNX_EEPROM_93C66 | GLYNX_EEPROM_8BIT);
        case 8:
            return GLYNX_EEPROM_93C76;
        case 9:
            return (GLYNX_EEPROM)(GLYNX_EEPROM_93C76 | GLYNX_EEPROM_8BIT);
        case 10:
            return GLYNX_EEPROM_93C86;
        case 11:
            return (GLYNX_EEPROM)(GLYNX_EEPROM_93C86 | GLYNX_EEPROM_8BIT);
        default:
            return GLYNX_EEPROM_NONE;
    }
}

static GLYNX_Cartridge_Hardware CartridgeHardwareForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return GLYNX_CARTRIDGE_HARDWARE_STANDARD;
        case 2:
            return GLYNX_CARTRIDGE_HARDWARE_GAME_DRIVE;
        case 3:
            return GLYNX_CARTRIDGE_HARDWARE_EL_CHEAPO_SD;
        default:
            return GLYNX_CARTRIDGE_HARDWARE_AUTO;
    }
}

static GLYNX_Keys KeyForButton(GearlynxButton button)
{
    switch (button)
    {
        case GearlynxButtonUp: return GLYNX_KEY_UP;
        case GearlynxButtonDown: return GLYNX_KEY_DOWN;
        case GearlynxButtonLeft: return GLYNX_KEY_LEFT;
        case GearlynxButtonRight: return GLYNX_KEY_RIGHT;
        case GearlynxButtonA: return GLYNX_KEY_A;
        case GearlynxButtonB: return GLYNX_KEY_B;
        case GearlynxButtonOption1: return GLYNX_KEY_OPTION1;
        case GearlynxButtonOption2: return GLYNX_KEY_OPTION2;
        case GearlynxButtonPause: return GLYNX_KEY_PAUSE;
    }

    return GLYNX_KEY_A;
}

@interface GearlynxEmulator ()
{
    GearlynxCore* m_core;
    u16* m_frameBuffer;
    s16* m_audioBuffer;
    IOSAudioQueue m_audioQueue;
    uint32_t m_pressedButtons;
    BOOL m_loaded;
    BOOL m_muted;
    BOOL m_legacySpriteRenderer;
    NSInteger m_console;
    NSInteger m_eeprom;
    NSInteger m_cartridgeHardware;
    NSInteger m_rotation;
    NSInteger m_lowpassCutoff;
    NSInteger m_saveStateSlot;
    NSInteger m_frameWidth;
    NSInteger m_frameHeight;
    double m_framesPerSecond;
    NSURL* m_firmwareDirectory;
    AVAudioEngine* m_audioEngine;
    AVAudioSourceNode* m_audioSourceNode;
}

- (void)applyConfiguration;
- (GLYNX_Bios_State)loadFirmware;
- (void)updateRuntimeInfo;
- (void)configureAudio;
- (void)audioEngineConfigurationChanged:(NSNotification*)notification;
- (void)clearAudio;
- (void)enqueueAudioSamples:(const s16*)samples count:(int)count;
- (OSStatus)renderAudioFrames:(AVAudioFrameCount)frameCount outputData:(AudioBufferList*)outputData silence:(BOOL*)isSilence;

@end

@implementation GearlynxEmulator

+ (NSString*)romCRCInArchiveAtURL:(NSURL*)url
{
    return ROMCRCInArchive(url);
}

- (instancetype)init
{
    self = [super init];

    if (self)
    {
        m_core = new GearlynxCore();
        m_core->Init(GLYNX_PIXEL_RGB565);

        m_frameBuffer = new u16[GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_WIDTH]();
        m_audioBuffer = new s16[GLYNX_AUDIO_BUFFER_SIZE]();
        m_audioQueue.Configure(GLYNX_AUDIO_QUEUE_SIZE, 3);
        m_pressedButtons = 0;
        m_loaded = NO;
        m_muted = NO;
        m_legacySpriteRenderer = NO;
        m_console = 0;
        m_eeprom = 0;
        m_cartridgeHardware = 0;
        m_rotation = 0;
        m_lowpassCutoff = 3500;
        m_saveStateSlot = 1;
        m_frameWidth = GLYNX_SCREEN_WIDTH;
        m_frameHeight = GLYNX_SCREEN_HEIGHT;
        m_framesPerSecond = 60.0;

        [self applyConfiguration];
        [self configureAudio];
    }

    return self;
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
    [self stopAudio];

    if (m_loaded)
    {
        m_core->SaveRam();
    }

    SafeDeleteArray(m_audioBuffer);
    SafeDeleteArray(m_frameBuffer);
    SafeDelete(m_core);
}

- (void)configureWithConsole:(NSInteger)console
                       eeprom:(NSInteger)eeprom
            cartridgeHardware:(NSInteger)cartridgeHardware
        legacySpriteRenderer:(BOOL)legacySpriteRenderer
                     rotation:(NSInteger)rotation
               lowpassCutoff:(NSInteger)lowpassCutoff
               saveStateSlot:(NSInteger)saveStateSlot
           firmwareDirectory:(NSURL*)firmwareDirectory
{
    m_console = console;
    m_eeprom = eeprom;
    m_cartridgeHardware = cartridgeHardware;
    m_legacySpriteRenderer = legacySpriteRenderer;
    m_rotation = rotation;
    m_lowpassCutoff = lowpassCutoff;
    m_firmwareDirectory = firmwareDirectory;

    if (saveStateSlot < 1)
    {
        m_saveStateSlot = 1;
    }
    else if (saveStateSlot > 5)
    {
        m_saveStateSlot = 5;
    }
    else
    {
        m_saveStateSlot = saveStateSlot;
    }

    [self applyConfiguration];
}

- (void)applyConfiguration
{
    m_core->GetMedia()->ForceConsoleType(ConsoleForOption(m_console));
    m_core->GetMedia()->ForceRotation(RotationForOption(m_rotation));

    if (m_eeprom == 0)
    {
        m_core->GetMedia()->AutoDetectEEPROM();
    }
    else
    {
        m_core->GetMedia()->ForceEEPROM(EEPROMForOption(m_eeprom));
    }

    if (m_cartridgeHardware == 0)
    {
        m_core->GetMedia()->AutoDetectCartridgeHardware();
    }
    else
    {
        m_core->GetMedia()->ForceCartridgeHardware(CartridgeHardwareForOption(m_cartridgeHardware));
    }

    m_core->GetSuzy()->SetFastSpriteRendering(m_legacySpriteRenderer);
    m_core->GetAudio()->SetLowpassCutoff((float)m_lowpassCutoff);
}

- (GLYNX_Bios_State)loadFirmware
{
    if (!m_firmwareDirectory)
    {
        return BIOS_LOAD_FILE_ERROR;
    }

    NSURL* biosURL = [m_firmwareDirectory URLByAppendingPathComponent:@"lynxboot.img"];
    if (![NSFileManager.defaultManager fileExistsAtPath:biosURL.path])
    {
        m_core->UnloadBios();
        return BIOS_LOAD_FILE_ERROR;
    }

    GLYNX_Bios_State state = m_core->LoadBios(biosURL.fileSystemRepresentation);
    return m_core->GetMedia()->IsBiosLoaded() ? BIOS_LOAD_OK : state;
}

- (void)updateRuntimeInfo
{
    GLYNX_Runtime_Info runtimeInfo;

    if (m_core->GetRuntimeInfo(runtimeInfo))
    {
        m_frameWidth = runtimeInfo.screen_width;
        m_frameHeight = runtimeInfo.screen_height;

        if (runtimeInfo.frame_time > 0.0f)
        {
            m_framesPerSecond = 1000.0 / runtimeInfo.frame_time;
        }
    }
}

- (BOOL)loadROMAtURL:(NSURL*)url error:(NSError**)error
{
    if (!url.isFileURL)
    {
        if (error)
        {
            *error = [NSError errorWithDomain:GearlynxEmulatorErrorDomain
                                         code:1
                                     userInfo:@{NSLocalizedDescriptionKey: @"The selected item is not a local ROM file."}];
        }

        return NO;
    }

    if (m_loaded)
    {
        m_core->SaveRam();
    }

    [self releaseAllButtons];
    [self clearAudio];

    [self applyConfiguration];
    GLYNX_Bios_State biosState = [self loadFirmware];

    if (biosState != BIOS_LOAD_OK)
    {
        m_loaded = NO;

        if (error)
        {
            NSString* message = @"Atari Lynx BIOS lynxboot.img is not installed.";

            if (biosState == BIOS_LOAD_INVALID_SIZE)
            {
                message = @"lynxboot.img must be exactly 512 bytes.";
            }
            *error = [NSError errorWithDomain:GearlynxEmulatorErrorDomain
                                         code:2
                                     userInfo:@{NSLocalizedDescriptionKey: message}];
        }

        return NO;
    }

    BOOL loaded = m_core->LoadROM(url.fileSystemRepresentation);

    if (!loaded)
    {
        m_loaded = NO;

        if (error)
        {
            *error = [NSError errorWithDomain:GearlynxEmulatorErrorDomain
                                         code:3
                                     userInfo:@{NSLocalizedDescriptionKey: @"Gearlynx could not load this ROM."}];
        }

        return NO;
    }

    m_core->LoadRam();
    [self applyConfiguration];
    m_core->Pause(false);
    m_loaded = YES;
    memset(m_frameBuffer, 0, GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_WIDTH * sizeof(u16));
    [self updateRuntimeInfo];

    return YES;
}

- (void)runFrame
{
    if (!m_loaded || m_core->IsPaused())
    {
        return;
    }

    int sampleCount = 0;
    m_core->RunToVBlank(reinterpret_cast<u8*>(m_frameBuffer), m_audioBuffer, &sampleCount);
    [self updateRuntimeInfo];

    if (!m_muted && (sampleCount > 0))
    {
        [self enqueueAudioSamples:m_audioBuffer count:sampleCount];
    }
}

- (void)setButton:(GearlynxButton)button pressed:(BOOL)pressed
{
    if (!m_loaded)
    {
        return;
    }

    uint32_t buttonMask = 1U << (uint32_t)button;
    BOOL wasPressed = (m_pressedButtons & buttonMask) != 0;

    if (pressed == wasPressed)
    {
        return;
    }

    GLYNX_Keys key = KeyForButton(button);

    if (pressed)
    {
        m_pressedButtons |= buttonMask;
        m_core->KeyPressed(key);
    }
    else
    {
        m_pressedButtons &= ~buttonMask;
        m_core->KeyReleased(key);
    }
}

- (void)releaseAllButtons
{
    if (!m_core)
    {
        return;
    }

    static const GearlynxButton buttons[] =
    {
        GearlynxButtonUp,
        GearlynxButtonDown,
        GearlynxButtonLeft,
        GearlynxButtonRight,
        GearlynxButtonA,
        GearlynxButtonB,
        GearlynxButtonOption1,
        GearlynxButtonOption2,
        GearlynxButtonPause
    };

    for (GearlynxButton button : buttons)
    {
        if ((m_pressedButtons & (1U << (uint32_t)button)) != 0)
        {
            [self setButton:button pressed:NO];
        }
    }
}

- (void)pause
{
    if (m_loaded)
    {
        [self releaseAllButtons];
        m_core->Pause(true);
    }
}

- (void)resume
{
    if (m_loaded)
    {
        m_core->Pause(false);
    }
}

- (void)reset
{
    if (!m_loaded)
    {
        return;
    }

    [self releaseAllButtons];
    m_core->SaveRam();
    [self applyConfiguration];
    m_core->ResetROM(true);
    [self applyConfiguration];
    [self updateRuntimeInfo];
    [self clearAudio];
}

- (void)saveRAM
{
    if (m_loaded)
    {
        m_core->SaveRam();
    }
}

- (void)saveState
{
    if (m_loaded)
    {
        m_core->SaveState(NULL, (int)m_saveStateSlot);
    }
}

- (void)loadState
{
    if (m_loaded)
    {
        [self releaseAllButtons];
        m_core->LoadState(NULL, (int)m_saveStateSlot);
        [self clearAudio];
    }
}

- (BOOL)isLoaded
{
    return m_loaded;
}

- (BOOL)isPaused
{
    return !m_loaded || m_core->IsPaused();
}

- (BOOL)isMuted
{
    return m_muted;
}

- (void)setMuted:(BOOL)muted
{
    m_muted = muted;

    if (muted)
    {
        [self clearAudio];
    }
}

- (const uint16_t*)frameBuffer
{
    return m_frameBuffer;
}

- (NSInteger)frameWidth
{
    return m_frameWidth;
}

- (NSInteger)frameHeight
{
    return m_frameHeight;
}

- (double)framesPerSecond
{
    return m_framesPerSecond;
}

- (void)configureAudio
{
    m_audioEngine = [[AVAudioEngine alloc] init];
    AVAudioFormat* format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:GLYNX_AUDIO_SAMPLE_RATE channels:2];
    __weak GearlynxEmulator* weakSelf = self;

    m_audioSourceNode = [[AVAudioSourceNode alloc] initWithFormat:format
                                                     renderBlock:^OSStatus(BOOL* isSilence,
                                                                         const AudioTimeStamp* timestamp,
                                                                         AVAudioFrameCount frameCount,
                                                                         AudioBufferList* outputData)
    {
        UNUSED(timestamp);
        GearlynxEmulator* strongSelf = weakSelf;

        if (!strongSelf)
        {
            *isSilence = YES;

            for (UInt32 bufferIndex = 0; bufferIndex < outputData->mNumberBuffers; ++bufferIndex)
            {
                AudioBuffer* buffer = &outputData->mBuffers[bufferIndex];
                memset(buffer->mData, 0, buffer->mDataByteSize);
            }

            return noErr;
        }

        return [strongSelf renderAudioFrames:frameCount outputData:outputData silence:isSilence];
    }];

    [m_audioEngine attachNode:m_audioSourceNode];
    [m_audioEngine connect:m_audioSourceNode to:m_audioEngine.mainMixerNode format:format];
    [m_audioEngine prepare];

    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(audioEngineConfigurationChanged:)
                                               name:AVAudioEngineConfigurationChangeNotification
                                             object:m_audioEngine];
}

- (void)startAudio
{
    if (m_audioEngine.isRunning)
    {
        return;
    }

    AVAudioSession* session = AVAudioSession.sharedInstance;
    NSError* error = nil;
    [session setCategory:AVAudioSessionCategoryAmbient
                    mode:AVAudioSessionModeDefault
                 options:AVAudioSessionCategoryOptionMixWithOthers
                   error:&error];

    if (!error)
    {
        NSError* preferenceError = nil;
        [session setPreferredSampleRate:GLYNX_AUDIO_SAMPLE_RATE error:&preferenceError];
        preferenceError = nil;
        [session setPreferredIOBufferDuration:512.0 / GLYNX_AUDIO_SAMPLE_RATE error:&preferenceError];
        [session setActive:YES error:&error];
    }

    [self clearAudio];

    if (!error)
    {
        [m_audioEngine startAndReturnError:&error];
    }

    if (error)
    {
        NSLog(@"Unable to start Gearlynx audio: %@", error.localizedDescription);
    }
}

- (void)stopAudio
{
    if (m_audioEngine.isRunning)
    {
        [m_audioEngine pause];
    }

    [self clearAudio];

    NSError* error = nil;
    [AVAudioSession.sharedInstance setActive:NO
                                 withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                       error:&error];

    if (error)
    {
        NSLog(@"Unable to stop Gearlynx audio: %@", error.localizedDescription);
    }
}

- (void)audioEngineConfigurationChanged:(NSNotification*)notification
{
    UNUSED(notification);
    [self clearAudio];

    __weak GearlynxEmulator* weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        GearlynxEmulator* strongSelf = weakSelf;

        if (!strongSelf || !strongSelf->m_loaded ||
            strongSelf->m_core->IsPaused() || strongSelf->m_audioEngine.isRunning)
        {
            return;
        }

        [strongSelf->m_audioEngine prepare];

        NSError* error = nil;
        [strongSelf->m_audioEngine startAndReturnError:&error];

        if (error)
        {
            NSLog(@"Unable to restart Gearlynx audio: %@", error.localizedDescription);
        }
    });
}

- (void)clearAudio
{
    m_audioQueue.Reset();
}

- (void)enqueueAudioSamples:(const s16*)samples count:(int)count
{
    if (count > 0)
        m_audioQueue.Write(samples, (uint32_t)count);
}

- (OSStatus)renderAudioFrames:(AVAudioFrameCount)frameCount outputData:(AudioBufferList*)outputData silence:(BOOL*)isSilence
{
    bool audible = false;

    if (outputData->mNumberBuffers >= 2)
    {
        float* left = (float*)outputData->mBuffers[0].mData;
        float* right = (float*)outputData->mBuffers[1].mData;
        audible = m_audioQueue.Render(left, right, (uint32_t)frameCount);
    }
    else if (outputData->mNumberBuffers == 1)
    {
        float* output = (float*)outputData->mBuffers[0].mData;
        audible = m_audioQueue.RenderInterleaved(output, (uint32_t)frameCount);
    }

    *isSilence = !audible;
    return noErr;
}

@end
