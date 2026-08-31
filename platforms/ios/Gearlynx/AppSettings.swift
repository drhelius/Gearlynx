import Foundation

enum GearlynxConsoleOption: Int, CaseIterable {
    case automatic
    case lynxI
    case lynxII

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .lynxI: return "Lynx I"
        case .lynxII: return "Lynx II"
        }
    }
}

enum GearlynxEEPROMOption: Int, CaseIterable {
    case automatic
    case none
    case c46_16
    case c46_8
    case c56_16
    case c56_8
    case c66_16
    case c66_8
    case c76_16
    case c76_8
    case c86_16
    case c86_8

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .none: return L10n("Settings::None")
        case .c46_16: return "93C46 · 128 B · 16-bit"
        case .c46_8: return "93C46 · 128 B · 8-bit"
        case .c56_16: return "93C56 · 256 B · 16-bit"
        case .c56_8: return "93C56 · 256 B · 8-bit"
        case .c66_16: return "93C66 · 512 B · 16-bit"
        case .c66_8: return "93C66 · 512 B · 8-bit"
        case .c76_16: return "93C76 · 1 KB · 16-bit"
        case .c76_8: return "93C76 · 1 KB · 8-bit"
        case .c86_16: return "93C86 · 2 KB · 16-bit"
        case .c86_8: return "93C86 · 2 KB · 8-bit"
        }
    }
}

enum GearlynxCartridgeHardwareOption: Int, CaseIterable {
    case automatic
    case standard
    case gameDrive
    case elCheapoSD

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .standard: return L10n("Settings::Standard")
        case .gameDrive: return "GameDrive"
        case .elCheapoSD: return "ElCheapoSD"
        }
    }
}

enum GearlynxRotationOption: Int, CaseIterable {
    case automatic
    case left
    case right
    case disabled
    case upsideDown

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .left: return L10n("Settings::Left")
        case .right: return L10n("Settings::Right")
        case .disabled: return L10n("Settings::Disabled")
        case .upsideDown: return "180°"
        }
    }
}

enum GearlynxLowpassOption: Int, CaseIterable {
    case hz500 = 500
    case hz1000 = 1000
    case hz1500 = 1500
    case hz2000 = 2000
    case hz2500 = 2500
    case hz3000 = 3000
    case hz3500 = 3500
    case hz4000 = 4000
    case hz4500 = 4500
    case hz5000 = 5000

    var title: String {
        "\(rawValue) Hz"
    }
}

enum AppSettings {
    private enum Key {
        static let audioEnabled = "settings.audioEnabled"
        static let hapticsEnabled = "settings.hapticsEnabled"
        static let smoothingEnabled = "settings.smoothingEnabled"
        static let screenSize = "settings.screenSize"
        static let console = "settings.console"
        static let eeprom = "settings.eeprom"
        static let cartridgeHardware = "settings.cartridgeHardware"
        static let legacySpriteRendererEnabled = "settings.legacySpriteRendererEnabled"
        static let rotation = "settings.rotation"
        static let lowpassCutoff = "settings.lowpassCutoff"
        static let saveStateSlot = "settings.saveStateSlot"
    }

    static func registerDefaults() {
        UserDefaults.standard.register(defaults: [
            Key.audioEnabled: true,
            Key.hapticsEnabled: true,
            Key.smoothingEnabled: false,
            Key.screenSize: ScreenSizeOption.fitToWidth.rawValue,
            Key.console: GearlynxConsoleOption.automatic.rawValue,
            Key.eeprom: GearlynxEEPROMOption.automatic.rawValue,
            Key.cartridgeHardware: GearlynxCartridgeHardwareOption.automatic.rawValue,
            Key.legacySpriteRendererEnabled: false,
            Key.rotation: GearlynxRotationOption.automatic.rawValue,
            Key.lowpassCutoff: GearlynxLowpassOption.hz3500.rawValue,
            Key.saveStateSlot: 1
        ])
    }

    static var audioEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.audioEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.audioEnabled) }
    }

    static var hapticsEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.hapticsEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.hapticsEnabled) }
    }

    static var smoothingEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.smoothingEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.smoothingEnabled) }
    }

    static var screenSize: ScreenSizeOption {
        get { ScreenSizeOption(rawValue: UserDefaults.standard.integer(forKey: Key.screenSize)) ?? .fitToWidth }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.screenSize) }
    }

    static var console: GearlynxConsoleOption {
        get { GearlynxConsoleOption(rawValue: UserDefaults.standard.integer(forKey: Key.console)) ?? .automatic }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.console) }
    }

    static var eeprom: GearlynxEEPROMOption {
        get { GearlynxEEPROMOption(rawValue: UserDefaults.standard.integer(forKey: Key.eeprom)) ?? .automatic }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.eeprom) }
    }

    static var cartridgeHardware: GearlynxCartridgeHardwareOption {
        get {
            GearlynxCartridgeHardwareOption(
                rawValue: UserDefaults.standard.integer(forKey: Key.cartridgeHardware)
            ) ?? .automatic
        }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.cartridgeHardware) }
    }

    static var legacySpriteRendererEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.legacySpriteRendererEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.legacySpriteRendererEnabled) }
    }

    static var rotation: GearlynxRotationOption {
        get { GearlynxRotationOption(rawValue: UserDefaults.standard.integer(forKey: Key.rotation)) ?? .automatic }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.rotation) }
    }

    static var lowpassCutoff: GearlynxLowpassOption {
        get { GearlynxLowpassOption(rawValue: UserDefaults.standard.integer(forKey: Key.lowpassCutoff)) ?? .hz3500 }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.lowpassCutoff) }
    }

    static var saveStateSlot: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.saveStateSlot), 1), 5) }
        set { UserDefaults.standard.set(min(max(newValue, 1), 5), forKey: Key.saveStateSlot) }
    }
}
