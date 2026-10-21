/* TEST33: collapse observes the app's pan without registering its collection
   as contentScrollView of another tab controller. That registration lets
   UIKit adjust a foreign list when a proxy tab expands (Favorites jump-top).
   Native animation requests use the resident coordinator endpoint instead. */
