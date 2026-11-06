import UIKit
import ObjectiveC

// AwemeLiquidBar 3.0: the home feed chrome, ported from LiquidGlassLab.
//
// Everything drawn here is UIKit bar glass, the same material as the tab
// bar: navigation bar items (top side buttons, right column, double-tap
// panel) and a standalone UITabBar (top category group). Each surface is
// registered as an edge element of the feed scroll view with that edge's
// effect hidden ("登记不画"), and is pinned Dark. Douyin's own views stay
// in place, invisible, and only receive forwarded taps (Tweak.x).

// MARK: - Shared helpers

@available(iOS 26.0, *)
private enum ALB3Glass {
    /// Small controls (bar buttons) inside a bar.
    static func controls(in root: UIView) -> [UIView] {
        var result: [UIView] = []
        var queue: [UIView] = [root]
        var index = 0
        while index < queue.count && index < 300 && result.count < 8 {
            let view = queue[index]
            index += 1
            if view !== root,
               view is UIControl,
               !view.bounds.isEmpty,
               view.bounds.width < root.bounds.width * 0.9 {
                result.append(view)
                continue
            }
            queue.append(contentsOf: view.subviews)
        }
        return result
    }

    /// The tab bar's glass platter (the capsule that holds the items).
    static func findPlatter(in root: UIView) -> UIView? {
        var queue: [UIView] = [root]
        var index = 0
        while index < queue.count && index < 300 {
            let view = queue[index]
            index += 1
            if NSStringFromClass(type(of: view)).contains("TabBarPlatterView"),
               !view.bounds.isEmpty,
               !view.isHidden {
                return view
            }
            queue.append(contentsOf: view.subviews)
        }
        return nil
    }

    /// A title drawn as a template image (the tab bar tints it), drawn
    /// 1/scale larger at a matching pixel density so it stays sharp after
    /// the bar is scaled down.
    static func textGlyph(
        _ text: String,
        pointSize: CGFloat,
        layoutScale: CGFloat,
        displayScale: CGFloat
    ) -> UIImage {
        let scale = max(0.3, layoutScale)
        let font = UIFont.systemFont(ofSize: pointSize / scale, weight: .semibold)
        let attributes: [NSAttributedString.Key: Any] = [
            .font: font,
            .foregroundColor: UIColor.black,
        ]
        let string = text as NSString
        let measured = string.size(withAttributes: attributes)
        let size = CGSize(width: ceil(measured.width) + 2, height: ceil(measured.height) + 2)
        let format = UIGraphicsImageRendererFormat()
        format.opaque = false
        format.scale = max(1, displayScale * scale)
        let renderer = UIGraphicsImageRenderer(size: size, format: format)
        let image = renderer.image { _ in
            string.draw(at: CGPoint(x: 1, y: 1), withAttributes: attributes)
        }
        return image.withRenderingMode(.alwaysTemplate)
    }
}

private var alb3EdgeInteractionKey: UInt8 = 0

/// "登记不画": registers a view as an edge element of a scroll view, the
/// way a navigation bar or tab bar is registered, with that edge's visible
/// effect hidden. Registered, its bar glass resolves like the tab bar's.
@available(iOS 26.0, *)
@objc(ALB3Edge)
public final class ALB3Edge: NSObject {
    /// `edge` is a UIRectEdge raw value (top 1, left 2, bottom 4, right 8).
    /// A nil scroll view removes the registration.
    @objc(registerView:scrollView:edge:)
    public static func register(_ view: UIView, scrollView: UIScrollView?, edge: UInt) {
        let existing = objc_getAssociatedObject(view, &alb3EdgeInteractionKey) as? NSObject
        guard let scrollView else {
            if let existing, let interaction = existing as? UIInteraction {
                view.removeInteraction(interaction)
            }
            objc_setAssociatedObject(view, &alb3EdgeInteractionKey, nil, .OBJC_ASSOCIATION_RETAIN_NONATOMIC)
            return
        }
        hideEdgeEffect(of: scrollView, edge: edge)
        if let existing {
            if existing.responds(to: NSSelectorFromString("scrollView")),
               (existing.value(forKey: "scrollView") as? UIScrollView) !== scrollView {
                existing.setValue(scrollView, forKey: "scrollView")
            }
            return
        }
        guard let type = NSClassFromString("UIScrollEdgeElementContainerInteraction") as? NSObject.Type else {
            return
        }
        let object = type.init()
        guard object.responds(to: NSSelectorFromString("setScrollView:")),
              object.responds(to: NSSelectorFromString("setEdge:")),
              let interaction = object as? UIInteraction
        else {
            return
        }
        object.setValue(scrollView, forKey: "scrollView")
        object.setValue(edge, forKey: "edge")
        view.addInteraction(interaction)
        objc_setAssociatedObject(view, &alb3EdgeInteractionKey, object, .OBJC_ASSOCIATION_RETAIN_NONATOMIC)
    }

    private static func hideEdgeEffect(of scrollView: UIScrollView, edge: UInt) {
        let name: String
        switch edge {
        case 1: name = "topEdgeEffect"
        case 2: name = "leftEdgeEffect"
        case 4: name = "bottomEdgeEffect"
        case 8: name = "rightEdgeEffect"
        default: return
        }
        guard scrollView.responds(to: NSSelectorFromString(name)),
              let effect = scrollView.value(forKey: name) as? NSObject,
              effect.responds(to: NSSelectorFromString("setHidden:"))
        else {
            return
        }
        if (effect.value(forKey: "hidden") as? Bool) != true {
            effect.setValue(true, forKey: "hidden")
        }
    }
}

/// Our bars draw no background of their own: only their items' glass.
/// Douyin sets app-wide UINavigationBar appearance (a light bar), which
/// UIAppearance applies when a bar enters a window, so the transparent look
/// is set again after that.
@available(iOS 26.0, *)
private func alb3ClearBarBackground(_ bar: UINavigationBar) {
    let appearance = UINavigationBarAppearance()
    appearance.configureWithTransparentBackground()
    appearance.backgroundColor = .clear
    appearance.backgroundEffect = nil
    appearance.shadowColor = .clear
    bar.standardAppearance = appearance
    bar.compactAppearance = appearance
    bar.scrollEdgeAppearance = appearance
    bar.compactScrollEdgeAppearance = appearance
    bar.setBackgroundImage(UIImage(), for: .default)
    bar.shadowImage = UIImage()
    bar.barTintColor = nil
    bar.isTranslucent = true
    bar.backgroundColor = .clear
}

/// A navigation bar that only takes touches on its items.
@available(iOS 26.0, *)
private final class ALB3ItemOnlyNavigationBar: UINavigationBar {
    override init(frame: CGRect) {
        super.init(frame: frame)
        alb3ClearBarBackground(self)
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func didMoveToWindow() {
        super.didMoveToWindow()
        if window != nil { alb3ClearBarBackground(self) }
    }

    override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
        let hit = super.hitTest(point, with: event)
        return hit === self ? nil : hit
    }
}

