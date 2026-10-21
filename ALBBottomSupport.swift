import UIKit
import QuartzCore

// Recorder-only scalar snapshots. Callers gate before invoking these helpers;
// they never walk a subtree, read artwork, or write to disk from an animation.
@available(iOS 26.0, *)
func ALBReproBottomRect(_ rect: CGRect) -> [Double] {
    [Double(rect.minX), Double(rect.minY), Double(rect.width), Double(rect.height)]
}

@available(iOS 26.0, *)
@MainActor
func ALBReproBottomViewState(_ view: UIView?, in host: UIView? = nil) -> [String: Any] {
    guard let view else { return ["present": false] }
    let transform = view.transform
    var state: [String: Any] = [
        "present": true, "class": NSStringFromClass(type(of: view)),
        "id": String(describing: ObjectIdentifier(view)),
        "bounds": ALBReproBottomRect(view.bounds), "frameInParent": ALBReproBottomRect(view.frame),
        "centerInParent": [Double(view.center.x), Double(view.center.y)],
        "transform": [Double(transform.a), Double(transform.b), Double(transform.c), Double(transform.d),
                      Double(transform.tx), Double(transform.ty)],
        "alpha": Double(view.alpha), "hidden": view.isHidden,
        "interactionEnabled": view.isUserInteractionEnabled,
        "layerOpacity": Double(view.layer.opacity), "clipsToBounds": view.clipsToBounds,
        "layerAnimationKeys": Array((view.layer.animationKeys() ?? []).prefix(12)),
        "safeArea": [Double(view.safeAreaInsets.top), Double(view.safeAreaInsets.left),
                     Double(view.safeAreaInsets.bottom), Double(view.safeAreaInsets.right)]
    ]
    if let parent = view.superview {
        state["parentClass"] = NSStringFromClass(type(of: parent))
        state["parentID"] = String(describing: ObjectIdentifier(parent))
    }
    if let window = view.window { state["windowID"] = String(describing: ObjectIdentifier(window)) }
    if let host, view === host || view.isDescendant(of: host) || (view.window != nil && view.window === host.window) {
        state["rectInHost"] = ALBReproBottomRect(view.convert(view.bounds, to: host))
        if let presentation = view.layer.presentation(), let parent = view.superview {
            state["presentationFrameInParent"] = ALBReproBottomRect(presentation.frame)
            state["presentationRectUsingModelParent"] = ALBReproBottomRect(parent.convert(presentation.frame, to: host))
            state["presentationOpacity"] = Double(presentation.opacity)
        }
    }
    return state
}


// Explicit preference only. The default path returns without traversing or
// writing any material. A mode switch is applied while expanded, never per tick.
// Shared material writes live in ALBUnifiedMaterial.m.
