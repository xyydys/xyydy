#!/usr/bin/env python3
"""Run extracted production hash/allowlist code; inspect release integration.

No UIKit runtime or device claim. Account names are not stored in the release.
"""
from pathlib import Path
import hashlib, json, re, shutil, struct, subprocess, tempfile
from check_logos_boundaries import mask_literals

ROOT = Path(__file__).resolve().parents[1]
AUDIT = json.loads((ROOT/'validation/auth51_changes.json').read_text())
SOURCE = (ROOT/'Tweak.x').read_text()

def need(condition, message):
    if not condition:
        raise AssertionError('auth51: '+message)

def function_body(source, signature):
    # Match a definition, avoiding the forward declaration.
    match = re.search(re.escape(signature)+r'[^;]*?\{', source)
    need(match is not None, 'missing function '+signature)
    start = source.index('{', match.start())
    masked = mask_literals(source)
    depth = 0
    for end in range(start, len(source)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if not depth:
            return source[start+1:end]
    raise AssertionError(signature)

def table(source, name):
    match = re.search(re.escape(name)+r'\[[^\]]+\] = \{(.*?)\n\s*\};', source, re.S)
    need(match is not None, 'missing table '+name)
    rows = re.findall(r'\{ \{([^{}]*)\}, \{([^{}]*)\} \}', match.group(1))
    pairs = [tuple(bytes(int(v,16) for v in re.findall(r'0x[\dA-Fa-f]+', part))
                   for part in row) for row in rows]
    need(all(len(a)==len(b)==32 for a,b in pairs), 'invalid masked digest length')
    return pairs

def canonical_authorization_logic(source):
    core = source[source.index('#pragma mark - Local account authorization gate'):
                  source.index('#pragma mark - Transition diagnostics')]
    for name in ['kAWEAuthorizationDigests','kAWEAuthorizationDigestTags','allowed']:
        core = re.sub(re.escape(name)+r'\[[^\]]+\] = \{.*?\n\s*\};',
                      name+'[AUTH_COUNT] = { AUTH_TABLE };',core,flags=re.S)
    core = re.sub(r'entry < (?:2|sizeof\(allowed\) / sizeof\(allowed\[0\]\))',
                  'entry < AUTH_SPECIAL_COUNT',core)
    core = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',
                  lambda m: '' if m[0].startswith('/') else m[0],core)
    return hashlib.sha256(re.sub(r'\s+',' ',core).strip().encode()).hexdigest()

normal_pairs = table(SOURCE,'kAWEAuthorizationDigests')
special_body = function_body(SOURCE,'static BOOL AWECustomBottomBarReleaseAllowed(')
special_pairs = table(special_body,'allowed')
decode = lambda pairs: [bytes(a^b for a,b in zip(mask,value)) for mask,value in pairs]
normal, special = decode(normal_pairs), decode(special_pairs)
need(len(normal)==51 and len(set(normal))==51, 'normal count/uniqueness')
need(len(special)==3 and len(set(special))==3, 'special count/uniqueness')
pair_hash = lambda pairs: hashlib.sha256(b''.join(a+b for a,b in pairs)).hexdigest()
need(pair_hash(normal_pairs[:50])==AUDIT['preserved_normal_pairs_sha256'],
     'a prior normal authorization changed')
need(pair_hash(special_pairs[:2])==AUDIT['preserved_special_pairs_sha256'],
     'a prior special entitlement changed')
need(normal[-1].hex()==AUDIT['ordinary_added_digest'], 'wrong new ordinary digest')
need(normal[28].hex()==AUDIT['promoted_existing_digest']==special[-1].hex(),
     'existing row 29 was not promoted')
need(special[:2]==[normal[36],normal[21]], 'old special rows were not preserved')
need(normal[-1] not in special and set(special)<=set(normal), 'permission leakage')
need(canonical_authorization_logic(SOURCE)==AUDIT['authorization_logic_sha256'],
     'authorization normalization/proof/binding logic changed')
for name, wanted in AUDIT['rollback_runtime_sha256'].items():
    need(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==wanted,
         '2.3 runtime file changed: '+name)
need(not (ROOT/'ALB26NativeMaterial.m').exists() and
     not (ROOT/'ALB26NativeMaterial.h').exists(), 'rejected 2.4 implementation returned')