/// A standalone navigation bar whose empty, full-width areas let touches
/// through; its bar buttons keep theirs.
@available(iOS 26.0, *)
private final class ALB3PassThroughNavigationBar: UINavigationBar {
    override init(frame: CGRect) {
        super.init(frame: frame)
        alb3ClearBarBackground(self)
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func didMoveToWindow() {
        super.didMoveToWindow()
        if window != nil { alb3ClearBarBackground(self) }
    }

    override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
        guard let hit = super.hitTest(point, with: event), hit !== self else {
            return nil
        }
        let frame = hit.convert(hit.bounds, to: self)
        return frame.width < bounds.width * 0.9 ? hit : nil
    }
}

// MARK: - One bar-glass button

/// The visible glass of a bar item is larger than the item's control: the
/// control (about 36 pt) sits in a 44 pt glass platter. 3.0 sized against
/// the control and drew every button about 1.22 times too large.
@available(iOS 26.0, *)
private extension ALB3Glass {
    static let nominalGlass: CGFloat = 44

    /// The visible glass around `control`, in `bar` coordinates: the item's
    /// own glass view (PlatterGlassView) when there is one, else the largest
    /// glass or platter view centred on the control and a little larger than
    /// it; otherwise the control grown to UIKit's 44 pt glass height.
    /// 3.11.10: on iOS 26.6 the largest one is the platter's animation view
    /// (48.2 pt around a 44 pt glass, room for the press), and every face was
    /// drawn 44/48.2 of its size: a 46 pt right-column glass showed 42 pt, and
    /// the avatar (a 48 pt picture on a 52 pt glass) covered its 47.5 pt glass
    /// whole ("no glass", user report; 3.11.7's square avatar measured 1.21 ×
    /// its 36 pt control on screen, 44 / 36, not 48.2 / 36). Only the faces
    /// ask for the glass view (`ownGlass`); the top chrome measures as before.
    static func glassFrame(around control: UIView, in bar: UIView, ownGlass: Bool = false) -> (frame: CGRect, source: String) {
        let controlFrame = control.convert(control.bounds, to: bar)
        var best = CGRect.null
        var source = ""
        var glassBest = CGRect.null
        var glassSource = ""
        var queue: [UIView] = [bar]
        var index = 0
        while index < queue.count && index < 400 {
            let view = queue[index]
            index += 1
            queue.append(contentsOf: view.subviews)
            if view === bar || view === control || view.isHidden || view.alpha < 0.01 { continue }
            let name = NSStringFromClass(type(of: view))
            guard view is UIVisualEffectView || name.contains("Glass") || name.contains("Platter"),
                  !name.contains("Shadow")
            else {
                continue
            }
            let frame = view.convert(view.bounds, to: bar)
            guard frame.width >= controlFrame.width + 1, frame.height >= controlFrame.height + 1,
                  frame.width <= controlFrame.width + 24, frame.height <= controlFrame.height + 24,
                  abs(frame.midX - controlFrame.midX) < 3, abs(frame.midY - controlFrame.midY) < 3
            else {
                continue
            }
            if best.isNull || frame.width * frame.height > best.width * best.height {
                best = frame
                source = name
            }
            if name.contains("PlatterGlassView"),
               glassBest.isNull || frame.width * frame.height > glassBest.width * glassBest.height {
                glassBest = frame
                glassSource = name
            }
        }
        if ownGlass, !glassBest.isNull {
            return (glassBest, glassSource)
        }
        if !best.isNull {
            return (best, source)
        }
        let outset = max(0, (nominalGlass - controlFrame.height) / 2)
        return (controlFrame.insetBy(dx: -outset, dy: -outset), "推算")
    }

    /// A transparent image: the item keeps a standard square control while
    /// artwork is drawn over the glass.
    static let placeholderImage: UIImage = {
        let format = UIGraphicsImageRendererFormat()
        format.opaque = false
        format.scale = 1
        return UIGraphicsImageRenderer(size: CGSize(width: 18, height: 18), format: format)
            .image { _ in }
            .withRenderingMode(.alwaysOriginal)
    }()
}

/// A single button drawn by a one-item navigation bar, so its glass is bar
/// glass with UIKit's own press, stretch and menu morph. UIKit keeps the
/// item at its own size; the bar is scaled so the item's visible glass is
/// exactly this view (a circle for an icon, a capsule of this height for a
/// title), and the icon or title is drawn 1/scale larger so it stays sharp.
@available(iOS 26.0, *)
@objc(ALB3BarGlassButton)
public final class ALB3BarGlassButton: UIView {
    @objc public var primaryHandler: (() -> Void)? {
        didSet { updateActions() }
    }
    @objc public var menu: UIMenu? {
        didSet { if menu !== oldValue { updateActions() } }
    }
    /// Icon point size at the view's own size.
    @objc public var pointSize: CGFloat = 20 {
        didSet { if pointSize != oldValue { invalidateContent() } }
    }
    /// Title point size at the view's own size.
    @objc public var titlePointSize: CGFloat = 12.5 {
        didSet { if titlePointSize != oldValue { invalidateContent() } }
    }
    /// Artwork (avatar, music disc): the glass ring left around it, in
    /// points at the view's own size.
    @objc public var artworkPadding: CGFloat = 3 {
        didSet { if artworkPadding != oldValue { setNeedsLayout() } }
    }
    /// Symbols drawn Regular instead of Semibold (the double-tap panel).
    @objc public var regularWeight = false {
        didSet { if regularWeight != oldValue { invalidateContent() } }
    }
    /// The icon's own opacity; the glass stays opaque.
    @objc public var iconAlpha: CGFloat = 1 {
        didSet { if abs(iconAlpha - oldValue) > 0.001 { invalidateContent() } }
    }
    /// A light disc of this colour inside the glass, `fillInset` in from its
    /// edge, under the icon (the panel's 图标添加同色浅填充).
    @objc public var fillColor: UIColor? {
        didSet { setNeedsLayout() }
    }
    @objc public var fillInset: CGFloat = 3 {
        didSet { if fillInset != oldValue { setNeedsLayout() } }
    }

    private let bar = ALB3ItemOnlyNavigationBar()
    private let item = UIBarButtonItem(image: nil, style: .plain, target: nil, action: nil)
    private let artworkView = UIImageView()
    private let fillView = UIView()
    private var symbolName: String?
    private var customImage: UIImage?
    private var artwork = false
    private var originalColors = false
    private var titleText: String?
    private var tint: UIColor = .label
    private var contentKey = ""
    private var renderScale: CGFloat = 0
    private var shrink: CGFloat = 1
    private var measuredKey = ""
    private var measuredGlass = CGRect.null
    private var measuredControlSize = CGSize.zero
    private var glassSource = ""
    private var appliedScale: CGFloat = 0
    private weak var measuredControl: UIView?

