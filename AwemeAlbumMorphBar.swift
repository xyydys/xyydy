import UIKit
import QuartzCore


private extension CGAffineTransform {
    func isApproximatelyEqual(to other: CGAffineTransform, tolerance: CGFloat = 0.01) -> Bool {
        abs(a - other.a) < tolerance &&
        abs(b - other.b) < tolerance &&
        abs(c - other.c) < tolerance &&
        abs(d - other.d) < tolerance &&
        abs(tx - other.tx) < tolerance &&
        abs(ty - other.ty) < tolerance
    }
}


@objc(AWEAlbumMorphActionSink)
protocol AWEAlbumMorphActionSink: AnyObject {
    @objc(albumMorphDidRequestOriginalIndex:)
    func albumMorphDidRequestOriginalIndex(_ index: Int)

    @objc(albumMorphDidRequestSearch)
    func albumMorphDidRequestSearch()
}

// C helpers are imported through AwemeLiquidBar-Bridging-Header.h, so the
// compiler uses their actual C ABI (including CGRect and Objective-C BOOL).

private func albumMorphProbe(_ message: String) {
    message.withCString { AWEAlbumMorphProbeLog($0) }
}

private var albumMorphHideTitles: Bool {
    AWEAlbumMorphHideTitlesEnabled() != 0
}

private func albumMorphConfiguredImage(
    key: String,
    fallback: String,
    pointSize: CGFloat
) -> UIImage? {
    let raw: UnsafeMutableRawPointer? = key.withCString { keyCString in
        fallback.withCString { fallbackCString in
            AWEAlbumMorphCreateConfiguredImage(
                keyCString, fallbackCString, Double(pointSize)
            )
        }
    }
    guard let raw else { return UIImage(systemName: fallback) }
    return Unmanaged<UIImage>.fromOpaque(raw).takeRetainedValue()
}

@available(iOS 26.0, *)
private final class AWEAlbumMorphProxyCell: UICollectionViewCell {
    static let reuseIdentifier = "AWEAlbumMorphProxyCell"
}

/// Preserve v9's transparent UICollectionViewController context. UIKit uses
/// this content-scroll relationship when choosing the native floating material.
/// Host feed paging is observed separately and never writes this collection.
@available(iOS 26.0, *)
@objc(AWEAlbumMorphProxyController)
private final class AWEAlbumMorphProxyController: UICollectionViewController {
    let originalIndex: Int?
    let representsSearch: Bool

    init(originalIndex: Int?, representsSearch: Bool = false) {
        self.originalIndex = originalIndex
        self.representsSearch = representsSearch

        let itemSize = NSCollectionLayoutSize(
            widthDimension: .fractionalWidth(1.0),
            heightDimension: .absolute(96.0)
        )
        let item = NSCollectionLayoutItem(layoutSize: itemSize)
        let groupSize = NSCollectionLayoutSize(
            widthDimension: .fractionalWidth(1.0),
            heightDimension: .absolute(96.0)
        )
        let group = NSCollectionLayoutGroup.vertical(
            layoutSize: groupSize,
            repeatingSubitem: item,
            count: 1
        )
        let section = NSCollectionLayoutSection(group: group)
        let layout = UICollectionViewCompositionalLayout(section: section)

        super.init(collectionViewLayout: layout)
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        view.backgroundColor = .clear
        view.isOpaque = false
        collectionView.backgroundColor = .clear
        collectionView.isOpaque = false
        collectionView.alwaysBounceVertical = true
        collectionView.showsVerticalScrollIndicator = false
        collectionView.contentInsetAdjustmentBehavior = .never
        collectionView.isUserInteractionEnabled = false
        collectionView.accessibilityIdentifier =
            "AwemeLiquidBar.AlbumMorph.ProxyCollection"
        collectionView.register(
            AWEAlbumMorphProxyCell.self,
            forCellWithReuseIdentifier: AWEAlbumMorphProxyCell.reuseIdentifier
        )

        // The proxy stays visually inert. The actual user-controlled source
        // scroll view is bound later through UIKit's public content-scroll API.
        albumMorphProbe(
            "PROXY_VIEW_DID_LOAD index=\(originalIndex.map(String.init) ?? "search") " +
            "bounds=\(collectionView.bounds) content=\(collectionView.contentSize)"
        )
    }

    override func collectionView(
        _ collectionView: UICollectionView,
        numberOfItemsInSection section: Int
    ) -> Int {
        80
    }

    override func collectionView(
        _ collectionView: UICollectionView,
        cellForItemAt indexPath: IndexPath
    ) -> UICollectionViewCell {
        let cell = collectionView.dequeueReusableCell(
            withReuseIdentifier: AWEAlbumMorphProxyCell.reuseIdentifier,
            for: indexPath
        )
        cell.backgroundColor = .clear
        cell.contentView.backgroundColor = .clear
        return cell
    }



}

@available(iOS 26.0, *)
@_cdecl("AWEAlbumMorphCreateController")
public func AWEAlbumMorphCreateController() -> UnsafeMutableRawPointer {
    let controller = AWEAlbumMorphTabController()
    return Unmanaged.passRetained(controller).toOpaque()
}