def check_release(source):
    display = function_body(source,'static BOOL AWEBottomBarDisplayEnabled(')
    need(display.index('if (!AWECustomBottomBarReleaseAllowed()) return YES;') <
         display.index('objectForKey:'), 'ordinary user can disable bottom ownership')
    special_gate = function_body(source,'static BOOL AWECustomBottomBarReleaseAllowed(')
    need(special_gate.index('if (!AWEAuthorizationValid()) return NO;') <
         special_gate.index('static const AWEAuthorizationDigestPair allowed'),
         'special permission bypasses device-bound authorization')
    settings = source[source.index('NSMutableArray<UIView *> *bottomItems ='):]
    need(settings.index('if (AWECustomBottomBarReleaseAllowed()) {') <
         settings.index('action:@selector(bottomBarDisplaySwitchChanged:)'),
         'special switch is exposed to ordinary users')
    for name in ['AWEForceOriginalBottomBackdropClear','AWEClearOriginalTabBarBackdropTree',
                 'AWEBottomV16ClearShopBottomCovers','AWEBottomV17ClearShopSystemChrome',
                 'AWEBottomV18ClearShopTabBarCarrierTree','AWEBottomV22ClearShopNativeBarFast',
                 'AWEBottomV21PrepareShop','ALB24ClearNativeBottomBackdrops',
                 'AWEFSHideTabBackgrounds','AWEStartBarVisibilityDriver',
                 'AWEUpdateNativeBarVisibility']:
        match = re.search(r'^static [^\n]*\b'+name+r'\(',source,re.M)
        need(match is not None, 'missing release path '+name)
        body = function_body(source,match[0])
        need('!AWEBottomBarDisplayEnabled()' in body.split('return',1)[0],
             'OFF is ignored by '+name)
    attach = function_body(source,'static void AWEAttachNativeBarWithForce(')
    need(attach.index('if (!AWEBottomBarDisplayEnabled()) {') <
         attach.index('AWEInstallOrUpdateAlbumMorphBar'), 'legacy fallback owns released host')
    handler = function_body(source,'- (void)bottomBarDisplaySwitchChanged:')
    for token in ['AWEUninstallAlbumMorphBar();','[driver.displayLink invalidate];',
                  'AWERestoreBottomNativeVisualStates();','AWERestoreOriginalTabBarContents',
                  'AWEExtendRouteContentBehindBottomBar','AWEAttachNativeBarWithForce']:
        need(token in handler, 'switch misses '+token)
    uninstall = function_body(source,'static void AWEUninstallAlbumMorphBar(')
    for token in ['ALBRemoveRootPanObservers();','AWERestoreSharedSearchGestureToOriginal',
                  'AWERestoreAlbumMorphSearchGesture();','AWEIOS27DestroySourceBackdropMirror();',
                  'gAWEAlbumMorphOverlayWindow = nil;','gAWEAlbumMorphController = nil;']:
        need(token in uninstall,'teardown misses '+token)
    native = source[source.index('%hook AWENormalModeTabBar\n'):]
    color = function_body(native,'- (void)setBackgroundColor:')
    need('!AWEBottomBarDisplayEnabled()' in color.split('%orig(color)',1)[0],
         'native colors are overwritten while OFF')
    restore = function_body(source,'static void AWERestoreBottomNativeVisualStates(')
    need('weakToStrongObjectsMapTable' in source, 'native snapshot retains recycled views')
    need('AWEBottomNativeVisualValuesEqual(' in restore and
         restore.index('[owned addObject:key]') < restore.index('[view setValue:value forKey:key]'),
         'restoration overwrites newer external values or compares after mutation')
    need('preserveLayer' in restore and 'externalLayer' in restore,
         'UIView restoration loses an external layer color')
    capture = function_body(source,'static void AWECaptureBottomNativeVisualState(')
    need('booleanGetter' in capture and 'NSSelectorFromString(booleanGetter)' in capture,
         'UIKit isHidden/isOpaque getters are skipped by native state capture')
    need('AWERecordBottomNativeVisualState(v,' in
         function_body(source,'static void ALB24ClearNativeBottomBackdrops('),
         'backdrop snapshot is not recorded')

check_release(SOURCE)

# Compile the actual C hash, tag, comparison, normal table and special loop.
# Only NSData/NSUserDefaults access at the boundary is replaced by input bytes;
# device-proof validity is an explicit test parameter, never bypassed in runtime.
hash_code = SOURCE[SOURCE.index('typedef struct {\n    uint8_t data[64];'):
                   SOURCE.index('static NSData *AWEHashData(')]