    @objc public override init(frame: CGRect) {
        super.init(frame: frame)
        backgroundColor = .clear
        clipsToBounds = false
        overrideUserInterfaceStyle = .dark
        bar.tintColor = .label
        bar.directionalLayoutMargins = .zero
        let navigationItem = UINavigationItem(title: "")
        navigationItem.leftBarButtonItem = item
        bar.setItems([navigationItem], animated: false)
        addSubview(bar)
        artworkView.isUserInteractionEnabled = false
        artworkView.contentMode = .scaleAspectFill
        artworkView.clipsToBounds = true
        artworkView.layer.cornerCurve = .continuous
        artworkView.isHidden = true
        fillView.isUserInteractionEnabled = false
        fillView.isHidden = true
        updateActions()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    /// An SF Symbol icon; `tint` nil keeps the bar's own ink (white in Dark,
    /// black when the glass turns light over a bright picture).
    @objc(setSymbolName:tint:)
    public func setSymbol(_ name: String, tint: UIColor?) {
        let tint = tint ?? .label
        if symbolName == name, customImage == nil, titleText == nil, self.tint.isEqual(tint) { return }
        symbolName = name
        customImage = nil
        artwork = false
        originalColors = false
        titleText = nil
        self.tint = tint
        invalidateContent()
    }

    /// Any image as the icon. `artwork` draws it as a full-colour circle over
    /// the glass, `artworkPadding` inside its edge (avatar, music disc);
    /// otherwise it is a template tinted like a symbol.
    @objc(setImage:artwork:tint:)
    public func setImage(_ image: UIImage?, artwork: Bool, tint: UIColor?) {
        let tint = tint ?? .label
        if symbolName == nil, titleText == nil, customImage === image, self.artwork == artwork,
           !originalColors, self.tint.isEqual(tint) { return }
        symbolName = nil
        customImage = image
        self.artwork = artwork
        originalColors = false
        titleText = nil
        self.tint = tint
        invalidateContent()
    }

    /// A full-colour icon (a user's own PNG), fitted in a `pointSize` box
    /// and never tinted. It is drawn over the glass like artwork, so a large
    /// PNG keeps its size without widening the item into a capsule.
    @objc(setOriginalImage:)
    public func setOriginalImage(_ image: UIImage?) {
        if symbolName == nil, titleText == nil, customImage === image, !artwork, originalColors { return }
        symbolName = nil
        customImage = image
        artwork = false
        originalColors = true
        titleText = nil
        tint = .label
        invalidateContent()
    }

    /// A text capsule.
    @objc(setTitle:tint:)
    public func setTitle(_ title: String, tint: UIColor?) {
        let tint = tint ?? .label
        if titleText == title, self.tint.isEqual(tint) { return }
        symbolName = nil
        customImage = nil
        artwork = false
        originalColors = false
        titleText = title
        self.tint = tint
        invalidateContent()
    }

    @objc public var accessibilityTitle: String? {
        get { item.accessibilityLabel }
        set { item.accessibilityLabel = newValue }
    }

    /// UIKit's own disabled look and behaviour.
    @objc public var enabled: Bool {
        get { item.isEnabled }
        set {
            if item.isEnabled != newValue {
                item.isEnabled = newValue
                setNeedsLayout()
            }
        }
    }

    /// Where the glass sits when it is narrower than this view (a short
    /// title): 0 centre, 1 leading edge, 2 trailing edge.
    @objc public var contentAlignment: Int = 0 {
        didSet { if contentAlignment != oldValue { setNeedsLayout() } }
    }

    /// The visible glass in this view's coordinates after the last layout.
    @objc public private(set) var contentFrame: CGRect = .zero

    /// 3.11.7: the item's control is being pressed (a right-column face:
    /// the glass button under it reads as tracking meanwhile).
    @objc public var faceTracking: Bool {
        return (liveControl() as? UIControl)?.isTracking ?? false
    }

    /// Measurements for the diagnostics text.
    @objc public var diagnosticsText: String {
        func f(_ value: CGFloat) -> String { String(format: "%.1f", Double(value)) }
        // One piece at a time (a single long + chain is slow to type-check).
        let glassWidth = measuredGlass.isNull ? 0 : measuredGlass.width
        let glassHeight = measuredGlass.isNull ? 0 : measuredGlass.height
        var text = "控件 \(f(measuredControlSize.width))×\(f(measuredControlSize.height))"
        text += " 玻璃 \(f(glassWidth))×\(f(glassHeight))"
        text += "（\(glassSource)）比例 \(f(appliedScale)) 显示 \(f(contentFrame.width))×\(f(contentFrame.height))"
        text += clipChainText
        return text
    }

    /// 3.11.8 诊断: what lies between the item's control and the bar, and
    /// which of them clips (裁) or masks (罩): where artwork could be cut.
    private var clipChainText: String {
        guard let control = liveControl() else { return "" }
        var parts: [String] = []
        var cursor: UIView? = control
        var depth = 0
        while let view = cursor, view !== bar, depth < 6 {
            let name = String(describing: type(of: view))
            let clips = view.clipsToBounds || view.layer.masksToBounds ? "裁" : ""
            let masked = view.layer.mask != nil ? "罩" : ""
            parts.append("\(name.suffix(28))\(clips)\(masked)\(Int(view.bounds.width))")
            cursor = view.superview
            depth += 1
        }
        return " 链[" + parts.joined(separator: ">") + "]"
    }

    private func updateActions() {
        if let primaryHandler {
            // Assigning an action also assigns its (empty) title and image;
            // the content is drawn again below.
            item.primaryAction = UIAction { _ in primaryHandler() }
        } else {
            item.primaryAction = nil
        }
        item.menu = menu
        invalidateContent()
    }

    private func invalidateContent() {
        contentKey = ""
        measuredKey = ""
        setNeedsLayout()
    }

    public override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
        guard isUserInteractionEnabled, !isHidden, alpha > 0.01, self.point(inside: point, with: event) else {
            return nil
        }
        let hit = super.hitTest(point, with: event)
        // The item's control is smaller than its glass (about 36 of 44 pt):
        // a touch anywhere on the visible glass is the control's, a touch
        // outside the glass (the corners of this square) is nobody's.
        if let control = liveControl() {
            if let hit, hit === control || hit.isDescendant(of: control) { return hit }
            return insideGlass(point) ? control : nil
        }
        return hit === self || hit === artworkView ? nil : hit
    }

    private func insideGlass(_ point: CGPoint) -> Bool {
        let frame = contentFrame
        guard frame.width > 0, frame.height > 0 else { return false }
        let radius = frame.height / 2
        let clampedX = min(max(point.x, frame.minX + radius), frame.maxX - radius)
        let dx = point.x - clampedX
        let dy = point.y - frame.midY
        return dx * dx + dy * dy <= radius * radius
    }

