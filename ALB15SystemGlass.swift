import UIKit
import SwiftUI
import ObjectiveC

// The same native APIs Apple uses in the custom-glass example: container +
// capsule effects, frame changes in UIView.animate's SYSTEM DEFAULT animation.
// No sampled progress, authored timing curve, nonuniform carrier transform,
// corner-radius animation, rounded-rectangle mask or delayed hide timer.
@available(iOS 26.0, *)
@MainActor
private final class ALB15PassThroughView: UIView {
    override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
        guard !isHidden, alpha > 0.01, isUserInteractionEnabled else { return nil }
        for child in subviews.reversed() {
            if let hit = child.hitTest(convert(point, to: child), with: event) {
                var candidate: UIView? = hit
                while let view = candidate, view !== self {
                    if view is UIControl { return hit }
                    candidate = view.superview
                }
            }
        }
        return nil
    }
}

@available(iOS 26.0, *)
@MainActor
private final class ALB15VisibilityLease {
    weak var view: UIView?
    let originalOpacity: Float
    let originalInteraction: Bool
    init(_ view: UIView) {
        self.view = view
        originalOpacity = view.layer.opacity
        originalInteraction = view.isUserInteractionEnabled
    }
    func suppress() {
        guard let view else { return }
        // Do not toggle isHidden on a native arranged surface: that can make
        // UIKit reflow its geometry. This is an all-or-nothing render handoff,
        // not a translucent glass layer or an opacity animation.
        CATransaction.begin(); CATransaction.setDisableActions(true)
        if view.layer.opacity != 0 { view.layer.opacity = 0 }
        if view.isUserInteractionEnabled { view.isUserInteractionEnabled = false }
        CATransaction.commit()
    }
    func restore() {
        guard let view else { return }
        CATransaction.begin(); CATransaction.setDisableActions(true)
        if view.layer.opacity != originalOpacity { view.layer.opacity = originalOpacity }
        if view.isUserInteractionEnabled != originalInteraction { view.isUserInteractionEnabled = originalInteraction }
        CATransaction.commit()
    }
}

@available(iOS 26.0, *)
@MainActor
@objc(ALB15SystemMerge)
final class ALB15SystemMerge: NSObject {
    private weak var bar: UITabBar?
    private weak var host: UIView?
    private weak var leftSource: UIView?
    private weak var rightSource: UIView?
    private var leases: [ALB15VisibilityLease] = []
    private let overlay = ALB15PassThroughView()
    private let container = UIVisualEffectView(effect: UIGlassContainerEffect())
    private let leftGlass = UIVisualEffectView()
    private let rightGlass = UIVisualEffectView()
    private let content = UIView()
    private let avatarButton = UIButton(type: .custom)
    private var effectLeft: UIVisualEffect?
    private var effectRight: UIVisualEffect?
    private var originalRightImage: UIImage?
    private var showingAvatar: UIImage?
    private var requested = false
    private var visible = false
    private var transitionState = ALB15StateMake()
    private var lastLeft = CGRect.null
    private var lastRight = CGRect.null
    private var target = CGRect.null
    private var transitioning = false
    private var statusText = "等待原生玻璃材料"
    private var style = UIUserInterfaceStyle.unspecified