@available(iOS 26.0, *)
@objc(AWEAlbumMorphTabController)
final class AWEAlbumMorphTabController:
    UITabBarController,
    UITabBarControllerDelegate, UIGestureRecognizerDelegate {

    @objc weak var actionSink: AWEAlbumMorphActionSink?

    private enum Identifier {
        static let home = "com.xyy.awemeliquidbar.album.home"
        static let second = "com.xyy.awemeliquidbar.album.second"
        static let publish = "com.xyy.awemeliquidbar.album.publish"
        static let messages = "com.xyy.awemeliquidbar.album.messages"
        static let profile = "com.xyy.awemeliquidbar.album.profile"
    }

    private enum ItemKey {
        static let home = "home"
        static let second = "second"
        static let publish = "publish"
        static let messages = "messages"
        static let profile = "profile"
        static let search = "search"

        static let allowed = [home, second, publish, messages, profile, search]
        static let defaultVisible = [home, second, messages, profile, search]
        static let selectablePages = Set([home, second, messages, profile])
    }

    private var proxies: [AWEAlbumMorphProxyController] = []
    private var suppressCallbacks = false
    private var secondTitle = "朋友"
    private var explicitImagesByIdentifier: [String: (normal: UIImage?, selected: UIImage?)] = [:]
    private var secondNormalSymbol = "hands.and.sparkles"
    private var secondImageRole = "friends"
    private var cachedOriginalIndex = 0
    private var pendingUserIndex: Int?
    private var pendingSelectionUntil: CFTimeInterval = 0
    private var compactMode = false
    private var secondIsShop = false
    private var boundDriver: UIScrollView?
    private weak var boundPage: UIViewController?
    private weak var boundRootScroll: UIScrollView?
    private var driverGeneration = 0
    private var requestedCompact = false
    private var nativeRequestOutstanding = false
    private var nativeRequestAccepted = false
    private var nativeRequestGeneration = 0
    private var nativeRefreshQueued = false
    private var nativeRequestDispatchQueued = false
    private var nativeRequestInFlight = false
    // TEST42 FIX8: device log shows every collapse after the first was issued
    // ~0.55 s after an expand, while UIKit's ~0.8 s minimize spring was still
    // running. Reversing it mid-flight is what overshoots (too small, then
    // springs back); the first collapse, from rest, never did. The next
    // request now waits for UIKit's own animation to complete (its Core
    // Animation transaction completion), with a bounded staleness guard.
    private var nativeAnimationRunningSince: CFTimeInterval?
    private var nativeRequestWaitingForSettle = false
    private var avatarArtworkActive = false
    private var lastObservedNativeState: Int32 = -1
    private var lastConfiguredCompactStyle = 1
    private var verticalOffset: CGFloat = 0
    private var bottomLayoutInProgress = false
    private weak var sourceScrollView: UIScrollView?
    private var nativeMinimizeEnabled = true
    private var morphVisibleFraction: CGFloat = 1
    private let visibilityMaskView = UIView(frame: .zero)
    private var hasUserDrivenSelection = false
    private var visibleItemIdentifiers = ItemKey.defaultVisible
    // UIKit's compact (iPhone) tab bar shows at most five root tabs; with six
    // or more it shows the first four plus the legacy More item. Set once the
    // live bar has shown that More item for five regular tabs + UISearchTab.
    private static let compactRootTabCapacity = 5
    // 3.5: that limit is UITabBarController's own (_effectiveMaxItems, set
    // by _setMaximumNumberOfItems:, the 3.4 诊断 names). Raised to six, five
    // regular tabs and Search are six root tabs without More.
    private static let raisedRootTabCapacity = 6
    private var rootTabCapacityState = "未设置"
    private var searchYieldsToRegularTabs = false
    private var messageBadgeValue: String?
    private var secondBadgeValue: String?
    private var reproLastLayoutSample: CFTimeInterval = 0
    private var reproLastRequestSample: CFTimeInterval = 0
    private var reproLayoutReentries = 0

    override func viewDidLoad() {
        super.viewDidLoad()
        raiseRootTabCapacity()

        view.backgroundColor = .clear
        view.isOpaque = false
        view.tintColor = .systemCyan

        delegate = self
        lastConfiguredCompactStyle = ALB15CompactStyle()
        tabBarMinimizeBehavior = .onScrollDown

        // 3.0: fixed Dark, never following the system or the page.
        overrideUserInterfaceStyle = .dark
        view.overrideUserInterfaceStyle = .dark
        tabBar.overrideUserInterfaceStyle = .dark

        tabBar.accessibilityIdentifier =
            "AwemeLiquidBar.AlbumMorph.SystemTabBar"
        tabBar.isTranslucent = true
        tabBar.isOpaque = false
        tabBar.backgroundColor = .clear
        tabBar.clipsToBounds = false
        visibilityMaskView.backgroundColor = .black
        visibilityMaskView.isUserInteractionEnabled = false

        // Keep every regular item on the same native sizing path. Never scale
        // the whole tab bar or squeeze only the two end items when five page/
        // action buttons are enabled beside the independent search capsule.
        tabBar.itemPositioning = .fill
        tabBar.itemWidth = 0
        tabBar.itemSpacing = 0

        rebuildTabs(preservingOriginalIndex: 0)
        // No cancelling overlay tap recognizer. Real touches remain on UIKit's
        // native tab / auxiliary button for lens, press, stretch and expansion.
        NotificationCenter.default.addObserver(self, selector: #selector(nativeOptionsChanged(_:)),
            name: Notification.Name("ALB5.OptionsChanged"), object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(ownProfileImageChanged(_:)),
            name: Notification.Name("ALB8OwnProfileDidChange"), object: nil)
        // Preserve compact intent through scene suspension. A surviving
        // native hierarchy retains its leases; a rebuilt one is recaptured.
        for name in [UIApplication.willEnterForegroundNotification, UIApplication.didBecomeActiveNotification,
                     UIApplication.willResignActiveNotification, UIApplication.didEnterBackgroundNotification] {
            NotificationCenter.default.addObserver(self, selector: #selector(reproApplicationLifecycle(_:)),
                name: name, object: nil)
        }
        reproRecordController("bottom.controller.viewDidLoad")
    }

    private func reproControllerSnapshot() -> [String: Any] {
        guard ALBReproIsRecording() else { return [:] }
        let root = viewIfLoaded
        let parent = root?.superview ?? root?.window
        var fields: [String: Any] = [
            "controllerID": String(describing: ObjectIdentifier(self)),
            "root": ALBReproBottomViewState(root), "rootParent": ALBReproBottomViewState(parent),
            "verticalOffset": Double(verticalOffset),
            "compactStyle": ALB15CompactStyle(), "compactMode": compactMode,
            "requestedCompact": requestedCompact, "requestOutstanding": nativeRequestOutstanding,
            "requestAccepted": nativeRequestAccepted, "requestGeneration": nativeRequestGeneration,
            "requestInFlight": nativeRequestInFlight, "requestQueued": nativeRequestDispatchQueued,
            "avatarArtworkActive": avatarArtworkActive, "lastObservedNativeState": lastObservedNativeState,
            "minimizeEnabled": nativeMinimizeEnabled, "visibleFraction": Double(morphVisibleFraction),
            "currentOriginalIndex": cachedOriginalIndex, "layoutInProgress": bottomLayoutInProgress,
            "layoutReentriesSinceSample": reproLayoutReentries,
            "additionalSafeArea": [Double(additionalSafeAreaInsets.top), Double(additionalSafeAreaInsets.left),
                                   Double(additionalSafeAreaInsets.bottom), Double(additionalSafeAreaInsets.right)]
        ]
        if let root {
            fields["tabBar"] = ALBReproBottomViewState(tabBar, in: root)
        }
        return fields
    }

    private func reproRecordController(_ name: String) {
        guard ALBReproIsRecording() else { return }
        ALBReproRecordEvent(name, reproControllerSnapshot())
    }

    @objc private func reproApplicationLifecycle(_ notification: Notification) {
        if ALBReproIsRecording() {
            var fields = reproControllerSnapshot()
            fields["notification"] = notification.name.rawValue
            ALBReproRecordEvent("bottom.controller.applicationLifecycle", fields)
        }
        if notification.name != UIApplication.willResignActiveNotification &&
           notification.name != UIApplication.didEnterBackgroundNotification && isViewLoaded {
            view.setNeedsLayout()
            view.layoutIfNeeded()
            queueNativeRefresh()
        }
    }

    private func makeProxy(
        originalIndex: Int?,
        search: Bool = false
    ) -> UIViewController {
        let proxy = AWEAlbumMorphProxyController(
            originalIndex: originalIndex,
            representsSearch: search
        )
        proxies.append(proxy)

        return proxy
    }

    private func makeRegularTab(
        title: String,
        iconRole: String,
        fallback: String,
        identifier: String,
        originalIndex: Int
    ) -> UITab {
        // 2.4.2: 0 = no symbol configuration; UIKit's floating tab bar uses
        // its own default glyph size (the forced 22 pt looked oversized).
        let image = albumMorphConfiguredImage(
            key: iconRole,
            fallback: fallback,
            pointSize: 0.0
        )
        explicitImagesByIdentifier[identifier] = (image, image)
        let tab = UITab(
            title: albumMorphHideTitles ? "" : title,
            image: image,
            identifier: identifier
        ) { [weak self] _ in
            self?.makeProxy(originalIndex: originalIndex)
                ?? UIViewController()
        }
        if #available(iOS 26.1, *) {
            tab.selectedImage = image
        }
        return tab
    }

    private func makeSearchTab() -> UISearchTab {
        let search = UISearchTab { [weak self] _ in
            self?.makeProxy(originalIndex: nil, search: true)
                ?? UIViewController()
        }
        search.automaticallyActivatesSearch = false
        search.image = albumMorphConfiguredImage(
            key: "newbar.search.normal",
            fallback: "magnifyingglass",
            pointSize: 0.0
        )
        return search
    }

    private func tabForItemKey(_ key: String) -> UITab? {
        switch key {
        case ItemKey.home:
            return makeRegularTab(
                title: "首页",
                iconRole: "newbar.home.normal",
                fallback: "photo.on.rectangle.angled",
                identifier: Identifier.home,
                originalIndex: 0
            )
        case ItemKey.second:
            return makeRegularTab(
                title: secondTitle,
                iconRole: "newbar.\(secondImageRole).normal",
                fallback: secondNormalSymbol,
                identifier: Identifier.second,
                originalIndex: 1
            )
        case ItemKey.publish:
            return makeRegularTab(
                title: "发布",
                iconRole: "newbar.publish.normal",
                fallback: "plus",
                identifier: Identifier.publish,
                originalIndex: 2
            )
        case ItemKey.messages:
            return makeRegularTab(
                title: "消息",
                iconRole: "newbar.messages.normal",
                fallback: "message",
                identifier: Identifier.messages,
                originalIndex: 3
            )
        case ItemKey.profile:
            return makeRegularTab(
                title: "我",
                iconRole: "newbar.profile.normal",
                fallback: "person.crop.circle",
                identifier: Identifier.profile,
                originalIndex: 4
            )
        case ItemKey.search:
            return makeSearchTab()
        default:
            return nil
        }
    }

    private func applyExplicitSelectionImages(selectedIdentifier: String?) {
        for tab in tabs {
            let identifier = tab.identifier
            guard let images = explicitImagesByIdentifier[identifier] else {
                continue
            }
            let selected = identifier == selectedIdentifier
            tab.image = selected ? (images.selected ?? images.normal) : images.normal
            if #available(iOS 26.1, *) {
                tab.selectedImage = images.selected ?? images.normal
            }
        }
        tabBar.setNeedsLayout()
    }

    private func applyBadge(_ value: String?, identifier: String) {
        guard let tab = tab(forIdentifier: identifier) else { return }
        let normalized = value?.isEmpty == false ? value : nil

        // Keep UIKit's native badge path. This is the same path that was
        // already stable for Messages in 1.0.52c; Friends now mirrors it.
        if tab.responds(to: NSSelectorFromString("setBadgeValue:")) {
            tab.setValue(normalized, forKey: "badgeValue")
        }
        // Regular tabs precede Search, so tabs[i] <-> items[i] holds unless
        // UIKit's More item replaced a slot; then rely on the UITab badge above.
        if let index = tabs.firstIndex(where: { $0 === tab }),
           let items = tabBar.items, index < items.count, !nativeBarUsesMoreRoute() {
            items[index].badgeValue = normalized
            items[index].badgeColor = .systemRed
        }
    }

    private func applyUnreadBadges() {
        applyBadge(secondBadgeValue, identifier: Identifier.second)
        applyBadge(messageBadgeValue, identifier: Identifier.messages)
        tabBar.setNeedsLayout()
    }

    @objc(updateMessageBadgeValue:)
    func updateMessageBadgeValue(_ value: String?) {
        guard messageBadgeValue != value else { return }
        messageBadgeValue = value
        applyUnreadBadges()
    }

    @objc(updateSecondBadgeValue:)
    func updateSecondBadgeValue(_ value: String?) {
        guard secondBadgeValue != value else { return }
        secondBadgeValue = value
        applyUnreadBadges()
    }

    private func applyCompactUnreadBadgeScale(in root: UIView) {
        for child in root.subviews {
            let className = String(describing: type(of: child)).lowercased()
            let hasDirectBadgeChild = child.subviews.contains {
                String(describing: type(of: $0)).lowercased().contains("badge")
            }
            if className.contains("badge"), !hasDirectBadgeChild,
               child.bounds.width > 4, child.bounds.width < 72,
               child.bounds.height > 4, child.bounds.height < 48 {
                // Scale only the final native badge visual, not its container.
                // This keeps the requested reduction near 10%, not ~19%.
                let target = CGAffineTransform(scaleX: 0.90, y: 0.90)
                if !child.transform.isApproximatelyEqual(to: target) { child.transform = target }
            }
            applyCompactUnreadBadgeScale(in: child)
        }
    }

    private func sanitizedVisibleItems(_ identifiers: [String]) -> [String] {
        var seen = Set<String>()
        var result: [String] = []
        for identifier in identifiers where ItemKey.allowed.contains(identifier) {
            guard seen.insert(identifier).inserted else { continue }
            result.append(identifier)
        }

        let selectableCount = result.filter {
            ItemKey.selectablePages.contains($0)
        }.count
        if selectableCount < 2 {
            return ItemKey.defaultVisible
        }

        // UISearchTab owns its independent right-side system capsule. Keep it
        // last regardless of the order of regular items so UIKit never tries
        // to place regular buttons on both sides of that capsule.
        if let searchIndex = result.firstIndex(of: ItemKey.search) {
            result.remove(at: searchIndex)
            result.append(ItemKey.search)
        }
        // No cap: all five regular buttons plus Search are requested. If the
        // live bar answers with UIKit's More route, Search yields instead of a
        // regular tab (see yieldSearchIfNativeBarOverflowed).
        return result
    }

    private var effectiveItemKeys: [String] {
        guard searchYieldsToRegularTabs,
              visibleItemIdentifiers.count > Self.compactRootTabCapacity else {
            return visibleItemIdentifiers
        }
        return visibleItemIdentifiers.filter { $0 != ItemKey.search }
    }

    /// True when the live UITabBar contains UIKit's More item, i.e. some root
    /// tabs (and their identifier mapping) were moved into the More list.
    private func nativeBarUsesMoreRoute() -> Bool {
        guard tabs.count > Self.compactRootTabCapacity,
              let items = tabBar.items, !items.isEmpty else { return false }
        let moreItem: UITabBarItem? = moreNavigationController.tabBarItem
        if let moreItem {
            if items.contains(where: { $0 === moreItem }) { return true }
            if let title = moreItem.title, !title.isEmpty,
               items.contains(where: { $0.title == title }) { return true }
        }
        return false
    }

    /// Read-only: UITabBarController selectors/ivars naming the More/max-item
    /// threshold on this OS build, logged once for a later native experiment.
    private func moreRouteRuntimeNames() -> String {
        var names: [String] = []
        func relevant(_ name: String) -> Bool {
            let lower = name.lowercased()
            return lower.contains("maxitem") || lower.contains("maximumnumber") ||
                lower.contains("maxnumber") || lower.contains("moretab") ||
                lower.contains("moreitem") || lower.contains("morenavigation")
        }
        var count: UInt32 = 0
        if let methods = class_copyMethodList(UITabBarController.self, &count) {
            for index in 0..<Int(count) {
                let name = NSStringFromSelector(method_getName(methods[index]))
                if relevant(name) { names.append(name) }
            }
            free(methods)
        }
        count = 0
        if let ivars = class_copyIvarList(UITabBarController.self, &count) {
            for index in 0..<Int(count) {
                guard let raw = ivar_getName(ivars[index]) else { continue }
                let name = String(cString: raw)
                if relevant(name) { names.append("ivar:" + name) }
            }
            free(ivars)
        }
        return names.prefix(32).joined(separator: ",")
    }

    /// Five regular tabs + UISearchTab are six root tabs. If UIKit put them
    /// on its More route (first four + More), keep all five regular tabs at
    /// native size and let Search yield (the top bar keeps its own Search),
    /// so no tab hides inside More and no index mapping shifts.
    @discardableResult
    private func yieldSearchIfNativeBarOverflowed() -> Bool {
        guard !searchYieldsToRegularTabs,
              tabs.contains(where: { $0 is UISearchTab }),
              nativeBarUsesMoreRoute() else { return false }
        searchYieldsToRegularTabs = true
        albumMorphProbe(
            "NATIVE_MORE_ROUTE tabs=\(tabs.count) items=\(tabBar.items?.count ?? 0) " +
            "action=search-yields keep=\(effectiveItemKeys.joined(separator: ",")) " +
            "runtime=\(moreRouteRuntimeNames())"
        )
        rebuildTabs(preservingOriginalIndex: cachedOriginalIndex)
        return true
    }

    /// Before tabs are set (UIKit decides the More route when they are).
    /// The private calls are made in Objective-C (ALB5BottomGeometry.m),
    /// where an assertion can be caught and a failed launch skips them once.
    private func raiseRootTabCapacity() {
        let state = ALB35RaiseRootTabCapacity(self, Self.raisedRootTabCapacity)
        if state != rootTabCapacityState {
            rootTabCapacityState = state
            albumMorphProbe("ROOT_TAB_CAPACITY \(state)")
        }
    }

    private func rebuildTabs(preservingOriginalIndex originalIndex: Int) {
        raiseRootTabCapacity()
        proxies.removeAll()
        explicitImagesByIdentifier.removeAll(keepingCapacity: true)

        let sourceKeys = effectiveItemKeys
        let rebuilt = sourceKeys.compactMap(tabForItemKey)
        guard !rebuilt.isEmpty else { return }

        let fallback = rebuilt.first { self.originalIndex(for: $0) != nil }
        let preferred = rebuilt.first { self.originalIndex(for: $0) == originalIndex } ?? fallback
        let resolvedIndex = self.originalIndex(for: preferred) ?? 0

        suppressCallbacks = true
        UIView.performWithoutAnimation {
            tabs = rebuilt
            selectedTab = preferred
            applyExplicitSelectionImages(selectedIdentifier: preferred?.identifier)
            tabBar.itemPositioning = .fill
            tabBar.itemWidth = 0
            tabBar.itemSpacing = 0
            tabBar.setNeedsLayout()
            tabBar.layoutIfNeeded()
        }
        cachedOriginalIndex = resolvedIndex
        pendingUserIndex = nil
        pendingSelectionUntil = 0
        suppressCallbacks = false
        applyStableVerticalOffset()
        applyUnreadBadges()
        albumMorphProbe(
            "VISIBLE_ITEMS_APPLIED items=\(sourceKeys.joined(separator: ",")) " +
            "nativeItems=\(tabBar.items?.count ?? 0) searchYields=\(searchYieldsToRegularTabs) " +
            "selected=\(resolvedIndex) tabFrame=\(tabBar.frame)"
        )
        if !yieldSearchIfNativeBarOverflowed(), tabs.count > Self.compactRootTabCapacity {
            // UIKit may publish its bar items on the next run-loop turn.
            DispatchQueue.main.async { [weak self] in
                _ = self?.yieldSearchIfNativeBarOverflowed()
            }
        }
    }

    private func tabForOriginalIndex(_ index: Int) -> UITab? {
        switch index {
        case 0:
            return tab(forIdentifier: Identifier.home)
        case 1:
            return tab(forIdentifier: Identifier.second)
        case 3:
            return tab(forIdentifier: Identifier.messages)
        case 4:
            return tab(forIdentifier: Identifier.profile)
        default:
            return nil
        }
    }

    private func originalIndex(for tab: UITab?) -> Int? {
        guard let identifier = tab?.identifier else { return nil }
        switch identifier {
        case Identifier.home:
            return 0
        case Identifier.second:
            return 1
        case Identifier.publish:
            return 2
        case Identifier.messages:
            return 3
        case Identifier.profile:
            return 4
        default:
            return nil
        }
    }

    private var activeProxy: AWEAlbumMorphProxyController? {
        selectedViewController as? AWEAlbumMorphProxyController
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        reproRecordController("bottom.controller.viewWillAppear")
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        reproRecordController("bottom.controller.viewWillDisappear")
    }

    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        reproRecordController("bottom.controller.viewDidDisappear")
    }

    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        reproRecordController("bottom.controller.viewDidAppear.before")
        // Safety net if UIKit only materialised its items once on screen.
        yieldSearchIfNativeBarOverflowed()
        prepareIndependentDriver()
        if nativeRequestOutstanding && !nativeRequestAccepted {
            enqueueNativeRequest(animated: true)
        }
        queueNativeRefresh()
        reproRecordController("bottom.controller.viewDidAppear.after")
    }

    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        guard !bottomLayoutInProgress else {
            if ALBReproIsRecording() { reproLayoutReentries += 1 }
            return
        }
        var reproBefore: [String: Any]?
        if ALBReproIsRecording() {
            let now = CACurrentMediaTime()
            if now - reproLastLayoutSample >= 0.1 {
                reproLastLayoutSample = now
                reproBefore = reproControllerSnapshot()
                reproLayoutReentries = 0
            }
        }
        bottomLayoutInProgress = true
        defer { bottomLayoutInProgress = false }

        applyStableVerticalOffset()
        setVisibleFraction(morphVisibleFraction, animated: false)
        applyCompactUnreadBadgeScale(in: tabBar)
        // Never disable Core Animation actions across super/native layout.
        ALB5UpdateBottomGeometry(tabBar, view, compactMode, false)
        queueNativeRefresh()
        if let reproBefore, ALBReproIsRecording() {
            var fields = reproControllerSnapshot(); fields["beforeOwnedLayout"] = reproBefore
            ALBReproRecordEvent("bottom.controller.layout.sample", fields)
        }
    }

    @objc private func ownProfileImageChanged(_ notification: Notification) {
        guard isViewLoaded else { return }
    }

    @objc private func nativeOptionsChanged(_ notification: Notification) {
        guard isViewLoaded else { return }
        reproRecordController("bottom.controller.optionsChanged.before")
        let style = ALB15CompactStyle()
        if style != lastConfiguredCompactStyle {
            compactMode = false
            nativeRequestOutstanding = false
            nativeRequestGeneration += 1
            ALB5CompactStateChanged(false)
            lastConfiguredCompactStyle = style
        }

        tabBar.setNeedsLayout()
        applyStableVerticalOffset()
        prepareIndependentDriver(force: true)
        queueNativeRefresh()
        reproRecordController("bottom.controller.optionsChanged.after")
    }

    private func applyStableVerticalOffset() {
        guard isViewLoaded else { return }
        // Bottom-bar scaling is removed: UIKit renders the bar at its native
        // size (scale exactly 1). The owned canvas only carries the user's
        // vertical offset; the controller root stays untransformed.
        guard let canvas = view.superview,
              canvas.accessibilityIdentifier == "AwemeLiquidBar.BottomScaleCanvas" else { return }
        let target = CGAffineTransform(translationX: 0, y: verticalOffset)
        guard !canvas.transform.isApproximatelyEqual(to: target) else { return }
        canvas.transform = target
    }

    @objc(setVerticalOffset:)
    func setVerticalOffset(_ offset: CGFloat) {
        let clamped = offset.isFinite ? min(max(offset, -24.0), 80.0) : 0
        // Compare against the stored offset, not the canvas transform.
        let changed = abs(verticalOffset - clamped) > 0.01
        verticalOffset = clamped
        applyStableVerticalOffset()
        guard changed else { return }
        if ALBReproIsRecording() {
            var fields = reproControllerSnapshot()
            fields["requestedOffset"] = Double(offset)
            ALBReproRecordEvent("bottom.controller.verticalOffset.changed", fields)
        }
        setVisibleFraction(morphVisibleFraction, animated: false)
        albumMorphProbe(
            "VERTICAL_OFFSET_DYNAMIC value=\(String(format: "%.2f", clamped)) " +
            "viewTY=\(view.transform.ty) tabFrame=\(tabBar.frame)"
        )
    }

    @objc(configureSecondTabAsShop:)
    func configureSecondTab(asShop shop: Bool) {
        secondIsShop = shop
        let newTitle = shop ? "商城" : "朋友"
        let newNormalSymbol = shop ? "storefront" : "hands.and.sparkles"
        let newImageRole = shop ? "shop" : "friends"
        guard newTitle != secondTitle || newImageRole != secondImageRole else {
            return
        }

        secondTitle = newTitle
        secondNormalSymbol = newNormalSymbol
        secondImageRole = newImageRole
        rebuildTabs(preservingOriginalIndex: cachedOriginalIndex)
    }

    @objc(reloadConfiguredImages)
    func reloadConfiguredImages() {
        rebuildTabs(preservingOriginalIndex: cachedOriginalIndex)
    }

    @objc(configureVisibleItemIdentifiers:)
    func configureVisibleItemIdentifiers(_ identifiers: [String]) {
        let sanitized = sanitizedVisibleItems(identifiers)
        guard sanitized != visibleItemIdentifiers else { return }

        let previousIndex = cachedOriginalIndex
        visibleItemIdentifiers = sanitized
        rebuildTabs(preservingOriginalIndex: previousIndex)

        if cachedOriginalIndex != previousIndex {
            let fallbackIndex = cachedOriginalIndex
            DispatchQueue.main.async { [weak self] in
                self?.actionSink?.albumMorphDidRequestOriginalIndex(fallbackIndex)
            }
        }
    }

    @objc(updateAccentColor:)
    func updateAccentColor(_ color: UIColor) {
        guard view.tintColor != color || tabBar.tintColor != color else { return }
        if view.tintColor != color { view.tintColor = color }
        if tabBar.tintColor != color { tabBar.tintColor = color }
    }

    @objc(setMorphBarVisible:animated:)
    func setMorphBarVisible(_ visible: Bool, animated: Bool) {
        // Keep the native Liquid Glass tree alive at all times. Visibility is
        // represented by a left-to-right mask, matching the proven 1.0.10b
        // return architecture instead of alpha/hide reconstruction.
        tabBar.isHidden = false
        view.isHidden = false
        view.alpha = 1.0
        applyStableVerticalOffset()
        setVisibleFraction(visible ? 1.0 : 0.0, animated: animated)
    }

    @objc(setVisibleFraction:animated:)
    func setVisibleFraction(_ fraction: CGFloat, animated: Bool) {
        let clamped = min(max(fraction, 0.0), 1.0)
        morphVisibleFraction = clamped
        ALB8SetBottomVisibleFraction(tabBar, clamped)

        if tabBar.isHidden { tabBar.isHidden = false }
        if view.isHidden { view.isHidden = false }
        if abs(view.alpha - 1.0) > 0.001 { view.alpha = 1.0 }

        let changes = {
            // The visible Liquid Glass platter is hosted by the controller's
            // floating-bar container, not strictly inside UITabBar. Mask the
            // whole resident controller view so steady hidden pages and
            // interactive returns reveal exactly the uncovered screen width.
            if self.tabBar.mask != nil { self.tabBar.mask = nil }
            if clamped >= 0.999 {
                if self.view.mask != nil { self.view.mask = nil }
            } else {
                let maskFrame = CGRect(
                    x: 0,
                    y: 0,
                    width: max(0, self.view.bounds.width * clamped),
                    height: self.view.bounds.height
                )
                if self.visibilityMaskView.frame != maskFrame {
                    self.visibilityMaskView.frame = maskFrame
                }
                if self.view.mask !== self.visibilityMaskView {
                    self.view.mask = self.visibilityMaskView
                }
            }
            let interactive = clamped >= 0.82
            if self.view.isUserInteractionEnabled != interactive {
                self.view.isUserInteractionEnabled = interactive
            }
            if self.tabBar.isUserInteractionEnabled != interactive {
                self.tabBar.isUserInteractionEnabled = interactive
            }
        }

        if animated {
            changes()
        } else {
            UIView.performWithoutAnimation(changes)
        }
    }

    private var isFeedPage: Bool {
        cachedOriginalIndex == 0 || (cachedOriginalIndex == 1 && !secondIsShop)
    }

    // Native collection/proxy relationship and shared bottom material are kept.
    // Feed paging/timer policy requests a state; UIKit alone animates that state.
    // Host scrolling sends intent without rebinding the host view hierarchy.
    private func prepareIndependentDriver(force: Bool = false) {
        // The root-pan observer supplies direction for both modes. UIKit still
        // renders native-left transitions, but never owns the app's scroll view.
        let nativeLeft = nativeMinimizeEnabled
        let behavior: UITabBarController.MinimizeBehavior = nativeLeft ? .onScrollDown : .never
        if tabBarMinimizeBehavior != behavior { tabBarMinimizeBehavior = behavior }
        if let page = boundPage { page.setContentScrollView(nil, for: .bottom) }
        boundDriver = nil
        boundPage = nil
        boundRootScroll = nil
    }

    private func queueNativeRefresh() {
        guard !nativeRefreshQueued, isViewLoaded, view.window != nil else { return }
        nativeRefreshQueued = true
        DispatchQueue.main.async { [weak self] in
            guard let self else { return }
            defer { self.nativeRefreshQueued = false }
            guard self.view.window != nil else { return }
            // Readback only. Layout must not bind observers or issue/retry a
            // minimized request, both of which can trigger another layout.
            self.observeNativeState()
        }
    }

    private func observeNativeState() {
        guard isViewLoaded, view.window != nil else { return }
        ALB5UpdateBottomGeometry(tabBar, view, compactMode, false)
        var state = ALB15NativeMinimizedState(self)
        if state < 0 { state = ALB15ObserveCompactGeometry(tabBar) }
        guard state >= 0 else { return }
        let actual = state != 0
        lastObservedNativeState = state
        if nativeRequestOutstanding && actual == requestedCompact { nativeRequestOutstanding = false }
        if !nativeRequestOutstanding { requestedCompact = actual }
        if compactMode != actual {
            compactMode = actual
            ALB5CompactStateChanged(actual)
            albumMorphProbe("NATIVE_OBSERVED minimized=\(actual) owner=UIKit")
        }
        ALB5UpdateBottomGeometry(tabBar, view, actual, false)
    }

    @objc(prepareMinimizeForGesture)
    func prepareMinimizeForGesture() { prepareIndependentDriver() }

    @objc(setNativeMinimizeEnabled:)
    func setNativeMinimizeEnabled(_ enabled: Bool) {
        nativeMinimizeEnabled = enabled
        prepareIndependentDriver()
    }

    @objc(setCompactMode:animated:)
    func setCompactMode(_ compact: Bool, animated: Bool) {
        guard Thread.isMainThread else { return }
        if ALBReproIsRecording() {
            let now = CACurrentMediaTime()
            if now - reproLastRequestSample >= 0.1 {
                reproLastRequestSample = now
                var fields = reproControllerSnapshot()
                fields["targetCompact"] = compact; fields["animated"] = animated
                fields["blockedByMinimizeOption"] = !nativeMinimizeEnabled && compact
                fields["duplicateOutstanding"] = nativeRequestOutstanding && compact == requestedCompact
                fields["duplicateCurrent"] = compact == compactMode && !nativeRequestOutstanding && lastObservedNativeState >= 0
                ALBReproRecordEvent("bottom.controller.compact.request", fields)
            }
        }
        if !nativeMinimizeEnabled && compact { return }
        // Deduplicate before UIKit is entered, not only after it has returned.
        if nativeRequestOutstanding && compact == requestedCompact { return }
        if compact == compactMode && !nativeRequestOutstanding && lastObservedNativeState >= 0 { return }
        requestedCompact = compact
        nativeRequestOutstanding = true
        nativeRequestAccepted = false
        nativeRequestGeneration += 1
        enqueueNativeRequest(animated: animated)
    }

    private func enqueueNativeRequest(animated: Bool) {
        guard !nativeRequestDispatchQueued else { return }
        nativeRequestDispatchQueued = true
        DispatchQueue.main.async { [weak self] in
            guard let self else { return }
            self.nativeRequestDispatchQueued = false
            self.issueNativeRequest(animated: animated)
        }
    }

    private func issueNativeRequest(animated: Bool) {
        guard !nativeRequestInFlight, isViewLoaded, view.window != nil,
              nativeRequestOutstanding else { return }
        // CATransaction completion can precede UIKit's own spring completion.
        // Keep both animated and nonanimated reversals behind the real motion.
        if nativeAnimationRunningSince != nil {
            nativeRequestWaitingForSettle = true
            return
        }
        nativeRequestInFlight = true
        defer { nativeRequestInFlight = false }
        nativeRequestWaitingForSettle = false
        prepareIndependentDriver()
        nativeAnimationRunningSince = CACurrentMediaTime()
        traceNativeScale("request")
        let accepted = AWEAlbumMorphRequestNativeMinimized(self, requestedCompact ? 1 : 0, animated ? 1 : 0) != 0
        nativeRequestAccepted = accepted
        if !accepted {
            nativeRequestOutstanding = false
            nativeAnimationRunningSince = nil
            return
        }
        observeNativeState()
        waitForNativeTransitionToSettle()
    }

    private func hasNativeGeometryAnimation() -> Bool {
        func changesGeometry(_ animation: CAAnimation) -> Bool {
            if let group = animation as? CAAnimationGroup {
                return (group.animations ?? []).contains(where: changesGeometry)
            }
            guard let property = animation as? CAPropertyAnimation,
                  let key = property.keyPath else { return false }
            return key.hasPrefix("bounds") || key.hasPrefix("position") || key.hasPrefix("transform")
        }
        var layers = [tabBar.layer]
        var index = 0
        while index < layers.count && index < 128 {
            let layer = layers[index]; index += 1
            if (layer.animationKeys() ?? []).contains(where: { key in
                layer.animation(forKey: key).map(changesGeometry) ?? false
            }) { return true }
            layers.append(contentsOf: layer.sublayers ?? [])
        }
        return false
    }

    private func waitForNativeTransitionToSettle() {
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.06) { [weak self] in
            guard let self, let started = self.nativeAnimationRunningSince else { return }
            let elapsed = CACurrentMediaTime() - started
            if self.isViewLoaded && self.view.window != nil &&
                ALBShouldWaitForNativeSettle(elapsed, self.hasNativeGeometryAnimation()) {
                self.waitForNativeTransitionToSettle()
                return
            }
            self.nativeAnimationRunningSince = nil
            self.observeNativeState()
            self.traceNativeScale(elapsed >= 1.8 ? "native-settle-timeout" : "native-settled")
            // No forced layout at completion: it snapped native spring geometry
            // back to the scaled model, producing an overshrink/recovery frame.
            if self.nativeRequestWaitingForSettle {
                self.nativeRequestWaitingForSettle = false
                guard self.nativeRequestOutstanding else { return }
                self.enqueueNativeRequest(animated: true)
            } else {
                self.nativeRequestOutstanding = false
            }
        }
    }

    private func traceNativeScale(_ phase: String) {
        phase.withCString { ALB49TraceNativeScale(tabBar, $0) }
    }

    @objc(nativeCollapseDiagnostics)
    func nativeCollapseDiagnostics() -> String {
        let source = sourceScrollView.map { String(describing: type(of: $0)) } ?? "未绑定"
        return "模式=原生左侧小球；驱动=共用方向手势；源=\(source)；实际状态=\(lastObservedNativeState)；UIKit原生动画；实际动画结束判定；原生尺寸（无缩放）；不绑定宿主列表"
    }

    /// 3.3: why five buttons and Search do not fit (UIKit's More route) and
    /// the UITabBarController names of that limit, for the 诊断 copy.
    @objc(nativeMoreRouteDiagnostics)
    func nativeMoreRouteDiagnostics() -> String {
        let more = nativeBarUsesMoreRoute() ? "是" : "否"
        let yields = searchYieldsToRegularTabs ? "是" : "否"
        let items = tabBar.items?.count ?? 0
        return "底栏 根标签=\(tabs.count) 原生项=\(items) 更多路由=\(more) 搜索让位=\(yields) 上限=\(rootTabCapacityState)"
    }

    @objc(isNativeAvatarCompact)
    func isNativeAvatarCompact() -> Bool { avatarArtworkActive }

    @objc(currentOriginalIndex)
    func currentOriginalIndex() -> Int { cachedOriginalIndex }

    @objc(bindSourceScrollView:)
    func bindSourceScrollView(_ scrollView: UIScrollView) {
        guard let sourceWindow = scrollView.window, let chromeWindow = viewIfLoaded?.window,
              sourceWindow === chromeWindow || (sourceWindow.windowScene != nil &&
                 sourceWindow.windowScene === chromeWindow.windowScene) else { return }
        let changed = sourceScrollView !== scrollView
        sourceScrollView = scrollView
        if changed { queueSourceBinding() }
    }

    private var sourceBindingQueued = false
    private func queueSourceBinding() {
        guard !sourceBindingQueued else { return }
        sourceBindingQueued = true
        DispatchQueue.main.async { [weak self] in
            guard let self else { return }
            self.sourceBindingQueued = false
            self.prepareIndependentDriver()
        }
    }

    private func reassertSelection(
        originalIndex: Int,
        after delay: TimeInterval = 0
    ) {
        let apply = { [weak self] in
            guard let self,
                  self.pendingUserIndex == originalIndex,
                  let target = self.tabForOriginalIndex(originalIndex) else {
                return
            }

            self.cachedOriginalIndex = originalIndex
            guard self.selectedTab !== target else { return }
            self.suppressCallbacks = true
            self.selectedTab = target
            self.applyExplicitSelectionImages(selectedIdentifier: target.identifier)
            self.suppressCallbacks = false
        }

        if delay <= 0 {
            apply()
        } else {
            DispatchQueue.main.asyncAfter(deadline: .now() + delay) {
                apply()
            }
        }
    }

    @objc(syncSelectedOriginalIndex:)
    func syncSelectedOriginalIndex(_ index: Int) {
        // Once the user has selected a tab on the transplanted native bar, it
        // becomes the authoritative selection source. Douyin's hidden original
        // tab controller can continue publishing a stale Messages index long
        // after the Profile page is already visible.
        if hasUserDrivenSelection && index != cachedOriginalIndex {
            return
        }

        let now = CACurrentMediaTime()

        if let pending = pendingUserIndex {
            if now < pendingSelectionUntil {
                if index != pending {
                    // Douyin can emit an intermediate Messages selection while
                    // the Profile controller is already becoming visible. Keep
                    // the user's native selection authoritative for the full
                    // transition window, even if the correct index appeared
                    // briefly before another stale callback.
                    reassertSelection(originalIndex: pending)
                    return
                }
            } else {
                pendingUserIndex = nil
                pendingSelectionUntil = 0
            }
        }

        guard let target = tabForOriginalIndex(index) else { return }
        cachedOriginalIndex = index

        guard selectedTab !== target else { return }
        sourceScrollView = nil
        boundRootScroll = nil
        boundPage = nil
        driverGeneration += 1
        suppressCallbacks = true
        selectedTab = target
        applyExplicitSelectionImages(selectedIdentifier: target.identifier)
        suppressCallbacks = false
    }

    @objc(mirrorSourceScrollOffset:)
    func mirrorSourceScrollOffset(_ delta: CGFloat) {
        _ = delta
    }

    func tabBarController(
        _ tabBarController: UITabBarController,
        shouldSelectTab tab: UITab
    ) -> Bool {
        if ALBReproIsRecording() {
            var fields = reproControllerSnapshot()
            fields["tabIdentifier"] = tab.identifier
            fields["isSearch"] = tab is UISearchTab
            fields["suppressCallbacks"] = suppressCallbacks
            ALBReproRecordEvent("bottom.controller.shouldSelectTab", fields)
        }
        guard !suppressCallbacks else { return true }
        if tab is UISearchTab {
            actionSink?.albumMorphDidRequestSearch()
            return false
        }
        if originalIndex(for: tab) == 0 && selectedTab === tab && cachedOriginalIndex == 0 {
            // Minimized, this is the ball itself: never veto UIKit's own
            // expansion. The Douyin Home re-tap stays blocked by the
            // coordinator (HOME_RESELECT_BLOCKED).
            return compactMode
        }
        if originalIndex(for: tab) == 2 {
            actionSink?.albumMorphDidRequestOriginalIndex(2)
            return false
        }
        return true
    }

    func tabBarController(
        _ tabBarController: UITabBarController,
        didSelectTab selectedTab: UITab,
        previousTab: UITab?
    ) {
        guard !suppressCallbacks else { return }
        guard let originalIndex = originalIndex(for: selectedTab) else {
            return
        }
        // The minimized Home ball re-selects Home only to let UIKit expand
        // the bar: keep the scroll binding and send Douyin nothing.
        if originalIndex == 0, let previousTab, previousTab === selectedTab {
            albumMorphProbe("TAB_RESELECT_HOME_BALL expand-only")
            return
        }

        cachedOriginalIndex = originalIndex
        sourceScrollView = nil
        prepareIndependentDriver(force: true)
        applyExplicitSelectionImages(selectedIdentifier: selectedTab.identifier)
        albumMorphProbe(
            "TAB_SELECT originalIndex=\(originalIndex) previous=\(previousTab?.identifier ?? "nil")"
        )
        hasUserDrivenSelection = true
        pendingUserIndex = originalIndex
        pendingSelectionUntil = CACurrentMediaTime() + 3.0

        // Forward the real Douyin action, then keep the system platter on the
        // user's chosen tab while Douyin emits its transient old-index
        // callbacks during the page transition.
        actionSink?.albumMorphDidRequestOriginalIndex(originalIndex)
        reassertSelection(originalIndex: originalIndex)
        reassertSelection(originalIndex: originalIndex, after: 0.12)
        reassertSelection(originalIndex: originalIndex, after: 0.45)
        reassertSelection(originalIndex: originalIndex, after: 1.10)
        reassertSelection(originalIndex: originalIndex, after: 2.40)

    }
}