    /// The item's control, found again if UIKit rebuilt the item's view.
    private func liveControl() -> UIView? {
        if let control = measuredControl, control.isDescendant(of: bar) { return control }
        guard let control = ALB3Glass.controls(in: bar).first else { return nil }
        measuredControl = control
        return control
    }

    public override func didMoveToWindow() {
        super.didMoveToWindow()
        measuredKey = ""
        setNeedsLayout()
    }

    public override func layoutSubviews() {
        super.layoutSubviews()
        guard bounds.width > 0, bounds.height > 0 else { return }
        let isTitle = titleText != nil
        // The glass height this view asks for: its height for a title, its
        // smaller side (the circle) for an icon. Content is sized against
        // UIKit's 44 pt glass at that size, never against the measurement,
        // so a measurement can never feed back into the content size.
        let target = isTitle ? bounds.height : min(bounds.width, bounds.height)
        let reference = max(0.3, target / ALB3Glass.nominalGlass)
        bar.transform = .identity
        // (3.11.13 narrowed this bar to 92 pt and UIKit folded the right
        // column's items into a "•••" button: 3.11.14 restores 160.)
        let barWidth: CGFloat = isTitle ? max(240, bounds.width / reference + 80) : 160
        let height = max(44, bar.sizeThatFits(CGSize(width: barWidth, height: 200)).height)
        if bar.bounds.size != CGSize(width: barWidth, height: height) {
            bar.bounds = CGRect(x: 0, y: 0, width: barWidth, height: height)
            measuredKey = ""
        }
        bar.center = CGPoint(x: bounds.midX, y: bounds.midY)
        if let control = measuredControl, !control.isDescendant(of: bar) { measuredKey = "" }
        if measuredControl == nil { measuredKey = "" }
        let glass = measureGlass(reference: reference, isTitle: isTitle)
        guard !glass.isNull, glass.height > 0 else { return }
        let scale = target / glass.height
        if renderScale <= 0 || abs(scale - renderScale) > 0.01 {
            // Only the pixel density changes; the layout does not.
            renderScale = scale
            updateContent(reference: reference)
            bar.layoutIfNeeded()
        }
        appliedScale = scale
        let offset = CGPoint(x: glass.midX - bar.bounds.midX, y: glass.midY - bar.bounds.midY)
        let glassWidth = glass.width * scale
        let midX: CGFloat
        switch contentAlignment {
        case 1: midX = bounds.minX + glassWidth / 2
        case 2: midX = bounds.maxX - glassWidth / 2
        default: midX = bounds.midX
        }
        bar.transform = CGAffineTransform(scaleX: scale, y: scale)
        bar.center = CGPoint(x: midX - offset.x * scale, y: bounds.midY - offset.y * scale)
        contentFrame = CGRect(x: midX - glassWidth / 2, y: bounds.midY - target / 2,
                              width: glassWidth, height: target)
        layoutOverlay(glass: glass, scale: scale, target: target)
        layoutFill(glass: glass, scale: scale)
    }

    /// Nothing between `control` and the bar that is smaller than `glass`
    /// (bar coordinates) clips what is drawn in the control.
    private func unclipAncestors(of control: UIView, smallerThan glass: CGRect) {
        var cursor: UIView? = control
        while let view = cursor, view !== bar {
            let frame = view.convert(view.bounds, to: bar)
            if frame.width < glass.width - 0.5 || frame.height < glass.height - 0.5 {
                if view.clipsToBounds { view.clipsToBounds = false }
                if view.layer.masksToBounds { view.layer.masksToBounds = false }
            }
            cursor = view.superview
        }
    }

    private func layoutFill(glass: CGRect, scale: CGFloat) {
        guard let color = fillColor, titleText == nil, !artwork, let control = liveControl(), scale > 0
        else {
            fillView.isHidden = true
            return
        }
        if fillView.superview !== control {
            fillView.removeFromSuperview()
            control.insertSubview(fillView, at: 0)
        }
        // 3.11.14: the item's control (36 pt) clips, and the round fill, as
        // big as the glass less its inset, came out square (user report; the
        // avatar's artwork had the same cause, layoutOverlay).
        unclipAncestors(of: control, smallerThan: glass)
        let rect = bar.convert(glass, to: control).insetBy(dx: fillInset / scale, dy: fillInset / scale)
        fillView.frame = rect
        fillView.layer.cornerRadius = min(rect.width, rect.height) / 2
        fillView.layer.cornerCurve = .continuous
        fillView.backgroundColor = color
        fillView.alpha = item.isEnabled ? 1 : 0.4
        fillView.isHidden = rect.isEmpty
    }

    /// Lays the bar out unscaled with the content at its nominal size and
    /// measures the item's glass. An icon whose glass came out wider than
    /// tall (a wide glyph or PNG) is drawn a little smaller until the glass
    /// is a circle again. Cached until the content or the size changes.
    private func measureGlass(reference: CGFloat, isTitle: Bool) -> CGRect {
        let key = "\(identity)|\(Int((reference * 1000).rounded()))|\(Int(bar.bounds.width))x\(Int(bar.bounds.height))"
        if key == measuredKey, !measuredGlass.isNull {
            return measuredGlass
        }
        shrink = 1
        var result = CGRect.null
        var bestShrink: CGFloat = 1
        for _ in 0..<6 {
            updateContent(reference: reference)
            bar.setNeedsLayout()
            bar.layoutIfNeeded()
            guard let control = ALB3Glass.controls(in: bar).first,
                  control.bounds.width > 0, control.bounds.height > 0
            else {
                break
            }
            let found = ALB3Glass.glassFrame(around: control, in: bar, ownGlass: true)
            // A smaller glyph that did not narrow the glass (UIKit's own
            // minimum width) is not used: the previous, larger one stays.
            if !result.isNull && found.frame.width >= result.width - 0.25 {
                break
            }
            result = found.frame
            bestShrink = shrink
            glassSource = found.source
            measuredControlSize = control.bounds.size
            measuredControl = control
            if isTitle || artwork || originalColors || found.frame.width <= found.frame.height + 0.5 || shrink < 0.5 {
                break
            }
            shrink *= max(0.75, min(0.97, found.frame.height / found.frame.width))
        }
        if shrink != bestShrink {
            // Draw the measured state again (the last step was not kept).
            shrink = bestShrink
            updateContent(reference: reference)
            bar.setNeedsLayout()
            bar.layoutIfNeeded()
            measuredControl = ALB3Glass.controls(in: bar).first ?? measuredControl
        }
        measuredGlass = result
        // An estimate made before the bar is on screen is measured again later.
        let estimated = glassSource == "推算" && window == nil
        measuredKey = result.isNull || estimated ? "" : key
        return result
    }

