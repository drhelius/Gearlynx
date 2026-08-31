import UIKit

final class GameControlsView: UIView {
    var onButtonChanged: ((GearlynxButton, Bool) -> Void)? {
        didSet {
            option1.onButtonChanged = onButtonChanged
            option2.onButtonChanged = onButtonChanged
            actionA.onButtonChanged = onButtonChanged
            actionB.onButtonChanged = onButtonChanged
            pause.onButtonChanged = onButtonChanged
        }
    }

    var hapticsEnabled = true {
        didSet {
            dPad.hapticsEnabled = hapticsEnabled
            option1.hapticsEnabled = hapticsEnabled
            option2.hapticsEnabled = hapticsEnabled
            actionA.hapticsEnabled = hapticsEnabled
            actionB.hapticsEnabled = hapticsEnabled
            pause.hapticsEnabled = hapticsEnabled
        }
    }

    let dPad = DirectionPadView()
    let option1 = GameControlButton(title: "O1", button: .option1, shape: .circle)
    let option2 = GameControlButton(title: "O2", button: .option2, shape: .circle)
    let actionA = GameControlButton(title: "A", button: .A, shape: .circle)
    let actionB = GameControlButton(title: "B", button: .B, shape: .circle)
    let pause = GameControlButton(title: "PAUSE", button: .pause, shape: .capsule)

    private let actionGuide = UILayoutGuide()
    private var portraitConstraints = [NSLayoutConstraint]()
    private var landscapeConstraints = [NSLayoutConstraint]()
    private var portraitBottomConstraints = [NSLayoutConstraint]()
    private var usingLandscapeConstraints = false

    override init(frame: CGRect) {
        super.init(frame: frame)
        configure()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        configure()
    }

    override func layoutSubviews() {
        let landscape = bounds.width > bounds.height
        if landscape != usingLandscapeConstraints {
            usingLandscapeConstraints = landscape
            NSLayoutConstraint.deactivate(landscape ? portraitConstraints : landscapeConstraints)
            NSLayoutConstraint.activate(landscape ? landscapeConstraints : portraitConstraints)
        }

        super.layoutSubviews()
    }

    func positionPortraitControls(after screenBottom: CGFloat) -> Bool {
        guard UIDevice.current.userInterfaceIdiom == .pad,
              bounds.height > bounds.width,
              dPad.bounds.height > 0.0,
              let primaryConstraint = portraitBottomConstraints.first else { return false }

        let minimumGap: CGFloat = 44.0
        let minimumBottomInset: CGFloat = 24.0
        let safeFrame = safeAreaLayoutGuide.layoutFrame
        let bottomInset = max(
            safeFrame.maxY - screenBottom - minimumGap - dPad.bounds.height,
            minimumBottomInset
        )
        let constant = -bottomInset
        guard abs(primaryConstraint.constant - constant) > 0.5 else { return false }

        for constraint in portraitBottomConstraints {
            constraint.constant = constant
        }
        return true
    }

    private func configure() {
        isMultipleTouchEnabled = true
        backgroundColor = .clear
        dPad.onDirectionChanged = { [weak self] direction, pressed in
            guard let self else { return }
            self.onButtonChanged?(self.emulatorButton(for: direction), pressed)
        }

        dPad.translatesAutoresizingMaskIntoConstraints = false
        option1.translatesAutoresizingMaskIntoConstraints = false
        option2.translatesAutoresizingMaskIntoConstraints = false
        actionA.translatesAutoresizingMaskIntoConstraints = false
        actionB.translatesAutoresizingMaskIntoConstraints = false
        pause.translatesAutoresizingMaskIntoConstraints = false

        addSubview(dPad)
        addSubview(option1)
        addSubview(option2)
        addSubview(actionA)
        addSubview(actionB)
        addSubview(pause)
        addLayoutGuide(actionGuide)

        let isPad = UIDevice.current.userInterfaceIdiom == .pad
        let dPadSize: CGFloat = isPad ? 176.0 : 132.0
        let actionSize: CGFloat = isPad ? 76.0 : 62.0
        let actionSpacing: CGFloat = isPad ? 14.0 : 10.0
        let primaryOffset: CGFloat = isPad ? 220.0 : 120.0
        let menuWidth: CGFloat = isPad ? 104.0 : 88.0
        let portraitBottomInset: CGFloat = isPad ? 160.0 : 24.0
        let portraitPrimaryConstraint = isPad
            ? dPad.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -portraitBottomInset)
            : dPad.centerYAnchor.constraint(equalTo: safeAreaLayoutGuide.centerYAnchor, constant: primaryOffset)
        let portraitPauseConstraint = pause.bottomAnchor.constraint(
            equalTo: safeAreaLayoutGuide.bottomAnchor,
            constant: -portraitBottomInset
        )

        if isPad {
            portraitBottomConstraints = [portraitPrimaryConstraint, portraitPauseConstraint]
        }

