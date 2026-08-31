import UIKit
import Combine
import UniformTypeIdentifiers

final class SettingsViewController: UITableViewController {
    private enum Section: Int, CaseIterable {
        case gameplay
        case system
        case video
        case audio
        case input
        case firmware
        case library
        case about
    }

    private enum GameplayRow: Int, CaseIterable {
        case audio
        case saveStateSlot
    }

    private enum SystemRow: Int, CaseIterable {
        case console
        case eeprom
        case cartridgeHardware
        case legacySpriteRenderer
    }

    private enum VideoRow: Int, CaseIterable {
        case rotation
        case screenSize
        case smoothing
    }

    private enum AudioRow: Int, CaseIterable {
        case lowpassCutoff
    }

    private enum InputRow: Int, CaseIterable {
        case haptics
    }

    private enum LibraryRow: Int, CaseIterable {
        case refresh
        case importedRoms
    }

    private var dataStoreSubscriber: AnyCancellable?
    private var pendingFirmware: Firmware?

    init() {
        super.init(style: .insetGrouped)
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        title = L10n("Common::Settings")
        navigationItem.largeTitleDisplayMode = .always
        navigationController?.navigationBar.prefersLargeTitles = true

        dataStoreSubscriber = dataStore.$allRoms
            .removeDuplicates { $0.count == $1.count }
            .receive(on: RunLoop.main)
            .sink { [weak self] _ in
                self?.tableView.reloadData()
            }
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        tableView.reloadData()
    }