    /// Avatar / music artwork and full-colour PNG icons sit over the glass
    /// inside the item's control, so they follow its press and menu morph.
    /// Artwork is a circle `artworkPadding` in from the glass edge; a PNG is
    /// fitted in a `pointSize` box at the glass centre.
    private func layoutOverlay(glass: CGRect, scale: CGFloat, target: CGFloat) {
        guard artwork || originalColors, let image = customImage, let control = liveControl(), scale > 0
        else {
            artworkView.isHidden = true
            artworkView.image = nil
            return
        }
        if artworkView.superview !== control {
            artworkView.removeFromSuperview()
            control.addSubview(artworkView)
        }
        // 3.11.8: the item's control (about 36 pt) clips on iOS 26.6, and
        // artwork as big as the 48 pt glass came out as a small rounded
        // square (user report): nothing between the artwork and the bar that
        // is smaller than the glass clips it.
        unclipAncestors(of: control, smallerThan: glass)
        let rect = bar.convert(glass, to: control)
        let side: CGFloat
        if artwork {
            side = max(1, (target - 2 * max(0, artworkPadding)) / scale)
            artworkView.contentMode = .scaleAspectFill
            artworkView.clipsToBounds = true
            artworkView.layer.cornerRadius = side / 2
        } else {
            side = max(1, pointSize / scale)
            artworkView.contentMode = .scaleAspectFit
            artworkView.clipsToBounds = false
            artworkView.layer.cornerRadius = 0
        }
        artworkView.bounds = CGRect(x: 0, y: 0, width: side, height: side)
        artworkView.center = CGPoint(x: rect.midX, y: rect.midY)
        artworkView.alpha = (artwork ? 1 : iconAlpha) * (item.isEnabled ? 1 : 0.4)
        if artworkView.image !== image { artworkView.image = image }
        artworkView.isHidden = false
    }

    private var identity: String {
        "\(symbolName ?? "")|\(titleText ?? "")|\(customImage.map { ObjectIdentifier($0).hashValue } ?? 0)|\(artwork)|\(originalColors)|\(tint.description)|\(Int(pointSize * 10))|\(Int(titlePointSize * 10))|\(regularWeight)|\(Int(iconAlpha * 100))"
    }

    private func updateContent(reference: CGFloat) {
        let scale = max(0.3, reference)
        let render = max(0.3, renderScale > 0 ? renderScale : reference)
        let displayScale = traitCollection.displayScale > 0 ? traitCollection.displayScale : 3
        let key = "\(identity)|\(Int(scale * 1000))|\(Int(render * 1000))|\(Int(shrink * 1000))"
        guard key != contentKey else { return }
        contentKey = key
        let colored = !tint.isEqual(UIColor.label)
        if let titleText {
            item.image = nil
            item.title = titleText
            var attributes: [NSAttributedString.Key: Any] = [
                .font: UIFont.systemFont(ofSize: titlePointSize / scale, weight: .medium),
            ]
            if colored {
                attributes[.foregroundColor] = tint
            }
            item.setTitleTextAttributes(attributes, for: .normal)
            item.setTitleTextAttributes(attributes, for: .highlighted)
            return
        }
        item.title = nil
        var image: UIImage?
        if artwork || originalColors {
            image = ALB3Glass.placeholderImage
        } else if let symbolName {
            let configuration = UIImage.SymbolConfiguration(
                pointSize: pointSize / scale * shrink,
                weight: regularWeight ? .regular : .semibold
            )
            image = UIImage(systemName: symbolName, withConfiguration: configuration)
                ?? UIImage(named: symbolName)
        } else if let customImage, customImage.isSymbolImage, !originalColors {
            // 3.11.11: an SF Symbol at its point size, as the glass button
            // drew it (its preferredSymbolConfigurationForImage). 3.11.7-3.11.10
            // fitted the symbol's whole image, padding included, into a box of
            // that size: the glyph came out about 70 % of the column's.
            let configuration = UIImage.SymbolConfiguration(
                pointSize: pointSize / scale * shrink,
                weight: regularWeight ? .regular : .medium,
                scale: .medium
            )
            image = customImage.withConfiguration(configuration).withRenderingMode(.alwaysTemplate)
        } else if let customImage {
            let side = pointSize / scale * shrink
            let source = customImage.size
            let fit = source.width > 0 && source.height > 0 ? min(side / source.width, side / source.height) : 1
            let size = CGSize(width: max(1, source.width * fit), height: max(1, source.height * fit))
            let format = UIGraphicsImageRendererFormat()
            format.opaque = false
            format.scale = max(1, displayScale * render)
            image = UIGraphicsImageRenderer(size: size, format: format).image { _ in
                customImage.draw(in: CGRect(origin: .zero, size: size))
            }.withRenderingMode(originalColors ? .alwaysOriginal : .alwaysTemplate)
        }
        if let current = image, colored, !artwork, !originalColors {
            image = current.withTintColor(tint, renderingMode: .alwaysOriginal)
        }
        if let current = image, iconAlpha < 0.999, !artwork, !originalColors {
            // The opacity goes into the pixels; a template keeps its tint.
            let format = UIGraphicsImageRendererFormat()
            format.opaque = false
            format.scale = max(1, displayScale * render)
            let alpha = max(0.05, iconAlpha)
            image = UIGraphicsImageRenderer(size: current.size, format: format).image { _ in
                current.draw(at: .zero, blendMode: .normal, alpha: alpha)
            }.withRenderingMode(colored ? .alwaysOriginal : .alwaysTemplate)
        }
        item.image = image
    }
}

// MARK: - Top category group (a standalone UITabBar at its native size)

@available(iOS 26.0, *)
private final class ALB3CategoryGroup: UIView, UITabBarDelegate, UIGestureRecognizerDelegate {
    var onSelect: ((Int) -> Void)?

    var groupScale: CGFloat = 0.75 {
        didSet {
            if abs(groupScale - oldValue) > 0.001 {
                glyphsDirty = true
                setNeedsLayout()
            }
        }
    }

    var slotWidth: CGFloat = 76 {
        didSet {
            if abs(slotWidth - oldValue) > 0.01 {
                fittedKey = ""
                setNeedsLayout()
            }
        }
    }

    private let bar = UITabBar()
    private var titles: [String] = []
    private(set) var selected = 0
    private var visibleCount = 2
    private var windowStart = 0
    private var keepSingleSelection = true
    private var glyphsDirty = true
    private var fittedKey = ""
    private var platterRect: CGRect = .zero
    private var platterFound = false
    private var refitPasses = 0

