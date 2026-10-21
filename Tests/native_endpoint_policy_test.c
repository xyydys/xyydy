#include <assert.h>
#include <stdio.h>
#include "../ALB22NativeMethodPolicy.h"
int main(void) {
    const char *ok[] = {"B", "c", "rB", "nc"};
    const char *bad[] = {"", "i", "q", "Q", "d", "@", "@?", "^B", "B8", NULL};
    unsigned count = 0;
    for (unsigned i=0;i<sizeof(ok)/sizeof(*ok);i++) {
        assert(ALB22BoolSetterSignature(3,"v",ok[i],NULL));count++;
        for(unsigned j=0;j<sizeof(ok)/sizeof(*ok);j++) {
            assert(ALB22BoolSetterSignature(4,"v",ok[i],ok[j]));count++;
        }
    }
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++) {
        assert(!ALB22BoolSetterSignature(3,"v",bad[i],NULL));
        assert(!ALB22BoolSetterSignature(4,"v","B",bad[i]));
        assert(!ALB22BoolSetterSignature(4,bad[i],"B","B"));count+=3;
    }
    assert(!ALB22BoolSetterSignature(2,"v","B",NULL));
    assert(!ALB22BoolSetterSignature(5,"v","B","B"));
    assert(ALB22ObjectType("@")); assert(ALB22ObjectType("r@\"NSObject\""));
    assert(!ALB22ObjectType("@?")); assert(!ALB22ObjectType("^@"));
    assert(!ALB22ObjectType(NULL));assert(!ALB22ObjectType(""));count+=8;
    const char *names[]={"UITabBar", "UITabBarController", "_UITabBarVisualProviderModernIOS",
       "_UIBarMinimizationController", "UIBarMinimization", "_TtC5UIKit16UITabBarProvider"};
    for(unsigned i=0;i<sizeof(names)/sizeof(*names);i++){assert(ALB22NativeBarClassName(names[i]));count++;}
    const char *other[]={"AWEElementStackView", "_UINavigationController", "UIScrollView", "UIView",
       "_TtC5UIKit18NavigationProvider", "", NULL};
    for(unsigned i=0;i<sizeof(other)/sizeof(*other);i++){assert(!ALB22NativeBarClassName(other[i]));count++;}
    printf("Native endpoint policy: %u assertions passed (ABI/name filter only; not UIKit execution).\n",count);
    return 0;
}