    override func numberOfSections(in tableView: UITableView) -> Int {
        Section.allCases.count
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        switch Section(rawValue: section) {
        case .gameplay: return GameplayRow.allCases.count
        case .system: return SystemRow.allCases.count
        case .video: return VideoRow.allCases.count
        case .audio: return AudioRow.allCases.count
        case .input: return InputRow.allCases.count
        case .firmware: return Firmware.allCases.count
        case .library: return LibraryRow.allCases.count
        case .about: return 1
        case nil: return 0
        }
    }

    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        switch Section(rawValue: section) {
        case .gameplay: return L10n("Settings::Gameplay")
        case .system: return L10n("Settings::System")
        case .video: return L10n("Settings::Video")
        case .audio: return L10n("Settings::AudioSection")
        case .input: return L10n("Settings::Input")
        case .firmware: return L10n("Settings::Firmware")
        case .library: return L10n("Settings::Library")
        case .about: return L10n("Settings::About")
        case nil: return nil
        }
    }

    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        switch Section(rawValue: section) {
        case .gameplay: return L10n("Settings::GameplayFooter")
        case .system: return L10n("Settings::SystemFooter")
        case .video: return L10n("Settings::VideoFooter")
        case .audio: return L10n("Settings::AudioFooter")
        case .input: return L10n("Settings::InputFooter")
        case .firmware: return L10n("Settings::FirmwareFooter")
        default: return nil
        }
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard let section = Section(rawValue: indexPath.section) else {
            return UITableViewCell()
        }

        switch section {
        case .gameplay: return gameplayCell(row: indexPath.row)
        case .system: return systemCell(row: indexPath.row)
        case .video: return videoCell(row: indexPath.row)
        case .audio: return audioCell()
        case .input: return inputCell()
        case .firmware: return firmwareCell()
        case .library: return libraryCell(row: indexPath.row)
        case .about: return aboutCell()
        }
    }

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        guard let section = Section(rawValue: indexPath.section) else { return }

        switch section {
        case .gameplay where GameplayRow(rawValue: indexPath.row) == .saveStateSlot:
            showOptions(
                title: L10n("Settings::SaveStateSlot"),
                options: (1...5).map { String(format: L10n("Settings::SlotFormat"), $0) },
                selectedIndex: AppSettings.saveStateSlot - 1
            ) { AppSettings.saveStateSlot = $0 + 1 }
        case .system:
            showSystemOptions(row: indexPath.row)
        case .video where VideoRow(rawValue: indexPath.row) == .rotation:
            showOptions(
                title: L10n("Settings::Rotation"),
                options: GearlynxRotationOption.allCases.map(\.title),
                selectedIndex: AppSettings.rotation.rawValue
            ) { AppSettings.rotation = GearlynxRotationOption(rawValue: $0) ?? .automatic }
        case .video where VideoRow(rawValue: indexPath.row) == .screenSize:
            showOptions(
                title: L10n("Settings::ScreenSize"),
                options: ScreenSizeOption.allCases.map(\.title),
                selectedIndex: AppSettings.screenSize.rawValue
            ) { AppSettings.screenSize = ScreenSizeOption(rawValue: $0) ?? .fitToWidth }
        case .audio:
            let options = GearlynxLowpassOption.allCases
            let selectedIndex = options.firstIndex(of: AppSettings.lowpassCutoff) ?? 0
            showOptions(
                title: L10n("Settings::LowpassCutoff"),
                options: options.map(\.title),
                selectedIndex: selectedIndex
            ) { index in
                guard options.indices.contains(index) else { return }
                AppSettings.lowpassCutoff = options[index]
            }
        case .firmware:
            importFirmware(.lynxBoot)
        case .library where LibraryRow(rawValue: indexPath.row) == .refresh:
            dataStore.updateAll()
        default:
            break
        }
    }

    private func showSystemOptions(row: Int) {
        switch SystemRow(rawValue: row) {
        case .console:
            showOptions(
                title: L10n("Settings::Console"),
                options: GearlynxConsoleOption.allCases.map(\.title),
                selectedIndex: AppSettings.console.rawValue
            ) { AppSettings.console = GearlynxConsoleOption(rawValue: $0) ?? .automatic }
        case .eeprom:
            showOptions(
                title: L10n("Settings::EEPROM"),
                options: GearlynxEEPROMOption.allCases.map(\.title),
                selectedIndex: AppSettings.eeprom.rawValue
            ) { AppSettings.eeprom = GearlynxEEPROMOption(rawValue: $0) ?? .automatic }
        case .cartridgeHardware:
            showOptions(
                title: L10n("Settings::CartridgeHardware"),
                options: GearlynxCartridgeHardwareOption.allCases.map(\.title),
                selectedIndex: AppSettings.cartridgeHardware.rawValue
            ) { AppSettings.cartridgeHardware = GearlynxCartridgeHardwareOption(rawValue: $0) ?? .automatic }
        default:
            break
        }
    }

    private func gameplayCell(row: Int) -> UITableViewCell {
        switch GameplayRow(rawValue: row) {
        case .audio:
            return toggleCell(
                title: L10n("Settings::Audio"),
                detail: L10n("Settings::AudioDetail"),
                image: "speaker.wave.2",
                isOn: AppSettings.audioEnabled,
                action: #selector(audioChanged(_:))
            )
        case .saveStateSlot:
            return optionCell(
                title: L10n("Settings::SaveStateSlot"),
                value: String(format: L10n("Settings::SlotFormat"), AppSettings.saveStateSlot),
                image: "square.stack.3d.up"
            )
        case nil:
            return UITableViewCell()
        }
    }

    private func systemCell(row: Int) -> UITableViewCell {
        switch SystemRow(rawValue: row) {
        case .console:
            return optionCell(title: L10n("Settings::Console"), value: AppSettings.console.title, image: "gamecontroller")
        case .eeprom:
            return optionCell(title: L10n("Settings::EEPROM"), value: AppSettings.eeprom.title, image: "memorychip")
        case .cartridgeHardware:
            return optionCell(
                title: L10n("Settings::CartridgeHardware"),
                value: AppSettings.cartridgeHardware.title,
                image: "externaldrive"
            )
        case .legacySpriteRenderer:
            return toggleCell(
                title: L10n("Settings::LegacySpriteRenderer"),
                detail: L10n("Settings::LegacySpriteRendererDetail"),
                image: "hare",
                isOn: AppSettings.legacySpriteRendererEnabled,
                action: #selector(legacySpriteRendererChanged(_:))
            )
        case nil:
            return UITableViewCell()
        }
    }

    private func videoCell(row: Int) -> UITableViewCell {
        switch VideoRow(rawValue: row) {
        case .rotation:
            return optionCell(title: L10n("Settings::Rotation"), value: AppSettings.rotation.title, image: "rotate.right")
        case .screenSize:
            return optionCell(
                title: L10n("Settings::ScreenSize"),
                value: AppSettings.screenSize.title,
                image: "arrow.up.left.and.arrow.down.right"
            )
        case .smoothing:
            return toggleCell(
                title: L10n("Settings::Smoothing"),
                detail: L10n("Settings::SmoothingDetail"),
                image: "square.resize",
                isOn: AppSettings.smoothingEnabled,
                action: #selector(smoothingChanged(_:))
            )
        case nil:
            return UITableViewCell()
        }
    }

    private func audioCell() -> UITableViewCell {
        optionCell(
            title: L10n("Settings::LowpassCutoff"),
            value: AppSettings.lowpassCutoff.title,
            image: "waveform"
        )
    }

    private func inputCell() -> UITableViewCell {
        toggleCell(
            title: L10n("Settings::Haptics"),
            detail: L10n("Settings::HapticsDetail"),
            image: "hand.tap",
            isOn: AppSettings.hapticsEnabled,
            action: #selector(hapticsChanged(_:))
        )
    }

    private func firmwareCell() -> UITableViewCell {
        let installed = FirmwareStore.isInstalled(.lynxBoot)
        let cell = baseCell(
            title: L10n("Settings::ImportLynxBIOS"),
            detail: installed ? L10n("Settings::Installed") : L10n("Settings::RequiredNotInstalled"),
            image: "square.and.arrow.down"
        )
        cell.textLabel?.textColor = view.tintColor
        cell.selectionStyle = .default
        cell.accessoryType = .disclosureIndicator
        return cell
    }

    private func libraryCell(row: Int) -> UITableViewCell {
        switch LibraryRow(rawValue: row) {
        case .refresh:
            let cell = baseCell(
                title: L10n("Settings::RefreshLibrary"),
                detail: L10n("Settings::RefreshLibraryDetail"),
                image: "arrow.clockwise"
            )
            cell.textLabel?.textColor = view.tintColor
            cell.selectionStyle = .default
            return cell
        case .importedRoms:
            let cell = baseCell(title: L10n("Settings::ImportedRoms"), detail: nil, image: "memorychip")
            cell.detailTextLabel?.text = String(dataStore.allRoms.count)
            return cell
        case nil:
            return UITableViewCell()
        }
    }

    private func aboutCell() -> UITableViewCell {
        let cell = baseCell(title: L10n("Settings::Version"), detail: nil, image: "info.circle")
        let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "-"
        let build = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? "-"
        cell.detailTextLabel?.text = "\(version) (\(build))"
        return cell
    }

    private func importFirmware(_ firmware: Firmware) {
        pendingFirmware = firmware
        let picker = UIDocumentPickerViewController(forOpeningContentTypes: [.data], asCopy: true)
        picker.delegate = self
        picker.allowsMultipleSelection = false
        present(picker, animated: true)
    }

    private func baseCell(title: String, detail: String?, image: String) -> UITableViewCell {
        let style: UITableViewCell.CellStyle = detail == nil ? .value1 : .subtitle
        let cell = UITableViewCell(style: style, reuseIdentifier: nil)
        cell.textLabel?.text = title
        cell.textLabel?.adjustsFontSizeToFitWidth = true
        cell.textLabel?.minimumScaleFactor = 0.78
        cell.textLabel?.allowsDefaultTighteningForTruncation = true
        cell.detailTextLabel?.text = detail
        cell.detailTextLabel?.adjustsFontSizeToFitWidth = true
        cell.detailTextLabel?.minimumScaleFactor = 0.75
        cell.imageView?.image = UIImage(systemName: image)
        cell.imageView?.tintColor = view.tintColor
        cell.selectionStyle = .none
        return cell
    }

    private func optionCell(title: String, value: String, image: String, isEnabled: Bool = true) -> UITableViewCell {
        let cell = UITableViewCell(style: .value1, reuseIdentifier: nil)
        cell.textLabel?.text = title
        cell.textLabel?.adjustsFontSizeToFitWidth = true
        cell.textLabel?.minimumScaleFactor = 0.78
        cell.textLabel?.allowsDefaultTighteningForTruncation = true
        cell.detailTextLabel?.text = value
        cell.detailTextLabel?.adjustsFontSizeToFitWidth = true
        cell.detailTextLabel?.minimumScaleFactor = 0.7
        cell.imageView?.image = UIImage(systemName: image)
        cell.accessoryType = .disclosureIndicator
        cell.selectionStyle = isEnabled ? .default : .none
        applyEnabledState(isEnabled, to: cell)
        return cell
    }

    private func toggleCell(
        title: String,
        detail: String,
        image: String,
        isOn: Bool,
        action: Selector,
        isEnabled: Bool = true
    ) -> UITableViewCell {
        let cell = UITableViewCell(style: .subtitle, reuseIdentifier: nil)
        cell.textLabel?.text = title
        cell.textLabel?.adjustsFontSizeToFitWidth = true
        cell.textLabel?.minimumScaleFactor = 0.78
        cell.textLabel?.allowsDefaultTighteningForTruncation = true
        cell.detailTextLabel?.text = detail
        cell.detailTextLabel?.adjustsFontSizeToFitWidth = true
        cell.detailTextLabel?.minimumScaleFactor = 0.75
        cell.imageView?.image = UIImage(systemName: image)
        cell.selectionStyle = .none

        let toggle = UISwitch()
        toggle.isOn = isOn
        toggle.isEnabled = isEnabled
        toggle.accessibilityLabel = title
        toggle.addTarget(self, action: action, for: .valueChanged)
        cell.accessoryView = toggle
        applyEnabledState(isEnabled, to: cell)
        return cell
    }

    private func applyEnabledState(_ isEnabled: Bool, to cell: UITableViewCell) {
        cell.isUserInteractionEnabled = isEnabled
        cell.textLabel?.textColor = isEnabled ? .label : .tertiaryLabel
        cell.detailTextLabel?.textColor = isEnabled ? .secondaryLabel : .tertiaryLabel
        cell.imageView?.tintColor = isEnabled ? view.tintColor : .tertiaryLabel
    }

    private func showOptions(
        title: String,
        options: [String],
        selectedIndex: Int,
        onSelection: @escaping (Int) -> Void
    ) {
        let controller = OptionSelectionViewController(
            title: title,
            optionTitles: options,
            selectedIndex: selectedIndex,
            onSelection: onSelection
        )
        navigationController?.pushViewController(controller, animated: true)
    }

    @objc private func audioChanged(_ sender: UISwitch) {
        AppSettings.audioEnabled = sender.isOn
    }

    @objc private func legacySpriteRendererChanged(_ sender: UISwitch) {
        AppSettings.legacySpriteRendererEnabled = sender.isOn
    }

    @objc private func smoothingChanged(_ sender: UISwitch) {
        AppSettings.smoothingEnabled = sender.isOn
    }

    @objc private func hapticsChanged(_ sender: UISwitch) {
        AppSettings.hapticsEnabled = sender.isOn
    }
}

extension SettingsViewController: UIDocumentPickerDelegate {
    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
        guard let firmware = pendingFirmware, let sourceURL = urls.first else { return }
        pendingFirmware = nil

        do {
            try FirmwareStore.importFile(at: sourceURL, as: firmware)
            tableView.reloadSections(IndexSet(integer: Section.firmware.rawValue), with: .automatic)
        } catch {
            let alert = UIAlertController(
                title: L10n("Settings::FirmwareImportFailed"),
                message: firmware.validationErrorMessage,
                preferredStyle: .alert
            )
            alert.addAction(UIAlertAction(title: L10n("Common::OK"), style: .default))
            present(alert, animated: true)
        }
    }

    func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
        pendingFirmware = nil
    }
}
