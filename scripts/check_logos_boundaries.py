#!/usr/bin/env python3
"""Check Logos line-tail hazards and hook boundaries; not an SDK compiler.

The upstream %orig branch currently replaces from the directive to end-of-line.
Keep subsequent statements and braces on their own lines. In --generated mode,
inspect REAL logos.pl output for unexpanded directives and nested hook functions.
Comments, strings, character literals and C preprocessor lines are masked without
changing offsets. No source or generated code is modified by this checker.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ("Tweak.x", "CommentGlassDYKiller.xm", "MessageTopGlassV3.xm")
LITERALS = re.compile(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\[\s\S]|[^"\\])*"|\'(?:\\[\s\S]|[^\'\\])*\'')
DIRECTIVES = re.compile(r'\bstatic\b[^;{}\n]*\b('
                        r'_logos_(?:method|meta_method|function)\$[\w$]+|'
                        r'_logosLocal(?:Ctor|Dtor)_\w+)\s*\([^;{}]*\)\s*\{')


def mask_literals(text: str) -> str:
    return LITERALS.sub(lambda m: re.sub(r'[^\n]', ' ', m.group()), text)


def mask_preprocessor(text: str) -> str:
    lines = text.splitlines(keepends=True)
    continuation = False
    for index, line in enumerate(lines):
        if continuation or line.lstrip().startswith('#'):
            continuation = line.rstrip('\r\n').endswith('\\')
            lines[index] = re.sub(r'[^\n]', ' ', line)
    return ''.join(lines)


def line_at(text: str, offset: int) -> int:
    return text.count('\n', 0, offset) + 1


def orig_issues(text: str) -> tuple[int, list[str]]:
    masked = mask_literals(text)
    issues: list[str] = []
    count = 0
    for match in re.finditer(r'%orig\b', masked):
        # &%orig returns a pointer and uses a different upstream patch branch.
        if masked[:match.start()].rstrip().endswith('&'):
            continue
        count += 1
        pos = match.end()
        while pos < len(masked) and masked[pos].isspace():
            pos += 1
        if pos < len(masked) and masked[pos] == '(':
            depth = 1
            pos += 1
            while pos < len(masked) and depth:
                depth += (masked[pos] == '(') - (masked[pos] == ')')
                pos += 1
            if depth:
                issues.append(f'{line_at(text, match.start())}: unterminated %orig arguments')
                continue
            while pos < len(masked) and masked[pos].isspace():
                pos += 1
        if pos >= len(masked) or masked[pos] != ';':
            issues.append(f'{line_at(text, match.start())}: keep %orig as a complete statement ending in ;')
            continue
        end = masked.find('\n', pos)
        end = len(masked) if end < 0 else end
        if masked[pos + 1:end].strip():
            issues.append(f'{line_at(text, match.start())}: executable text after %orig; '
                          'move return/statements/closing braces to a new line')
    return count, issues


def structure_issues(text: str, generated: bool) -> tuple[int, list[str]]:
    masked = mask_preprocessor(mask_literals(text))
    issues: list[str] = []
    stack: list[tuple[str, int]] = []
    pairs = {')': '(', ']': '[', '}': '{'}
    definitions = list(DIRECTIVES.finditer(masked)) if generated else []
    definition_at = {m.end() - 1: m.group(1) for m in definitions}
    brace_depth = 0
    for match in re.finditer(r'[()\[\]{}]', masked):
        pos, char = match.start(), match.group()
        if pos in definition_at and brace_depth != 0:
            issues.append(f'{line_at(text, pos)}: generated function {definition_at[pos]} '
                          f'is nested at brace depth {brace_depth}')
        if char in '([{':
            stack.append((char, pos))
            brace_depth += char == '{'
        else:
            if not stack or stack[-1][0] != pairs[char]:
                issues.append(f'{line_at(text, pos)}: unmatched {char}')
                return len(definitions), issues
            stack.pop()
            brace_depth -= char == '}'
    for char, pos in stack:
        issues.append(f'{line_at(text, pos)}: unclosed {char}')
    if generated:
        unexpanded = re.search(r'%(?:orig|hook|hookf|end|ctor|dtor|new|init|group|config)\b', masked)
        if unexpanded:
            issues.append(f'{line_at(text, unexpanded.start())}: unexpanded Logos directive')
        if not definitions:
            issues.append('no generated hook definitions found; verify this is real logos.pl output')
    return len(definitions), issues


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generated', action='store_true')
    parser.add_argument('files', nargs='*', type=Path)
    args = parser.parse_args()
    if args.generated and not args.files:
        parser.error('--generated requires actual preprocessed files')
    paths = args.files or [ROOT / name for name in SOURCES]
    failures = 0
    for path in paths:
        if not path.is_file():
            print(f'{path}: missing source', file=sys.stderr)
            failures += 1
            continue
        text = path.read_text(encoding='utf-8')
        n_orig, orig_errors = (0, []) if args.generated else orig_issues(text)
        n_def, errors = structure_issues(text, args.generated)
        errors = orig_errors + errors
        for error in errors:
            print(f'{path}:{error}', file=sys.stderr)
        failures += len(errors)
        if not errors:
            detail = f'{n_def} generated hooks' if args.generated else f'{n_orig} original-call directives'
            print(f'{path.name}: boundary checks passed ({detail}; not SDK compilation)')
    return 1 if failures else 0


if __name__ == '__main__':
    raise SystemExit(main())
