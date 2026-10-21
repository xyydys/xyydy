#pragma once
// Portable ABI policy shared by production endpoint resolution and its C tests.
#include "ALBNativeState.h"
static inline const char *ALB22UnqualifiedType(const char *type) {
    if (!type) return "";
    while (*type && strchr("rnNoORV", *type)) ++type;
    return type;
}
static inline bool ALB22BoolSetterSignature(unsigned long arguments,
    const char *result, const char *value, const char *animated) {
    return (arguments == 3 || arguments == 4) &&
        strcmp(ALB22UnqualifiedType(result), "v") == 0 && ALB15EncodingIsBool(value) &&
        (arguments == 3 || ALB15EncodingIsBool(animated));
}
static inline bool ALB22ObjectType(const char *type) {
    type = ALB22UnqualifiedType(type);
    return type[0] == '@' && (type[1] == '\0' || type[1] == '"');
}
static inline bool ALB22NativeBarClassName(const char *name) {
    if (!name) return false;
    return strncmp(name, "_UITabBar", 9) == 0 || strncmp(name, "UITabBar", 8) == 0 ||
        strncmp(name, "UIKit._UITabBar", 15) == 0 ||
        strncmp(name, "UIKit._UIBarMinimiz", 19) == 0 ||
        strncmp(name, "_UIBarMinimiz", 13) == 0 || strcmp(name, "UIBarMinimization") == 0 ||
        (strncmp(name, "_TtC5UIKit", 10) == 0 &&
         (strstr(name, "TabBar") != NULL || strstr(name, "BarMinimiz") != NULL));
}

// TEST23: inspect the actual Objective-C Method encoding, not an object's
// optional reflection/forwarding implementation. Full encodings include stack
// sizes and argument offsets (e.g. v20@0:8B16); never compare them to "B".
typedef struct {
    unsigned arguments;
    char result;
    char value;
    char animated;
} ALB23NativeBoolABI;

static inline const char *ALB23SkipEncodingOffset(const char *p) {
    if (*p == '+' || *p == '-') {
        ++p;
        if (*p < '0' || *p > '9') return NULL;
    }
    while (*p >= '0' && *p <= '9') ++p;
    return p;
}
static inline bool ALB23ReadEncodingToken(const char **cursor, char *token) {
    if (!cursor || !*cursor) return false;
    const char *p = ALB22UnqualifiedType(*cursor);
    if (!*p || !strchr("vBc@:", *p)) return false;
    *token = *p++;
    p = ALB23SkipEncodingOffset(p);
    if (!p) return false;
    *cursor = p;
    return true;
}
static inline bool ALB23ParseNativeBoolABI(const char *encoding,
    unsigned arguments, bool setter, ALB23NativeBoolABI *output) {
    ALB23NativeBoolABI abi = {0, 0, 0, 0};
    if (!encoding || !output ||
        (setter ? (arguments != 3 && arguments != 4) : arguments != 2)) return false;
    const char *p = encoding;
    char receiver = 0, selector = 0;
    if (!ALB23ReadEncodingToken(&p, &abi.result) ||
        !ALB23ReadEncodingToken(&p, &receiver) || receiver != '@' ||
        !ALB23ReadEncodingToken(&p, &selector) || selector != ':') return false;
    if (setter) {
        if (abi.result != 'v' || !ALB23ReadEncodingToken(&p, &abi.value) ||
            (abi.value != 'B' && abi.value != 'c')) return false;
        if (arguments == 4 && (!ALB23ReadEncodingToken(&p, &abi.animated) ||
            (abi.animated != 'B' && abi.animated != 'c'))) return false;
    } else if (abi.result != 'B' && abi.result != 'c') return false;
    if (*p != '\0') return false; // reject pointers, blocks, structs and extra arguments
    abi.arguments = arguments;
    *output = abi;
    return true;
}
