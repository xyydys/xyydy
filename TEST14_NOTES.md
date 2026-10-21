# TEST14 focused changes

- Compact avatar keeps the resident native auxiliary/search glass. The expansion tap recognizer no longer cancels UIKit touches, so the native press/stretch/highlight sequence can run before expansion. No second compact glass button is created.
- The old left platter is only part of the collapse morph and is hidden immediately after the morph settles, avoiding a second refracting glass layer under the avatar ball.
- Top left/right buttons now use UIKit regular interactive glass (`glassButtonConfiguration`) in the same top trait/material context as the native top category bar; playback/right-side controls remain Clear.
- No right-rail/layout or feed-collapse trigger logic was changed in this focused build.
