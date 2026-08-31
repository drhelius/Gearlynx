/*
 * Gearlynx - Atari Lynx Emulator
 * Copyright (C) 2025 Ignacio Sanchez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, GearlynxButton)
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

@interface GearlynxEmulator : NSObject

@property (nonatomic, readonly, getter=isLoaded) BOOL loaded;
@property (nonatomic, readonly, getter=isPaused) BOOL paused;
@property (nonatomic, getter=isMuted) BOOL muted;
@property (nonatomic, readonly) const uint16_t* frameBuffer;
@property (nonatomic, readonly) NSInteger frameWidth;
@property (nonatomic, readonly) NSInteger frameHeight;
@property (nonatomic, readonly) double framesPerSecond;

+ (nullable NSString*)romCRCInArchiveAtURL:(NSURL*)url NS_SWIFT_NAME(romCRC(inArchiveAt:));
- (void)configureWithConsole:(NSInteger)console
                       eeprom:(NSInteger)eeprom
            cartridgeHardware:(NSInteger)cartridgeHardware
        legacySpriteRenderer:(BOOL)legacySpriteRenderer
                     rotation:(NSInteger)rotation
               lowpassCutoff:(NSInteger)lowpassCutoff
               saveStateSlot:(NSInteger)saveStateSlot
           firmwareDirectory:(NSURL*)firmwareDirectory
    NS_SWIFT_NAME(configure(console:eeprom:cartridgeHardware:legacySpriteRenderer:rotation:lowpassCutoff:saveStateSlot:firmwareDirectory:));
- (BOOL)loadROMAtURL:(NSURL*)url error:(NSError* _Nullable* _Nullable)error NS_SWIFT_NAME(loadROM(at:));
- (void)runFrame;
- (void)setButton:(GearlynxButton)button pressed:(BOOL)pressed;
- (void)releaseAllButtons;
- (void)pause;
- (void)resume;
- (void)reset;
- (void)saveRAM;
- (void)saveState;
- (void)loadState;
- (void)startAudio;
- (void)stopAudio;

@end

NS_ASSUME_NONNULL_END
