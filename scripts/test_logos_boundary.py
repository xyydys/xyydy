#!/usr/bin/env python3
"""Regression for TEST19's specific %orig line-tail failure.

Extracts the production layout method and its following method, then models only
upstream Logos' %orig end-of-line replacement and function-signature generation.
On macOS/Xcode CI it explicitly uses the iPhoneOS SDK for iOS-target syntax checks;
on non-Apple development hosts it falls back to an SDK-free host syntax check.
CI separately runs real logos.pl over all three sources and checks its output.
"""
from __future__ import annotations

import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from check_logos_boundaries import mask_literals, orig_issues, structure_issues

ROOT = Path(__file__).resolve().parents[1]

PREAMBLE = r'''
typedef signed char BOOL;
typedef long NSInteger;
#define YES ((BOOL)1)
#define nil ((id)0)
#define OBJC_ASSOCIATION_RETAIN_NONATOMIC 1
__attribute__((objc_root_class)) @interface NSObject @end
@interface NSNumber : NSObject
+ (NSNumber *)numberWithBool:(BOOL)value;
- (BOOL)boolValue;
@end
@interface UIView : NSObject @end
@interface AWEElementStackView : UIView @end
@interface UITraitCollection : NSObject @end
@interface UIApplication : NSObject
+ (instancetype)sharedApplication;
@property(readonly) NSInteger applicationState;
@end
enum { UIApplicationStateActive = 0 };
id objc_getAssociatedObject(id object, const void *key);
void objc_setAssociatedObject(id object, const void *key, id value, int policy);
void nativeLayout(AWEElementStackView *self, SEL _cmd);
void nativeTrait(AWEElementStackView *self, SEL _cmd, UITraitCollection *previousTraitCollection);
void ALB6BeginRightGeometry(UIView *stack);
void ALB6EndRightGeometry(UIView *stack);
void AWEInternalFSHandleElementStackLayout(UIView *stack);
void AWERefreshSameStyleGlass(UIView *stack);
void AWELayoutBoundHomeActionSlots(UIView *stack);
BOOL AWEStableCleanShouldSuppressElementStack(UIView *stack);
void AWEStableCleanSuppressRightStack(UIView *stack);
void AWERefreshHomeActionStack(UIView *stack);
static UIView *gAWEStableCleanScreenStack;
'''


def extract_body(source: str, signature: str) -> str:
    hook = source.index('%hook AWEElementStackView')
    start = source.index('{', source.index(signature, hook))
    masked = mask_literals(source)
    depth = 0
    for pos in range(start, len(masked)):
        depth += (masked[pos] == '{') - (masked[pos] == '}')
        if depth == 0:
            return source[start:pos + 1]
    raise RuntimeError('unclosed production method: ' + signature)


def line_tail_model(body: str, replacement: str) -> str:
    """Model only the current range(%orig_start, length(line)) patch behavior."""
    result = []
    for line, masked in zip(body.splitlines(keepends=True),
                            mask_literals(body).splitlines(keepends=True)):
        pos = masked.find('%orig')
        if pos >= 0:
            line = line[:pos] + replacement + ';' + ('\n' if line.endswith('\n') else '')
        result.append(line)
    return ''.join(result)


def fixture(layout: str, trait: str) -> str:
    return (PREAMBLE + '\nstatic void '
            '_logos_method$_ungrouped$AWEElementStackView$layoutSubviews('
            'AWEElementStackView *self, SEL _cmd) '
            + line_tail_model(layout, 'nativeLayout(self, _cmd)')
            + '\n\nstatic void '
            '_logos_method$_ungrouped$AWEElementStackView$traitCollectionDidChange$('
            'AWEElementStackView *self, SEL _cmd, UITraitCollection *previousTraitCollection) '
            + line_tail_model(trait, 'nativeTrait(self, _cmd, previousTraitCollection)')
            + '\n')


