import Foundation

enum AppConfiguration {
    static let libraryTitleLocalizationKey = "Common::Gearlynx"
    static let thumbnailBaseURL = URL(string: "https://www.drhelius.com/thumbnails/gearlynx/")!

    static func romCRC(inArchiveAt url: URL) -> String? {
        GearlynxEmulator.romCRC(inArchiveAt: url)
    }
}
