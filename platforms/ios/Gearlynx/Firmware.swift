import Foundation

enum Firmware: String, CaseIterable {
    case lynxBoot = "lynxboot.img"

    var title: String {
        L10n("Settings::LynxBIOS")
    }

    var expectedSize: Int {
        0x200
    }

    var validationErrorMessage: String {
        L10n("Settings::LynxBIOSInvalid")
    }
}