    override init() {
        super.init()
        overlay.accessibilityIdentifier = "AwemeLiquidBar.NativeMerge.Root"
        overlay.backgroundColor = .clear
        overlay.isOpaque = false
        overlay.clipsToBounds = false
        container.accessibilityIdentifier = "AwemeLiquidBar.NativeMerge.Container"
        container.clipsToBounds = false
        overlay.addSubview(container)
        for glass in [leftGlass, rightGlass] {
            glass.cornerConfiguration = .capsule()
            glass.clipsToBounds = false
            container.contentView.addSubview(glass)
        }
        leftGlass.accessibilityIdentifier = "AwemeLiquidBar.NativeMerge.Left"
        rightGlass.accessibilityIdentifier = "AwemeLiquidBar.NativeMerge.Right"
        leftGlass.isUserInteractionEnabled = false
        leftGlass.contentView.addSubview(content)
        content.isUserInteractionEnabled = false
        // Only content: UIButton adds no second glass background. Its physical
        // touch is inside the INTERACTIVE UIGlassEffect's actual contentView.
        avatarButton.backgroundColor = .clear
        avatarButton.configuration = .plain()
        avatarButton.configuration?.contentInsets = .zero
        avatarButton.imageView?.contentMode = .scaleAspectFit
        avatarButton.accessibilityLabel = "展开底栏"
        avatarButton.accessibilityIdentifier = "AwemeLiquidBar.NativeMerge.Expand"
        avatarButton.addTarget(self, action: #selector(touchBegan), for: .touchDown)
        avatarButton.addTarget(self, action: #selector(expand), for: .touchUpInside)
        avatarButton.addTarget(self, action: #selector(touchCancelled), for: .touchCancel)
        rightGlass.contentView.addSubview(avatarButton)
        overlay.isHidden = true
    }

    @objc var active: Bool { visible }
    @objc var status: NSString { statusText as NSString }
    @objc var hitRect: CGRect {
        guard visible, let bar, !target.isNull else { return .null }
        // Include both positions during native reverse/fusion animation.
        let occupied = requested ? target : target.union(lastLeft).union(lastRight)
        return overlay.convert(occupied, to: bar)
    }

    @objc(hitAt:event:)
    func hit(at point: CGPoint, event: UIEvent?) -> UIView? {
        guard visible, let bar, requested else { return nil }
        let local = bar.convert(point, to: avatarButton)
        guard avatarButton.point(inside: local, with: event) else { return nil }
        // Returning the existing button preserves the UIGlassEffect ancestor
        // recognizers. Do not invent touches or return a noninteractive wrapper.
        return avatarButton.hitTest(local, with: event) ?? avatarButton
    }

    @objc private func touchBegan() { log("COMPACT_TOUCH_DOWN nativeInteractive=1 content=UIButton") }
    @objc private func touchCancelled() { log("COMPACT_TOUCH_CANCEL") }
    @objc private func expand() {
        guard visible, requested, let bar else { return }
        log("COMPACT_TOUCH_UP_INSIDE expand=1")
        // No sibling UITapGestureRecognizer and no synthetic tab selection.
        ALB15ExpandCompactBar(bar)
    }
    private func log(_ text: String) {
        ("GLASS15 " + text).withCString { AWEAlbumMorphProbeLog($0) }
    }
    private func circleImage(_ image: UIImage, side: CGFloat) -> UIImage {
        let renderer = UIGraphicsImageRenderer(size: CGSize(width: side, height: side))
        return renderer.image { _ in
            UIBezierPath(ovalIn: CGRect(x: 0, y: 0, width: side, height: side)).addClip()
            let ratio = max(side / max(1, image.size.width), side / max(1, image.size.height))
            let size = CGSize(width: image.size.width * ratio, height: image.size.height * ratio)
            image.draw(in: CGRect(x: (side-size.width)/2, y: (side-size.height)/2, width: size.width, height: size.height))
        }.withRenderingMode(.alwaysOriginal)
    }
    private func updateButtonImage(side: CGFloat) {
        let own = ALB8OwnAvatar()
        guard !avatarButton.isTracking, own !== showingAvatar || avatarButton.currentImage == nil else { return }
        showingAvatar = own
        let image = own.map { circleImage($0, side: max(12, side-12)) }
            ?? originalRightImage ?? UIImage(systemName: "magnifyingglass")
        var cfg = UIButton.Configuration.plain()
        cfg.contentInsets = NSDirectionalEdgeInsets(top: 6, leading: 6, bottom: 6, trailing: 6)
        cfg.image = image
        cfg.baseForegroundColor = .label
        avatarButton.configuration = cfg
    }
    private func buildForeground(bar: UITabBar, leftSize: CGSize) {
        content.subviews.forEach { $0.removeFromSuperview() }
        content.frame = CGRect(origin: .zero, size: leftSize)
        // Copy only foreground images/text, not a screenshot containing glass.
        let items = Array((bar.items ?? []).dropLast())
        guard !items.isEmpty else { return }
        let width = leftSize.width / CGFloat(items.count)
        for (index, item) in items.enumerated() {
            let chosen = bar.selectedItem === item
            let image = chosen ? (item.selectedImage ?? item.image) : item.image
            let imageView = UIImageView(image: image)
            imageView.contentMode = .scaleAspectFit
            imageView.tintColor = chosen ? bar.tintColor : bar.unselectedItemTintColor
            imageView.frame = CGRect(x: CGFloat(index)*width+(width-27)/2, y: (leftSize.height-27)/2, width: 27, height: 27)
            content.addSubview(imageView)
            if let badge = item.badgeValue, !badge.isEmpty {
                let label = UILabel()
                label.text = badge; label.font = .systemFont(ofSize: 11, weight: .semibold)
                label.textAlignment = .center; label.textColor = .white; label.backgroundColor = .systemRed
                label.layer.cornerRadius = 9; label.clipsToBounds = true
                let w = max(18, label.intrinsicContentSize.width+8)
                label.frame = CGRect(x: imageView.frame.maxX-1, y: imageView.frame.minY-5, width: w, height: 18)
                content.addSubview(label)
            }
        }
    }

    @objc(updateWithBar:host:left:right:leftRect:rightRect:compact:animated:)
    func update(bar: UITabBar, host: UIView, left: UIView, right: UIView,
                leftRect: CGRect, rightRect: CGRect, compact: Bool, animated: Bool) -> Bool {
        guard bar.window != nil, host.window === bar.window,
              leftRect.width > 0, rightRect.width > 0 else { return false }
        let newSources = self.host !== host || leftSource !== left || rightSource !== right
        if newSources { reset() }
        self.bar = bar; self.host = host; leftSource = left; rightSource = right
        let changed = requested != compact
        let geometryChanged = lastLeft != leftRect || lastRight != rightRect
        let nativeStyle = left.traitCollection.userInterfaceStyle
        let styleChanged = style != nativeStyle
        // Leave the original Photos-style bar completely in charge while expanded.
        if !compact && !visible {
            requested = false
            statusText = "原生展开 · 未覆盖材质"
            return true
        }
        if avatarButton.isTracking && visible && (styleChanged || geometryChanged) {
            leases.forEach { $0.suppress() }
            return true
        }
        if !visible || styleChanged || effectLeft == nil || effectRight == nil {
            guard let le = ALB15NativeGlassEffect(left), let re = ALB15NativeGlassEffect(right),
                  let lc = ALB15CopyInteractiveGlass(le, false),
                  let rc = ALB15CopyInteractiveGlass(re, true) else {
                statusText = "材料读取失败：保留原栏，不创建近似玻璃"
                log("MATERIAL_UNAVAILABLE left=\(type(of: left)) right=\(type(of: right))")
                return false
            }
            effectLeft = lc; effectRight = rc
            style = nativeStyle
            container.overrideUserInterfaceStyle = nativeStyle
            // Preserve source style/tint. Only the interactive property changes.
            leftGlass.effect = lc; rightGlass.effect = rc
            log("MATERIAL_COPY left=\(type(of: le)) right=\(type(of: re)) interactive=1")
        }
        if overlay.superview !== host { host.addSubview(overlay) }
        if overlay.frame != host.bounds { overlay.frame = host.bounds }
        if container.frame != overlay.bounds { container.frame = overlay.bounds }
        if !visible {
            leases = [ALB15VisibilityLease(left), ALB15VisibilityLease(right)]
            leftGlass.isHidden = false
            leftGlass.frame = leftRect; rightGlass.frame = rightRect
            content.alpha = 1
            buildForeground(bar: bar, leftSize: leftRect.size)
            originalRightImage = bar.items?.last?.image
            overlay.isHidden = false
            visible = true
        }
        // Original glass and the transition glass are never drawn together.
        for lease in leases { lease.suppress() }
        let diameter = min(rightRect.width, rightRect.height)
        let union = leftRect.union(rightRect)
        let finalRect = CGRect(x: union.midX-diameter/2, y: rightRect.midY-diameter/2,
                               width: diameter, height: diameter)
        updateButtonImage(side: diameter)
        // The button fills the real effect.contentView, with a symmetric inset
        // image. Autoresizing does not overwrite its native tracking transform.
        if !avatarButton.isTracking && avatarButton.bounds.size != rightGlass.contentView.bounds.size {
            avatarButton.bounds = rightGlass.contentView.bounds
            avatarButton.center = CGPoint(x: rightGlass.contentView.bounds.midX, y: rightGlass.contentView.bounds.midY)
        }
        avatarButton.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        guard changed || geometryChanged || newSources || !requested && compact else { return true }
        requested = compact
        lastLeft = leftRect; lastRight = rightRect; target = finalRect
        let revision = ALB15StateBegin(&transitionState, compact ? 1 : 0)
        transitioning = true
        leftGlass.isHidden = false
        if leftGlass.effect == nil { leftGlass.effect = effectLeft }
        let changes = { [self] in
            // Height stays constant; capsule(maximumRadius:nil) is installed
            // before this transaction. There is no rectangle keyframe.
            leftGlass.frame = compact ? finalRect : leftRect
            rightGlass.frame = compact ? finalRect : rightRect
            content.alpha = compact ? 0 : 1
            avatarButton.alpha = compact ? 1 : 0
        }
        let finish = { [weak self] in
            guard let self, ALB15StateComplete(&self.transitionState, revision, compact ? 1 : 0) != 0 else { return }
            self.transitioning = false
            if compact {
                // Same container made one combined surface. Remove redundant
                // geometry only after the native completion, never on a timer.
                self.leftGlass.effect = nil
                self.leftGlass.isHidden = true
                self.statusText = "中心圆球 · 单层原生交互玻璃 · 系统默认融合"
            } else {
                self.leases.forEach { $0.restore() }
                self.leases.removeAll()
                self.overlay.isHidden = true
                self.visible = false
                self.statusText = "原生展开 · 已恢复原栏"
            }
            self.log("MERGE_COMPLETED compact=\(compact) generation=\(revision)")
        }
        log("MERGE_REQUEST compact=\(compact) geometry=capsule container=UIGlassContainerEffect timing=system-default generation=\(revision)")
        if animated && !UIAccessibility.isReduceMotionEnabled {
            // Do not impose a plugin-specific collapse speed. Use the duration
            // of UIKit/Core Animation's current transaction for this native
            // glass transition, and let UIGlassContainerEffect do the morph.
            let duration = CATransaction.animationDuration()
            UIKit.UIView.animate(withDuration: duration,
                                 delay: 0,
                                 options: [.beginFromCurrentState, .allowUserInteraction],
                                 animations: changes) { _ in finish() }
        } else {
            UIKit.UIView.performWithoutAnimation(changes)
            finish()
        }
        return true
    }
    @objc func reset() {
        ALB15StateInvalidate(&transitionState)
        // Source/provider replacement cancels only our outer geometry layers.
        // Stale native completion closures are rejected by the shared token.
        for view in [leftGlass, rightGlass, content, avatarButton] { view.layer.removeAllAnimations() }
        leases.forEach { $0.restore() }; leases.removeAll()
        overlay.removeFromSuperview()
        overlay.isHidden = true
        requested = false; visible = false; transitioning = false
        lastLeft = .null; lastRight = .null; target = .null
        effectLeft = nil; effectRight = nil; showingAvatar = nil
        leftGlass.effect = nil; rightGlass.effect = nil
    }
}


private var ALB16TopSlotGlassKey: UInt8 = 0
private var ALB16TopEffectManagedKey: UInt8 = 0

@available(iOS 26.0, *)
@MainActor
@objc(ALB15TopMaterial)
final class ALB15TopMaterial: NSObject {
    private var leftGlass = UIVisualEffectView()
    private var rightGlass = UIVisualEffectView()
    private weak var previousLeft: UIButton?
    private weak var previousRight: UIButton?
    private weak var sourceView: UIView?
    private weak var leftSlotView: UIView?
    private weak var rightSlotView: UIView?
    @objc func refresh() {
        if let source = sourceView, let ls = leftSlotView, let rs = rightSlotView,
           let lb = previousLeft, let rb = previousRight {
            update(source: source, leftSlot: ls, leftButton: lb, rightSlot: rs, rightButton: rb)
        }
    }
    private var material: UIVisualEffect?
    private var materialSignature: String?
    private var style = UIUserInterfaceStyle.unspecified
    private var updating = false
    @objc var status: NSString {
        guard let material else { return "中央材质未识别；未覆盖按钮背景" }
        return "中央材料同源：\(type(of: material)) · 两侧interactive=true · 单层背景" as NSString
    }

    @objc(updateWithSource:leftSlot:leftButton:rightSlot:rightButton:)
    func update(source: UIView, leftSlot: UIView, leftButton: UIButton,
                rightSlot: UIView, rightButton: UIButton) {
        guard !updating, source.window != nil else { return }
        sourceView = source; leftSlotView = leftSlot; rightSlotView = rightSlot
        previousLeft = leftButton; previousRight = rightButton
        guard !leftButton.isTracking, !rightButton.isTracking else { return }
        guard let current = ALB15NativeGlassEffect(source) else { return }
        // Reuse the per-slot surfaces if the native group provider is replaced.
        // A second coordinator must never leave an old glass in the same slot.
        let key = UnsafeRawPointer(&ALB16TopSlotGlassKey)
        if let existing = objc_getAssociatedObject(leftSlot, key) as? UIVisualEffectView, existing !== leftGlass {
            leftGlass = existing; material = nil; materialSignature = nil
        } else { objc_setAssociatedObject(leftSlot, key, leftGlass, .OBJC_ASSOCIATION_RETAIN_NONATOMIC) }
        if let existing = objc_getAssociatedObject(rightSlot, key) as? UIVisualEffectView, existing !== rightGlass {
            rightGlass = existing; material = nil; materialSignature = nil
        } else { objc_setAssociatedObject(rightSlot, key, rightGlass, .OBJC_ASSOCIATION_RETAIN_NONATOMIC) }
        updating = true
        defer { updating = false }
        let currentStyle = source.traitCollection.userInterfaceStyle
        let signature = ALB15GlassSignature(current, source.traitCollection)
        if materialSignature != signature || material == nil || style != currentStyle {
            guard let copied = ALB15CopyInteractiveGlass(current, true) else { return }
            material = current; materialSignature = signature; style = currentStyle
            for glass in [leftGlass, rightGlass] {
                glass.effect = copied.copy() as? UIVisualEffect
                glass.cornerConfiguration = .capsule()
                glass.overrideUserInterfaceStyle = currentStyle
                glass.clipsToBounds = false
                glass.accessibilityIdentifier = "AwemeLiquidBar.TopNative.SourceCopiedGlass"
            }
        }
        for (glass, slot, button) in [(leftGlass, leftSlot, leftButton), (rightGlass, rightSlot, rightButton)] {
            if button.isTracking { continue }
            if glass.superview !== slot { slot.addSubview(glass) }
            if glass.frame != slot.bounds { glass.frame = slot.bounds }
            glass.autoresizingMask = [.flexibleWidth, .flexibleHeight]
            if !ALB15TopButtonManaged(button) {
                // Keep the business button, targets, menus and native touch
                // leases. Remove ONLY its independent background configuration.
                let old = button.configuration
                var plain = UIButton.Configuration.plain()
                plain.image = old?.image ?? button.currentImage
                plain.preferredSymbolConfigurationForImage = old?.preferredSymbolConfigurationForImage
                plain.baseForegroundColor = old?.baseForegroundColor
                plain.contentInsets = .zero
                button.configuration = plain
                objc_setAssociatedObject(button, UnsafeRawPointer(&ALB16TopEffectManagedKey), NSNumber(value: true), .OBJC_ASSOCIATION_RETAIN_NONATOMIC)
            }
            if button.superview !== glass.contentView { glass.contentView.addSubview(button) }
            if button.bounds != glass.contentView.bounds { button.bounds = glass.contentView.bounds }
            let center = CGPoint(x: glass.contentView.bounds.midX, y: glass.contentView.bounds.midY)
            if button.center != center { button.center = center }
            button.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        }
        previousLeft = leftButton; previousRight = rightButton
    }
}