table_code = SOURCE[SOURCE.index('typedef struct {\n    uint8_t mask[32];'):
                    SOURCE.index('static NSString *AWENormalizedAccountIdentifier(')]
gate = function_body(SOURCE,'static BOOL AWEAuthorizationDigestAllowed(')
gate = gate.replace('if (digest.length != 32) return NO;', 'if (length != 32) return NO;')
gate = gate.replace('const uint8_t *candidate = digest.bytes;', '')
special_gate = special_body[special_body.index('    static const AWEAuthorizationDigestPair allowed'):]
special_gate = 'if (!validAuthorization || length != 32) return NO;\n'+special_gate
hex_array = lambda b: '{'+','.join(f'0x{v:02x}' for v in b)+'}'
fixtures=[]
for index, fixture in enumerate(AUDIT['hash_vectors']):
    fixtures.append('''{
        const uint8_t payload[] = %s;
        const uint8_t wanted[32] = %s;
        AWEHashContext context; uint8_t actual[32];
        AWEHashInit(&context);
        AWEHashUpdate(&context, payload, sizeof(payload));
        AWEHashFinal(&context, actual);
        assert(memcmp(actual,wanted,32)==0);
        assert(AWEAuthorizationDigestAllowedBytes(actual,32));
        assert(AWECustomBottomBarReleaseAllowedBytes(actual,32,YES)==%s);
    }''' % (hex_array(bytes.fromhex(fixture['payload_hex'])),
             hex_array(bytes.fromhex(fixture['digest_hex'])), 'YES' if index else 'NO'))
c_code = '''#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
typedef size_t NSUInteger;
typedef int BOOL;
#define YES 1
#define NO 0
'''+hash_code+table_code+'''
static BOOL AWEAuthorizationDigestAllowedBytes(const uint8_t *candidate, size_t length) {
'''+gate+'''
}
static BOOL AWECustomBottomBarReleaseAllowedBytes(const uint8_t *candidate,
    size_t length, BOOL validAuthorization) {
'''+special_gate+'''
}
int main(void) {
    assert(AWE_AUTHORIZATION_DIGEST_COUNT==51);
    size_t granted=0, rejected=0;
    for (size_t row=0; row<AWE_AUTHORIZATION_DIGEST_COUNT; row++) {
        uint8_t value[32];
        for (size_t i=0;i<32;i++)
            value[i]=kAWEAuthorizationDigests[row].mask[i]^kAWEAuthorizationDigests[row].value[i];
        assert(AWEAuthorizationDigestTag(value)==kAWEAuthorizationDigestTags[row]);
        assert(AWEAuthorizationDigestAllowedBytes(value,32));
        BOOL entitled=AWECustomBottomBarReleaseAllowedBytes(value,32,YES);
        assert(entitled==(row==21 || row==28 || row==36));
        granted+=entitled;
        assert(!AWECustomBottomBarReleaseAllowedBytes(value,32,NO));
        assert(!AWEAuthorizationDigestAllowedBytes(value,31));
        assert(!AWECustomBottomBarReleaseAllowedBytes(value,31,YES));
        assert(!AWEAuthorizationDigestAllowedBytes(value,33));
        assert(!AWECustomBottomBarReleaseAllowedBytes(value,33,YES));
        for (size_t i=0;i<32;i++) {
            value[i]^=1;
            assert(!AWEAuthorizationDigestAllowedBytes(value,32));
            assert(!AWECustomBottomBarReleaseAllowedBytes(value,32,YES));
            rejected++;
            value[i]^=1;
        }
    }
    assert(granted==3 && rejected==1632);
'''+ '\n'.join(fixtures)+'''
    puts("Auth51: 51 authorized rows, 3 special grants, 1632 altered digests rejected; production hash/tag/compare code passed.");
    return 0;
}
'''
compiler = shutil.which('clang') or shutil.which('cc')
if compiler:
    with tempfile.TemporaryDirectory(prefix='alb-auth51-') as directory:
        code = Path(directory)/'authorization.c'
        executable = Path(directory)/'authorization'
        code.write_text(c_code)
        subprocess.run([compiler,'-std=c11','-Wall','-Wextra','-Werror',str(code),'-o',str(executable)],check=True)
        subprocess.run([str(executable)],check=True)
else:
    print('Auth51 C execution skipped: no C compiler; SDK compilation remains authoritative.')
print('Auth51 prior authorization/2.3 rollback/native-release source checks passed; no UIKit/device test.')