def resolve_compiler():
    """Return (command_prefix, sdk_path, mode).

    Apple CI must not pair an iOS target triple with Clang's default macOS
    sysroot.  Prefer the active Xcode iPhoneOS SDK whenever xcrun is present.
    The host fallback exists only so the structural regression can still run on
    non-Apple development machines; it does not claim to be an iOS SDK compile.
    """
    xcrun = shutil.which('xcrun')
    if xcrun:
        sdk = subprocess.run(
            [xcrun, '--sdk', 'iphoneos', '--show-sdk-path'],
            capture_output=True, text=True, timeout=15, check=False)
        clang = subprocess.run(
            [xcrun, '--sdk', 'iphoneos', '--find', 'clang'],
            capture_output=True, text=True, timeout=15, check=False)
        if sdk.returncode == 0 and clang.returncode == 0:
            sdk_path = sdk.stdout.strip()
            clang_path = clang.stdout.strip()
            if sdk_path and clang_path:
                return [clang_path], sdk_path, 'iphoneos'
    if os.environ.get('ALB_REQUIRE_IPHONEOS') == '1':
        raise SystemExit('iPhoneOS SDK/Clang resolution failed while ALB_REQUIRE_IPHONEOS=1')
    compiler = shutil.which('clang')
    if not compiler:
        raise SystemExit('clang is required for the Logos boundary regression')
    return [compiler], None, 'host'


def main() -> int:
    compiler_prefix, sdk_path, compile_mode = resolve_compiler()
    source = (ROOT / 'Tweak.x').read_text(encoding='utf-8')
    layout = extract_body(source, '- (void)layoutSubviews')
    trait = extract_body(source, '- (void)traitCollectionDidChange:')
    fixed = ('     [objc_getAssociatedObject(alb6Stack, &layoutInProgressKey) boolValue]) {\n'
             '     %orig;\n     return;\n }')
    old = '     [objc_getAssociatedObject(alb6Stack, &layoutInProgressKey) boolValue]) { %orig; return; }'
    if layout.count(fixed) != 1:
        raise SystemExit('expected the repaired production guard exactly once')
    legacy = layout.replace(fixed, old)
    # No executable token may be added/removed by this source-formatting repair.
    if ''.join(mask_literals(layout).split()) != ''.join(mask_literals(legacy).split()):
        raise SystemExit('repair changed production tokens beyond line boundaries')
    if orig_issues(layout)[1] or not orig_issues(legacy)[1]:
        raise SystemExit('source regression gate does not distinguish TEST19 from TEST20')
    good, bad = fixture(layout, trait), fixture(legacy, trait)
    count, errors = structure_issues(good, generated=True)
    _, bad_errors = structure_issues(bad, generated=True)
    if count != 2 or errors or not any('is nested' in e for e in bad_errors):
        raise SystemExit('generated-function boundary negative control failed')
    results = []
    with tempfile.TemporaryDirectory(prefix='alb-logos-boundary-') as folder:
        folder = Path(folder)
        for arch in ('arm64', 'arm64e'):
            for name, text, expected_success in (('fixed', good, True), ('test19_negative', bad, False)):
                path = folder / (arch + '_' + name + '.m')
                path.write_text(text, encoding='utf-8')
                if compile_mode == 'iphoneos':
                    command = compiler_prefix + [
                        '-target', f'{arch}-apple-ios15.0',
                        '-isysroot', sdk_path,
                        '-x', 'objective-c', '-fobjc-arc', '-fobjc-exceptions',
                        '-fblocks', '-fsyntax-only', '-Wall', '-Wextra', '-Werror',
                        '-Wno-unused-function', '-Wno-unused-variable', str(path)]
                else:
                    # Architecture is irrelevant to the brace/function-boundary
                    # property under test. Keep two labeled passes for parity with
                    # CI while compiling against the host toolchain only.
                    command = compiler_prefix + [
                        '-x', 'objective-c', '-fobjc-exceptions', '-fblocks',
                        '-fsyntax-only', '-Wall', '-Wextra', '-Werror',
                        '-Wno-unused-function', '-Wno-unused-variable', str(path)]
                result = subprocess.run(command, capture_output=True, text=True,
                                        timeout=30, check=False)
                passed = (result.returncode == 0) == expected_success
                if not expected_success:
                    passed = passed and 'function definition is not allowed here' in result.stderr
                results.append({'arch': arch, 'case': name,
                                'expected_success': expected_success,
                                'compiler_exit_code': result.returncode,
                                'passed': passed, 'stderr': result.stderr})
    print(json.dumps({'scope': 'production two-method extraction + upstream line-tail model',
                      'real_logos_execution': False,
                      'compile_mode': compile_mode,
                      'apple_sdk_compile': compile_mode == 'iphoneos',
                      'sdk_path': sdk_path,
                      'runtime_test': False, 'results': results}, indent=2))
    return 0 if all(r['passed'] for r in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