        NSLayoutConstraint.activate([
            dPad.widthAnchor.constraint(equalToConstant: dPadSize),
            dPad.heightAnchor.constraint(equalTo: dPad.widthAnchor),

            option2.widthAnchor.constraint(equalToConstant: actionSize),
            option2.heightAnchor.constraint(equalTo: option2.widthAnchor),

            option1.trailingAnchor.constraint(equalTo: option2.leadingAnchor, constant: -actionSpacing),
            option1.centerYAnchor.constraint(equalTo: option2.centerYAnchor),
            option1.widthAnchor.constraint(equalTo: option2.widthAnchor),
            option1.heightAnchor.constraint(equalTo: option2.heightAnchor),

            actionA.trailingAnchor.constraint(equalTo: option2.trailingAnchor),
            actionA.topAnchor.constraint(equalTo: option2.bottomAnchor, constant: actionSpacing),
            actionA.widthAnchor.constraint(equalTo: option2.widthAnchor),
            actionA.heightAnchor.constraint(equalTo: option2.heightAnchor),

            actionB.trailingAnchor.constraint(equalTo: option1.trailingAnchor),
            actionB.centerYAnchor.constraint(equalTo: actionA.centerYAnchor),
            actionB.widthAnchor.constraint(equalTo: option2.widthAnchor),
            actionB.heightAnchor.constraint(equalTo: option2.heightAnchor),

            pause.widthAnchor.constraint(equalToConstant: menuWidth),
            pause.heightAnchor.constraint(equalToConstant: 44.0),

            actionGuide.leadingAnchor.constraint(equalTo: option1.leadingAnchor),
            actionGuide.trailingAnchor.constraint(equalTo: option2.trailingAnchor)
        ])

        portraitConstraints = [
            dPad.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: 20.0),
            option2.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -20.0),
            portraitPrimaryConstraint,
            option2.centerYAnchor.constraint(equalTo: dPad.centerYAnchor, constant: -(actionSize + actionSpacing) * 0.5),
            pause.centerXAnchor.constraint(equalTo: safeAreaLayoutGuide.centerXAnchor),
            portraitPauseConstraint
        ]

        landscapeConstraints = [
            dPad.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: 8.0),
            option2.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -8.0),
            dPad.centerYAnchor.constraint(equalTo: safeAreaLayoutGuide.centerYAnchor),
            option2.centerYAnchor.constraint(equalTo: safeAreaLayoutGuide.centerYAnchor, constant: -(actionSize + actionSpacing) * 0.5),
            pause.centerXAnchor.constraint(equalTo: actionGuide.centerXAnchor),
            pause.bottomAnchor.constraint(
                equalTo: safeAreaLayoutGuide.bottomAnchor,
                constant: isPad ? -24.0 : -8.0
            )
        ]

        NSLayoutConstraint.activate(portraitConstraints)
    }

    private func emulatorButton(for direction: DirectionPadDirection) -> GearlynxButton {
        switch direction {
        case .up: return .up
        case .down: return .down
        case .left: return .left
        case .right: return .right
        }
    }
}

final class GameControlButton: UIButton {
    enum Shape {
        case circle
        case capsule
    }

    var onButtonChanged: ((GearlynxButton, Bool) -> Void)?
    var hapticsEnabled = true

    private let emulatorButton: GearlynxButton
    private let shape: Shape
    private var pressed = false
    private let feedback = UIImpactFeedbackGenerator(style: .light)

    init(title: String, button: GearlynxButton, shape: Shape) {
        self.emulatorButton = button
        self.shape = shape
        super.init(frame: .zero)

        setTitle(title, for: .normal)
        setTitleColor(.label, for: .normal)
        titleLabel?.font = shape == .circle
            ? .systemFont(ofSize: title.count > 1 ? 18.0 : 22.0, weight: .bold)
            : .systemFont(ofSize: 11.0, weight: .semibold)
        backgroundColor = UIColor.secondarySystemFill.withAlphaComponent(0.92)
        layer.borderColor = UIColor.separator.withAlphaComponent(0.65).cgColor
        layer.borderWidth = 1.0
        accessibilityLabel = title

        addTarget(self, action: #selector(press), for: [.touchDown, .touchDragEnter])
        addTarget(self, action: #selector(releaseButton), for: [.touchUpInside, .touchUpOutside, .touchCancel, .touchDragExit])
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func layoutSubviews() {
        super.layoutSubviews()
        layer.cornerRadius = shape == .circle ? bounds.width * 0.5 : bounds.height * 0.5
    }

    @objc private func press() {
        guard !pressed else { return }
        pressed = true
        if hapticsEnabled {
            feedback.prepare()
            feedback.impactOccurred(intensity: 0.55)
        }
        backgroundColor = tintColor.withAlphaComponent(0.28)
        transform = CGAffineTransform(scaleX: 0.94, y: 0.94)
        onButtonChanged?(emulatorButton, true)
    }

    @objc private func releaseButton() {
        guard pressed else { return }
        pressed = false
        backgroundColor = UIColor.secondarySystemFill.withAlphaComponent(0.92)
        transform = .identity
        onButtonChanged?(emulatorButton, false)
    }
}