    override init(frame: CGRect) {
        super.init(frame: frame)
        backgroundColor = .clear
        clipsToBounds = false
        bar.tintColor = .label
        bar.unselectedItemTintColor = .secondaryLabel
        bar.delegate = self
        addSubview(bar)

        let left = UISwipeGestureRecognizer(target: self, action: #selector(swipedLeft))
        left.direction = .left
        left.delegate = self
        let right = UISwipeGestureRecognizer(target: self, action: #selector(swipedRight))
        right.direction = .right
        right.delegate = self
        addGestureRecognizer(left)
        addGestureRecognizer(right)
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    func configure(titles: [String], selected: Int, visibleCount: Int, keepSingleSelection: Bool) {
        self.titles = titles
        self.selected = min(max(0, selected), max(0, titles.count - 1))
        self.visibleCount = min(max(1, visibleCount), max(1, titles.count))
        self.keepSingleSelection = keepSingleSelection
        glyphsDirty = true
        rebuildItems()
    }

    func showCategory(_ index: Int) {
        guard index >= 0, index < titles.count, index != selected || windowNeedsMove(for: index) else {
            return
        }
        selected = index
        rebuildItems()
    }

    private func windowNeedsMove(for index: Int) -> Bool {
        index < windowStart || index >= windowStart + visibleCount
    }

    private var currentItems: [UITabBarItem] {
        bar.items ?? []
    }

    private func rebuildItems() {
        guard !titles.isEmpty else {
            if !currentItems.isEmpty {
                bar.setItems([], animated: false)
                fittedKey = ""
            }
            return
        }
        let count = visibleCount
        if selected < windowStart {
            windowStart = selected
        } else if selected >= windowStart + count {
            windowStart = selected - count + 1
        }
        windowStart = min(max(0, windowStart), titles.count - count)

        if currentItems.count != count {
            var items: [UITabBarItem] = []
            for local in 0..<count {
                let item = UITabBarItem(title: nil, image: nil, selectedImage: nil)
                item.tag = windowStart + local
                items.append(item)
            }
            bar.setItems(items, animated: false)
            fittedKey = ""
        }
        glyphsDirty = true
        updateGlyphs()
        applySelection()
        setNeedsLayout()
        superview?.setNeedsLayout()
    }

    private func updateGlyphs() {
        guard glyphsDirty else { return }
        let displayScale = traitCollection.displayScale > 0 ? traitCollection.displayScale : 3
        for (local, item) in currentItems.enumerated() {
            let index = windowStart + local
            guard index < titles.count else { continue }
            let glyph = ALB3Glass.textGlyph(
                titles[index],
                pointSize: 15,
                layoutScale: groupScale,
                displayScale: displayScale
            )
            item.image = glyph
            item.selectedImage = glyph
            item.tag = index
            item.accessibilityLabel = titles[index]
        }
        glyphsDirty = false
    }

    private func applySelection() {
        let items = currentItems
        let local = selected - windowStart
        guard local >= 0, local < items.count else { return }
        if items.count == 1 && !keepSingleSelection {
            bar.selectedItem = nil
        } else {
            bar.selectedItem = items[local]
        }
    }

    /// The platter's height at the bar's native size (before groupScale).
    var nativePlatterHeight: CGFloat {
        platterRect.isEmpty ? 62 : platterRect.height
    }

    /// The group's on-screen size: UIKit's platter times groupScale.
    var visualSize: CGSize {
        let rect = platterRect.isEmpty
            ? CGRect(x: 0, y: 0, width: slotWidth * CGFloat(max(1, currentItems.count)), height: 62)
            : platterRect
        return CGSize(width: rect.width * groupScale, height: rect.height * groupScale)
    }

    func fitIfNeeded() {
        let count = max(1, currentItems.count)
        let key = "\(count)|\(Int(slotWidth * 10))|\(window == nil ? 0 : 1)"
        guard key != fittedKey else { return }
        fittedKey = key
        refitPasses = 0
        measure(desiredWidth: slotWidth * CGFloat(count))
    }

    /// Lays the bar out at its native size, widening or narrowing it until
    /// UIKit's own platter is `desiredWidth` wide. Nothing inside is resized.
    private func measure(desiredWidth: CGFloat) {
        var width = desiredWidth + 40
        let height = max(49, bar.sizeThatFits(CGSize(width: width, height: 160)).height)
        var rect = CGRect.zero
        var found = false
        for _ in 0..<4 {
            bar.bounds = CGRect(x: 0, y: 0, width: width, height: height)
            bar.setNeedsLayout()
            bar.layoutIfNeeded()
            guard let platter = ALB3Glass.findPlatter(in: bar) else {
                found = false
                break
            }
            found = true
            rect = platter.convert(platter.bounds, to: bar)
            let delta = desiredWidth - rect.width
            if abs(delta) < 0.5 {
                break
            }
            width = min(max(width + delta, desiredWidth), desiredWidth + 200)
        }
        if !found {
            rect = bar.bounds
        }
        platterRect = rect
        platterFound = found
    }

    override func didMoveToWindow() {
        super.didMoveToWindow()
        fittedKey = ""
        glyphsDirty = true
        setNeedsLayout()
        superview?.setNeedsLayout()
    }

    override func layoutSubviews() {
        super.layoutSubviews()
        fitIfNeeded()
        updateGlyphs()
        let rect = platterRect.isEmpty ? bar.bounds : platterRect
        let scale = groupScale
        bar.transform = CGAffineTransform(scaleX: scale, y: scale)
        bar.center = CGPoint(
            x: bounds.midX - (rect.midX - bar.bounds.midX) * scale,
            y: bounds.midY - (rect.midY - bar.bounds.midY) * scale
        )
        verifyPlatter()
    }

    /// UIKit may lay the bar out again once it is on screen; a moved
    /// platter is measured again, at most twice.
    private func verifyPlatter() {
        guard platterFound,
              refitPasses < 2,
              let platter = ALB3Glass.findPlatter(in: bar)
        else {
            return
        }
        let rect = platter.convert(platter.bounds, to: bar)
        let moved = abs(rect.width - platterRect.width) > 0.5
            || abs(rect.midX - platterRect.midX) > 0.5
            || abs(rect.midY - platterRect.midY) > 0.5
        guard moved else { return }
        refitPasses += 1
        DispatchQueue.main.async { [weak self] in
            guard let self else { return }
            let count = max(1, self.currentItems.count)
            self.measure(desiredWidth: self.slotWidth * CGFloat(count))
            self.setNeedsLayout()
            self.superview?.setNeedsLayout()
        }
    }

    // MARK: Selection

    func tabBar(_ tabBar: UITabBar, didSelect item: UITabBarItem) {
        userSelected(item.tag)
    }

    private func userSelected(_ index: Int) {
        guard index >= 0, index < titles.count else { return }
        selected = index
        onSelect?(index)

        if visibleCount == 1 && !keepSingleSelection {
            DispatchQueue.main.async { [weak self] in
                self?.bar.selectedItem = nil
            }
        }

        // The edge item reveals its neighbour after UIKit's lens settles.
        var start = windowStart
        if visibleCount > 1 {
            if index == windowStart + visibleCount - 1 && start + visibleCount < titles.count {
                start += 1
            } else if index == windowStart && start > 0 {
                start -= 1
            }
        }
        guard start != windowStart else { return }
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.35) { [weak self] in
            guard let self else { return }
            self.windowStart = start
            self.rebuildItems()
        }
    }

    @objc private func swipedLeft() {
        swipe(by: 1)
    }

    @objc private func swipedRight() {
        swipe(by: -1)
    }

    private func swipe(by step: Int) {
        let next = selected + step
        guard next >= 0, next < titles.count else { return }
        selected = next
        rebuildItems()
        onSelect?(next)
    }

    func gestureRecognizer(
        _ gestureRecognizer: UIGestureRecognizer,
        shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer
    ) -> Bool {
        true
    }

    var diagnosticsText: String {
        "分组 原生胶囊 \(Int(platterRect.width))×\(Int(platterRect.height))"
            + (platterFound ? "" : "（未找到胶囊）")
            + " 比例 \(String(format: "%.2f", Double(groupScale)))"
    }
}

// MARK: - Top chrome: left button, category group, search button

@available(iOS 26.0, *)
@objc(ALB3TopChromeView)
public final class ALB3TopChromeView: UIView {
    /// The user picked category `index` (in the titles last applied).
    @objc public var onSelectIndex: ((Int) -> Void)?
    @objc public var onSearch: (() -> Void)?
    /// Left button tapped while it is set to open Douyin's sidebar.
    @objc public var onLeftPrimary: (() -> Void)?
    /// Left button menu, built each time it opens.
    @objc public var menuElementsProvider: (() -> [UIMenuElement])?

    private var navigationBar: ALB3PassThroughNavigationBar?
    private let group = ALB3CategoryGroup()
    private var titles: [String] = []
    private var configuration: [String: Any] = [:]
    private var configurationKey = "unset"
    /// The side buttons' glass centre line, kept while a button is pressed
    /// (UIKit stretches the pressed glass) or its menu is open.
    private var sideGlassMidY: CGFloat = -1

    private var visibleCount: Int { min(4, max(1, (configuration["count"] as? NSNumber)?.intValue ?? 2)) }
    private var keepSingle: Bool { (configuration["keepSingle"] as? NSNumber)?.boolValue ?? true }
    /// 3.3: 1 means exactly as tall as the side buttons' glass.
    private var groupScale: CGFloat { CGFloat(min(1.2, max(0.5, (configuration["groupScale"] as? NSNumber)?.doubleValue ?? 1.0))) }
    private var slotWidth: CGFloat { CGFloat(min(120, max(44, (configuration["slotWidth"] as? NSNumber)?.doubleValue ?? 76))) }
    private var sideScale: CGFloat { CGFloat(min(1.3, max(0.6, (configuration["sideScale"] as? NSNumber)?.doubleValue ?? 1.0))) }
    private var sideMargin: CGFloat { CGFloat(min(40, max(0, (configuration["sideMargin"] as? NSNumber)?.doubleValue ?? 16))) }
    private var hideLeft: Bool { (configuration["hideLeft"] as? NSNumber)?.boolValue ?? false }
    private var hideSearch: Bool { (configuration["hideSearch"] as? NSNumber)?.boolValue ?? false }
    private var leftMenu: Bool { (configuration["leftMenu"] as? NSNumber)?.boolValue ?? true }
    private var showGroup: Bool { (configuration["showGroup"] as? NSNumber)?.boolValue ?? true }
    /// 2.5.3's mode: the side buttons next to the group, at their own gaps.
    private var nearGroup: Bool { (configuration["nearGroup"] as? NSNumber)?.boolValue ?? false }
    private var leftGap: CGFloat { CGFloat(min(80, max(0, (configuration["leftGap"] as? NSNumber)?.doubleValue ?? 14))) }
    private var rightGap: CGFloat { CGFloat(min(80, max(0, (configuration["rightGap"] as? NSNumber)?.doubleValue ?? 14))) }

    @objc public override init(frame: CGRect) {
        super.init(frame: frame)
        backgroundColor = .clear
        clipsToBounds = false
        overrideUserInterfaceStyle = .dark
        accessibilityIdentifier = "AwemeLiquidBar.TopChrome"
        group.onSelect = { [weak self] index in
            self?.onSelectIndex?(index)
        }
        addSubview(group)
        rebuildBar()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    public override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
        guard !isHidden, alpha > 0.01 else { return nil }
        let hit = super.hitTest(point, with: event)
        return hit === self ? nil : hit
    }

    /// Keys: count, keepSingle, groupScale, slotWidth, sideScale,
    /// sideMargin, hideLeft, hideSearch, leftMenu, showGroup, nearGroup,
    /// leftGap, rightGap.
    @objc(applyConfiguration:)
    public func applyConfiguration(_ configuration: [String: Any]) {
        let key = configuration.keys.sorted().map { "\($0)=\(configuration[$0] ?? "")" }.joined(separator: ";")
        guard key != configurationKey else { return }
        let barChanged = (configuration["hideLeft"] as? NSNumber)?.boolValue != hideLeft
            || (configuration["hideSearch"] as? NSNumber)?.boolValue != hideSearch
            || (configuration["leftMenu"] as? NSNumber)?.boolValue != leftMenu
            || configurationKey == "unset"
        self.configuration = configuration
        configurationKey = key
        if barChanged {
            rebuildBar()
        }
        group.isHidden = !showGroup
        // The group's scale is set in layoutSubviews, against the side buttons.
        group.slotWidth = slotWidth
        if !titles.isEmpty {
            group.configure(
                titles: titles,
                selected: group.selected,
                visibleCount: visibleCount,
                keepSingleSelection: keepSingle
            )
        }
        setNeedsLayout()
    }

    @objc(applyTitles:selectedIndex:)
    public func apply(titles: [String], selectedIndex: Int) {
        if titles != self.titles {
            self.titles = titles
            group.configure(
                titles: titles,
                selected: selectedIndex,
                visibleCount: visibleCount,
                keepSingleSelection: keepSingle
            )
        } else {
            group.showCategory(selectedIndex)
        }
        setNeedsLayout()
    }

    @objc(showSelectedIndex:)
    public func showSelectedIndex(_ index: Int) {
        group.showCategory(index)
    }

    /// Vertical centre of the row in this view (Douyin's own title row);
    /// negative centres it in the bounds.
    @objc public var rowCenterY: CGFloat = -1 {
        didSet {
            if abs(rowCenterY - oldValue) > 0.5 {
                setNeedsLayout()
            }
        }
    }

    @objc public var selectedIndex: Int {
        group.selected
    }

    private func rebuildBar() {
        navigationBar?.removeFromSuperview()
        let bar = ALB3PassThroughNavigationBar()
        bar.tintColor = .label
        let item = UINavigationItem(title: "")
        if !hideLeft {
            let left: UIBarButtonItem
            if leftMenu {
                let deferred = UIDeferredMenuElement.uncached { [weak self] completion in
                    completion(self?.menuElementsProvider?() ?? [])
                }
                left = UIBarButtonItem(
                    title: nil,
                    image: UIImage(systemName: "line.3.horizontal"),
                    primaryAction: nil,
                    menu: UIMenu(title: "", children: [deferred])
                )
            } else {
                left = UIBarButtonItem(
                    title: nil,
                    image: UIImage(systemName: "line.3.horizontal"),
                    primaryAction: UIAction(image: UIImage(systemName: "line.3.horizontal")) { [weak self] _ in
                        self?.onLeftPrimary?()
                    },
                    menu: nil
                )
            }
            left.accessibilityLabel = "菜单"
            item.leftBarButtonItem = left
        }
        if !hideSearch {
            let right = UIBarButtonItem(
                title: nil,
                image: UIImage(systemName: "magnifyingglass"),
                primaryAction: UIAction(image: UIImage(systemName: "magnifyingglass")) { [weak self] _ in
                    self?.onSearch?()
                },
                menu: nil
            )
            right.accessibilityLabel = "搜索"
            item.rightBarButtonItem = right
        }
        bar.setItems([item], animated: false)
        insertSubview(bar, belowSubview: group)
        navigationBar = bar
        setNeedsLayout()
    }

    @objc public var contentHeight: CGFloat {
        guard let bar = navigationBar else { return group.visualSize.height }
        return max(bar.frame.height, group.visualSize.height)
    }

    public override func layoutSubviews() {
        super.layoutSubviews()
        let width = bounds.width
        guard width > 0, let bar = navigationBar else { return }
        let midY = rowCenterY >= 0 ? rowCenterY : bounds.midY

        // UIKit keeps its own item sizes; the bar is scaled, and made
        // 1/scale as wide so it still spans the view. The items sit at the
        // side margins, or next to the group at their own gaps.
        let scale = sideScale
        let wantsNear = nearGroup && showGroup && !group.isHidden
        // Near the group the bar keeps the margins it was last corrected to,
        // so a settled layout changes nothing; otherwise the side margins.
        var leading = wantsNear ? bar.directionalLayoutMargins.leading : sideMargin / scale
        var trailing = wantsNear ? bar.directionalLayoutMargins.trailing : sideMargin / scale
        layoutBar(bar, width: width, midY: midY, scale: scale, leading: leading, trailing: trailing)

        // 3.3: at groupScale 1 the group's glass is exactly as tall as the
        // side buttons' glass and sits on their centre line (3.1 scaled
        // UIKit's taller native platter, so 1 was a much bigger capsule set
        // a few points lower).
        group.slotWidth = slotWidth
        group.fitIfNeeded()
        // The height is UIKit's bar-item glass height at the side scale (a
        // measured frame would follow a pressed, stretching glass).
        let sideHeight = ALB3Glass.nominalGlass * scale
        let sideControls = ALB3Glass.controls(in: bar)
        let pressed = sideControls.contains { view in
            guard let control = view as? UIControl else { return false }
            return control.isTracking || control.isHighlighted
        }
        if sideControls.isEmpty {
            sideGlassMidY = -1
        } else if !pressed, let control = sideControls.first {
            let glass = bar.convert(ALB3Glass.glassFrame(around: control, in: bar).frame, to: self)
            if glass.height > 1 {
                sideGlassMidY = glass.midY
            }
        }
        let groupMidY = sideGlassMidY >= 0 ? sideGlassMidY : midY
        let unit = (sideHeight / max(1, group.nativePlatterHeight) * 1000).rounded() / 1000
        group.groupScale = groupScale * unit
        let size = group.visualSize
        group.frame = CGRect(
            x: (width - size.width) / 2,
            y: groupMidY - size.height / 2,
            width: size.width,
            height: size.height
        )
        guard wantsNear, group.frame.width > 1 else { return }
        for _ in 0..<2 {
            let controls = ALB3Glass.controls(in: bar).sorted {
                $0.convert($0.bounds, to: self).midX < $1.convert($1.bounds, to: self).midX
            }
            let leftControl = hideLeft ? nil : controls.first
            let rightControl = hideSearch ? nil : (controls.count > (leftControl == nil ? 0 : 1) ? controls.last : nil)
            var moved = false
            if let control = leftControl {
                let glass = bar.convert(ALB3Glass.glassFrame(around: control, in: bar).frame, to: self)
                let shift = (group.frame.minX - leftGap) - glass.maxX
                if abs(shift) > 0.25 {
                    leading = max(0, leading + shift / scale)
                    moved = true
                }
            }
            if let control = rightControl, control !== leftControl {
                let glass = bar.convert(ALB3Glass.glassFrame(around: control, in: bar).frame, to: self)
                let shift = glass.minX - (group.frame.maxX + rightGap)
                if abs(shift) > 0.25 {
                    trailing = max(0, trailing + shift / scale)
                    moved = true
                }
            }
            guard moved else { break }
            layoutBar(bar, width: width, midY: midY, scale: scale, leading: leading, trailing: trailing)
        }
    }

    private func layoutBar(
        _ bar: UINavigationBar,
        width: CGFloat,
        midY: CGFloat,
        scale: CGFloat,
        leading: CGFloat,
        trailing: CGFloat
    ) {
        let margins = bar.directionalLayoutMargins
        if abs(margins.leading - leading) > 0.01 || abs(margins.trailing - trailing) > 0.01 {
            bar.directionalLayoutMargins = NSDirectionalEdgeInsets(
                top: margins.top,
                leading: leading,
                bottom: margins.bottom,
                trailing: trailing
            )
        }
        let barWidth = width / scale
        let barHeight = max(44, bar.sizeThatFits(CGSize(width: barWidth, height: 200)).height)
        bar.transform = .identity
        bar.bounds = CGRect(x: 0, y: 0, width: barWidth, height: barHeight)
        bar.center = CGPoint(x: width / 2, y: midY)
        bar.transform = CGAffineTransform(scaleX: scale, y: scale)
        bar.layoutIfNeeded()
    }

    @objc public var diagnosticsText: String {
        var lines = [group.diagnosticsText]
        if let bar = navigationBar {
            lines.append("导航栏 原生高 \(Int(bar.bounds.height)) 比例 \(String(format: "%.2f", Double(sideScale)))")
            for (number, control) in ALB3Glass.controls(in: bar).enumerated() {
                let frame = control.convert(control.bounds, to: nil)
                lines.append("  按钮\(number + 1) x\(Int(frame.minX)) y\(Int(frame.minY)) \(Int(frame.width))×\(Int(frame.height))")
            }
        }
        return lines.joined(separator: "\n")
    }
}
