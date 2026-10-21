#!/usr/bin/env python3
"""Guard against restoring the retired manual primary-action dispatch path."""
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "ALBTopNative/ALBTopNativeGroup.m"
def main():
    source = SOURCE.read_text(encoding="utf-8")
    tweak = (ROOT / "Tweak.x").read_text(encoding="utf-8")
    assert 'ALBTopNativeIconButton' not in source + tweak
    assert 'performPrimaryAction' not in source
    assert 'button.menu = menu;' in tweak
    assert 'button.showsMenuAsPrimaryAction = YES;' in tweak
    print(json.dumps({"test": "TEST9 direct UIButton menu source",
        "retired_manual_dispatch": True, "availability_sensitive_call_present": False,
        "apple_sdk_compile": False,
        "note": "Real carriers and Tweak are compiled separately by the iPhone SDK CI steps."}))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
