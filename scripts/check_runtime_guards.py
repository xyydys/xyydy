#!/usr/bin/env python3
"""Execute production request/queue/completion methods with a timed mock queue.
The native endpoint, readback and geometry-animation detector are mocked.
"""
from pathlib import Path
import subprocess, shutil, tempfile
from check_logos_boundaries import mask_literals
r=Path(__file__).resolve().parents[1]; sw=(r/'AwemeAlbumMorphBar.swift').read_text()
def method(signature):
    start=sw.index(signature); opening=sw.index('{',start); clean=mask_literals(sw); depth=0
    for end in range(opening,len(sw)):
        depth+=(clean[end]=='{')-(clean[end]=='}')
        if depth==0: return sw[start:end+1].replace('DispatchQueue.main','MockQueue.main')
    raise AssertionError(signature)
methods='\n'.join(method(s) for s in [
    '    func setCompactMode(_ compact: Bool, animated: Bool) {',
    '    private func enqueueNativeRequest(animated: Bool) {',
    '    private func issueNativeRequest(animated: Bool) {',
    '    private func waitForNativeTransitionToSettle() {',
    '    private func queueNativeRefresh() {'])
fixture=r'''
import Foundation
var clock:Double=0
func CACurrentMediaTime()->Double { clock }
func ALBReproIsRecording()->Bool { false }
func ALBReproRecordEvent(_ name:String,_ fields:[String:Any]) { }
// The real C policy is independently compiled by check_unified_material.py.
func ALBShouldWaitForNativeSettle(_ elapsed:Double,_ geometry:Bool)->Bool {
    elapsed < 1.8 && (elapsed < 0.12 || geometry)
}
struct MockTime {
    var time:Double
    static func now()->MockTime { MockTime(time:clock) }
    static func +(left:MockTime,right:Double)->MockTime { MockTime(time:left.time+right) }
}
final class MockQueue {
    static let main=MockQueue()
    var pending:[(Double,()->Void)]=[]
    func async(_ block:@escaping ()->Void) { pending.append((clock,block)) }
    func asyncAfter(deadline:MockTime,execute block:@escaping ()->Void) { pending.append((deadline.time,block)) }
    func ready() {
        var turns=0
        while let index=pending.firstIndex(where: { $0.0 <= clock+0.000001 }) {
            turns+=1; precondition(turns<10000); pending.remove(at:index).1()
        }
    }
    func advance(_ duration:Double) {
        let end=clock+duration
        while let next=pending.map({$0.0}).filter({$0 <= end}).min() {
            clock=max(clock,next); ready()
        }
        clock=end; ready()
    }
    func reset() { pending.removeAll();clock=0 }
}
final class MockView { var window:Int?=1 }
final class Subject {
    var nativeMinimizeEnabled=true, isViewLoaded=true, view=MockView()
    var nativeRequestOutstanding=false, nativeRequestAccepted=false
    var requestedCompact=false, compactMode=false, lastObservedNativeState=0
    var nativeRequestGeneration=0, nativeRequestDispatchQueued=false
    var nativeRequestInFlight=false, nativeRefreshQueued=false
    var nativeAnimationRunningSince:Double?=nil, nativeRequestWaitingForSettle=false
    var calls:[Bool]=[], reads=0, accept=true, geometryAnimating=false, nativeState=false
    var reproLastRequestSample:Double=0
    var callback:(()->Void)?
    func reproControllerSnapshot()->[String:Any] { [:] }
    func prepareIndependentDriver() { }
    func traceNativeScale(_ phase:String) { }
    func hasNativeGeometryAnimation()->Bool { geometryAnimating }
    func observeNativeState() {
        reads+=1; compactMode=nativeState; lastObservedNativeState=nativeState ? 1 : 0
        if nativeRequestOutstanding && compactMode == requestedCompact { nativeRequestOutstanding=false }
        if !nativeRequestOutstanding { requestedCompact=compactMode }
    }
    func request(_ compact:Bool,_ animated:Bool=true) { setCompactMode(compact,animated:animated) }
    func layout() { queueNativeRefresh() }
METHODS
}
func AWEAlbumMorphRequestNativeMinimized(_ subject:Subject,_ compact:Int32,_ animated:Int32)->Int32 {
    subject.calls.append(compact != 0)
    subject.callback?()
    if subject.accept { subject.nativeState=compact != 0; return 1 }
    return 0
}
var count=0
func reset() { MockQueue.main.reset() }
// Burst coalesces to the final target before entering UIKit.
reset();do {
    let s=Subject(); for _ in 0..<1000 { s.request(true);s.request(false) }
    precondition(MockQueue.main.pending.count==1);MockQueue.main.ready();MockQueue.main.advance(0.3)
    precondition(s.calls==[false] && !s.nativeRequestOutstanding);count+=1
}
reset();do {
    let s=Subject();for _ in 0..<1000 { s.request(true) };MockQueue.main.ready();MockQueue.main.advance(0.3)
    precondition(s.calls==[true]);count+=1
}
// A reversal, including nonanimated, waits for actual geometry motion.
for animated in [false,true] {
    reset();let s=Subject();s.geometryAnimating=true
    s.request(true);MockQueue.main.ready();s.request(false,animated);MockQueue.main.ready()
    MockQueue.main.advance(0.3);precondition(s.calls==[true] && s.nativeRequestWaitingForSettle)
    s.geometryAnimating=false;MockQueue.main.advance(0.3)
    precondition(s.calls==[true,false] && !s.compactMode && !s.nativeRequestOutstanding);count+=1
}
// If the pending target is withdrawn, do not reverse after settle.
reset();do {
    let s=Subject();s.geometryAnimating=true;s.request(true);MockQueue.main.ready()
    s.request(false);MockQueue.main.ready();s.request(true);MockQueue.main.ready()
    s.geometryAnimating=false;MockQueue.main.advance(0.3);precondition(s.calls==[true]);count+=1
}
// A rejected native endpoint never reissues from layout callbacks.
reset();do {
    let s=Subject();s.accept=false;s.request(true);MockQueue.main.ready()
    for _ in 0..<1000 { s.layout() };MockQueue.main.ready()
    precondition(s.calls==[true] && !s.compactMode && !s.nativeRequestOutstanding);count+=1
}
// Expired native motion cannot leave request scheduling permanently blocked.
reset();do {
    let s=Subject();s.geometryAnimating=true;s.request(true);MockQueue.main.ready()
    s.request(false);MockQueue.main.ready();MockQueue.main.advance(2)
    precondition(s.calls==[true,false]);count+=1
}
// Layout only reads state; does not request minimize or rebind scrolling.
reset();do {
    let s=Subject();for _ in 0..<1000 { s.layout() }
    precondition(MockQueue.main.pending.count==1);MockQueue.main.ready()
    precondition(s.reads==1 && s.calls.isEmpty);count+=1
}
print("Extracted production queue: \(count) cases passed; no UIKit/device test.")
'''.replace('METHODS',methods)
compiler=shutil.which('swiftc')
if not compiler: raise SystemExit('swiftc required for extracted scheduling test')
with tempfile.TemporaryDirectory(prefix='alb-material-queue-') as folder:
    source=Path(folder)/'main.swift'; binary=Path(folder)/'test'; source.write_text(fixture)
    subprocess.run([compiler,str(source),'-o',str(binary)],check=True,timeout=40)
    subprocess.run([str(binary)],check=True,timeout=10)
t=(r/'Tweak.x').read_text(); top=(r/'ALBTopNative/ALBTopNativeGroup.m').read_text()
assert t.count('if (ALB17IsResidentChromeController((UIViewController *)self)) {')==4
assert 'if (self.layoutInProgress) return;' in top and '@finally' in top
assert 'group.providerFitPasses < 2' in top
assert 'centeredSurface' not in sw
print('Resident controller and layout reentry guards retained.')
