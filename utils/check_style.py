#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.
"""Checks the C conventions of AGENTS.md that clang-format cannot enforce.

  utils/check_style.py                 every .c / .h of the project (see DEFAULT_DIRS)
  utils/check_style.py FILE...         only the given files

Rules (AGENTS.md "コーディング規約"):
  spdx            the file starts with the SPDX header block
  decl-top        local variables are declared only at the top of the function body
                  (not in nested blocks, not after a statement, not in a for header)
  decl-merge      one declaration per type (same qualifiers + base type) in a function
  decl-order      const first, then project/dependency types, uint64_t .. uint8_t,
                  int64_t .. int8_t, size_t, bool, char, double, float, void
  blank-decl      a blank line after the declarations
  blank-jump      a blank line before return / break / continue / goto that follows
                  another statement in the same block
  blank-ctrl      a blank line before if / for / while / do / switch that follows another
                  statement, unless that statement assigns a variable the header tests
  blank-close     a blank line after a closing brace that is followed by a statement
  braces          control statements always use braces
  typedef-suffix  typedef names end with _t
  func-order      static inline -> static -> plain -> extern function definitions
  define-top      #define only at the top of the file, before the first C declaration (include
                  guards and helper macros #undef'd in the same file excepted)
Exit status: 0 = clean, 1 = violations (printed as path:line: [rule] message).
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
DEFAULT_DIRS = (
    "library/src", "library/include", "library/tests", "library/utils", "cli", "app/electron/native",
    "python/colopresso",
)
TYPE_RANK = {
    "uint64_t": 2, "uint32_t": 3, "uint16_t": 4, "uint8_t": 5,
    "int64_t": 6, "int32_t": 7, "int16_t": 8, "int8_t": 9,
    "size_t": 10, "bool": 11, "char": 12, "double": 13, "float": 14, "void": 15,
}
RANK_ALIASES = {
    "int": "int32_t", "unsigned": "uint32_t", "unsigned int": "uint32_t", "signed": "int32_t",
    "unsigned char": "uint8_t", "signed char": "int8_t", "short": "int16_t",
    "unsigned short": "uint16_t", "long": "int64_t", "long long": "int64_t",
    "unsigned long": "uint64_t", "unsigned long long": "uint64_t",
    "uintptr_t": "size_t", "ptrdiff_t": "size_t", "ssize_t": "size_t",
}
RANK_NAMES = {1: "project/dependency type"}
RANK_NAMES.update({rank: name for name, rank in TYPE_RANK.items()})

KEYWORDS = {
    "return", "if", "else", "for", "while", "do", "switch", "case", "default", "goto", "break",
    "continue", "sizeof", "typedef", "struct", "enum", "union", "static", "const", "volatile",
    "extern", "inline", "register", "restrict", "unsigned", "signed", "_Static_assert",
    "static_assert", "_Alignas", "_Alignof", "__extension__",
}
QUALIFIERS = {"const", "static", "volatile", "register", "restrict", "__extension__"}
NOT_DECL_START = {
    "return", "if", "else", "for", "while", "do", "switch", "case", "default", "goto", "break",
    "continue", "sizeof", "typedef", "_Static_assert", "static_assert",
}
CONTROL = ("if", "for", "while", "do", "switch")
JUMPS = ("return", "break", "continue", "goto")
ASSIGN_RE = re.compile(r"^(?P<lhs>[^=;]*?)(?:<<|>>|[-+*/%&|^])?=(?!=)")
INCDEC_RE = re.compile(r"^(?:\+\+|--)?\s*(?P<lhs>[\w.\->\[\]*()\s]+?)\s*(?:\+\+|--)?\s*;$")
IDENT_RE = re.compile(r"[A-Za-z_]\w*")
DEFINE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)")
UNDEF_RE = re.compile(r"^\s*#\s*undef\s+([A-Za-z_]\w*)", re.MULTILINE)
EXTERN_C_RE = re.compile(r'^(?:extern\s+"C"\s*\{|\})$')
DECL_RE = re.compile(
    r"^(?:(?:const|static|volatile|register|__extension__)\s+)*"
    r"(?:(?:struct|enum|union)\s+)?(?:(?:unsigned|signed|long|short)\s+)*"
    r"[A-Za-z_]\w*(?:\s+(?:const|volatile))?"
    r"(?:\s*\*+\s*(?:(?:const|volatile)\s+)?|\s+)[A-Za-z_]\w*\s*(?:\[|=|,|;)"
)
FOR_DECL_RE = re.compile(
    r"^for\s*\(\s*(?:const\s+)?(?:struct\s+)?[A-Za-z_]\w*(?:\s*\*+\s*|\s+)[A-Za-z_]\w*\s*[=;]"
)


def strip_comments_and_literals(text):
    """Replaces comments and string / char literals with spaces, keeping newlines."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
            out.append("".join("\n" if ch == "\n" else " " for ch in text[i:end]))
            i = end
        elif text.startswith("//", i):
            end = text.find("\n", i)
            end = n if end < 0 else end
            out.append(" " * (end - i))
            i = end
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            j = min(j + 1, n)
            out.append(c + " " * max(j - i - 2, 0) + (c if j - i >= 2 else ""))
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


class Item:
    __slots__ = ("kind", "sub", "text", "line", "depth", "blank_before", "pp_path")

    def __init__(self, kind, sub, text, line, depth, blank_before, pp_path):
        self.kind = kind
        self.sub = sub
        self.text = text
        self.line = line
        self.depth = depth
        self.blank_before = blank_before
        self.pp_path = pp_path


class Checker:
    def __init__(self, path):
        self.path = path
        self.raw = path.read_text(encoding="utf-8").splitlines()
        self.code = strip_comments_and_literals("\n".join(self.raw)).split("\n")
        self.errors = []

    def error(self, line, rule, message):
        self.errors.append((line, rule, message))

    def is_blank(self, index):
        return self.raw[index].strip() == ""

    def is_comment_only(self, index):
        return self.code[index].strip() == "" and self.raw[index].strip() != ""

    def run(self):
        head = "\n".join(self.raw[:5])
        if "SPDX-License-Identifier" not in head:
            self.error(0, "spdx", "missing the SPDX header block at the top of the file")
        self.check_typedefs()
        self.check_define_top()
        self.scan_file()
        return self.errors

    def check_define_top(self):
        first_code = None
        i, n = 0, len(self.code)
        while i < n:
            stripped = self.code[i].strip()
            if stripped.startswith("#"):
                while self.code[i].rstrip().endswith("\\") and i + 1 < n:
                    i += 1
            elif stripped and not EXTERN_C_RE.match(self.raw[i].strip()):
                first_code = i
                break
            i += 1
        if first_code is None:
            return
        undefined = set(UNDEF_RE.findall("\n".join(self.code)))
        for line in range(first_code, n):
            match = DEFINE_RE.match(self.code[line])
            if match and match.group(1) not in undefined:
                self.error(line, "define-top",
                           f"move #define {match.group(1)} to the top of the file (after the "
                           "#include lines, before the first declaration)")

    def check_typedefs(self):
        text = "\n".join(self.code)
        for match in re.finditer(r"\btypedef\b", text):
            level, end = 0, match.end()
            while end < len(text) and not (text[end] == ";" and level == 0):
                level += {"{": 1, "}": -1}.get(text[end], 0)
                end += 1
            body = text[match.start():end + 1]
            line = text.count("\n", 0, match.start())
            # function pointer, optionally with a calling-convention macro: (CALL *name)
            fnptr = None if "{" in body else re.search(
                r"\(\s*(?:[A-Za-z_]\w*\s+)?\*\s*([A-Za-z_]\w*)\s*\)", body)
            if fnptr:
                name = fnptr.group(1)
            else:
                tail = body.rsplit("}", 1)[-1] if "}" in body else body
                # an array typedef names the type before its bounds: typedef char name_t[LEN];
                tail = re.sub(r"(?:\s*\[[^\]]*\])+\s*;?\s*$", "", tail)
                names = IDENT_RE.findall(tail)
                name = names[-1] if names else ""
            if name and not name.endswith("_t"):
                self.error(line, "typedef-suffix", f"typedef name '{name}' must end with _t")

    def scan_file(self):
        depth = 0
        signature = []
        pp_stack = []
        func_category = -1
        i = 0
        n = len(self.code)
        while i < n:
            stripped = self.code[i].strip()
            if stripped.startswith("#"):
                self.track_pp(stripped, pp_stack, i)
                while self.code[i].rstrip().endswith("\\") and i + 1 < n:
                    i += 1
                i += 1
                continue
            if depth == 0:
                if stripped == "{" and signature:
                    sig = " ".join(signature)
                    if "(" in sig and "=" not in sig.split("(")[0]:
                        category = self.function_category(sig)
                        if category < func_category:
                            names = ["static inline", "static", "plain", "extern"]
                            self.error(i, "func-order",
                                       f"{names[category]} function after {names[func_category]} "
                                       "functions (order: static inline, static, plain, extern)")
                        func_category = max(func_category, category)
                        i = self.scan_function(i, list(pp_stack))
                        signature = []
                        continue
                if stripped:
                    if stripped.endswith(";") or stripped.endswith("}") or stripped.endswith("};"):
                        signature = []
                    else:
                        signature.append(stripped)
            else:
                signature = []
            depth += stripped.count("{") - stripped.count("}")
            depth = max(depth, 0)
            i += 1

    @staticmethod
    def track_pp(stripped, pp_stack, index):
        word = stripped[1:].strip().split()[0] if stripped[1:].strip() else ""
        if word in ("if", "ifdef", "ifndef"):
            pp_stack.append([index, 0])
        elif word in ("elif", "else") and pp_stack:
            pp_stack[-1][1] += 1
        elif word == "endif" and pp_stack:
            pp_stack.pop()

    @staticmethod
    def function_category(sig):
        tokens = IDENT_RE.findall(sig.split("(")[0])
        if "static" in tokens and "inline" in tokens:
            return 0
        if "static" in tokens:
            return 1
        if "extern" in tokens:
            return 3
        return 2

    def build_items(self, start, pp_stack):
        """Splits the function body that opens at line `start` into items. Returns (items, end)."""
        items = []
        depth = 1
        buf = []
        buf_line = 0
        init_depth = 0
        paren = 0
        blank_pending = False
        i = start + 1
        n = len(self.code)
        while i < n:
            stripped = self.code[i].strip()
            if not buf:
                if self.is_blank(i):
                    blank_pending = True
                    i += 1
                    continue
                if self.is_comment_only(i):
                    i += 1
                    continue
                if stripped.startswith("#"):
                    self.track_pp(stripped, pp_stack, i)
                    while self.code[i].rstrip().endswith("\\") and i + 1 < n:
                        i += 1
                    i += 1
                    continue
                buf_line = i
            buf.append(stripped)
            paren += stripped.count("(") - stripped.count(")")
            text = " ".join(part for part in buf if part)
            ends_open = text.endswith("{")
            if init_depth or (ends_open and paren == 0 and self.has_top_level_assign(text)):
                init_depth += stripped.count("{") - stripped.count("}")
                if init_depth > 0 or not text.endswith(";"):
                    i += 1
                    continue
                init_depth = 0
            if paren > 0 or not (text.endswith((";", "{", "}", ":"))):
                i += 1
                continue
            path = tuple(tuple(entry) for entry in pp_stack)
            item = self.classify(text, buf_line, depth, blank_pending, path)
            items.append(item)
            buf = []
            paren = 0
            blank_pending = False
            if item.kind == "close":
                depth -= 1
            if item.kind == "open" or (item.kind == "close" and item.sub == "else"):
                depth += 1
            if depth == 0:
                return items, i + 1
            i += 1
        return items, n

    @staticmethod
    def has_top_level_assign(text):
        level = 0
        for idx, ch in enumerate(text):
            if ch in "([":
                level += 1
            elif ch in ")]":
                level -= 1
            elif ch == "=" and level == 0:
                prev = text[idx - 1] if idx else ""
                nxt = text[idx + 1] if idx + 1 < len(text) else ""
                if nxt != "=" and prev not in "=!<>":
                    return True
        return False

    def classify(self, text, line, depth, blank, path):
        def make(kind, sub=""):
            return Item(kind, sub, text, line, depth, blank, path)

        if text.startswith("}"):
            rest = text[1:].strip()
            if rest.startswith("else"):
                return make("close", "else")
            if rest.startswith("while"):
                return make("close", "while")
            return make("close")
        if re.match(r"^(case\b.*|default\s*|[A-Za-z_]\w*\s*):$", text):
            return make("label")
        first = IDENT_RE.match(text)
        word = first.group(0) if first else ""
        if text.endswith("{"):
            if word in CONTROL:
                return make("open", "ctrl")
            if word == "else":
                return make("open", "else")
            return make("open", "bare")
        if word in CONTROL or word == "else":
            return make("stmt", "nobrace")
        if word in JUMPS:
            return make("jump")
        if word not in NOT_DECL_START and DECL_RE.match(text):
            return make("decl")
        return make("stmt")

    def scan_function(self, start, pp_stack):
        items, end = self.build_items(start, pp_stack)
        self.check_declarations(items)
        self.check_blank_lines(items)
        return end

    @staticmethod
    def decl_key(text):
        head = re.split(r"[=,;\[]", text, maxsplit=1)[0]
        tokens = re.findall(r"[A-Za-z_]\w*|\*", head)
        qualifiers = [t for t in tokens if t in ("const", "static", "volatile")]
        names = [t for t in tokens if t not in QUALIFIERS and t != "*"]
        base_tokens = names[:-1] if len(names) > 1 else names
        base = " ".join(base_tokens)
        is_const = tokens and tokens[0] in ("const", "static") and "const" in qualifiers
        rank_name = RANK_ALIASES.get(base, base)
        rank = TYPE_RANK.get(rank_name, 1)
        return (tuple(sorted(set(qualifiers))), base), (0 if is_const else 1, rank)

    def check_declarations(self, items):
        seen = {}
        last_rank = None
        in_decls = True
        for item in items:
            if item.kind == "decl":
                if not in_decls or item.depth != 1:
                    self.error(item.line, "decl-top",
                               "declare variables only at the top of the function body")
                    continue
                key, rank = self.decl_key(item.text)
                previous = seen.get((key, item.pp_path))
                if previous is not None:
                    self.error(item.line, "decl-merge",
                               f"'{' '.join(key[0] + (key[1],))}' is already declared on line "
                               f"{previous + 1}; declare variables of one type in one statement")
                else:
                    seen[(key, item.pp_path)] = item.line
                if last_rank is not None and rank < last_rank:
                    want = "const " if rank[0] == 0 else ""
                    self.error(item.line, "decl-order",
                               f"{want}{RANK_NAMES.get(rank[1], key[1])} declared after "
                               f"{'const ' if last_rank[0] == 0 else ''}"
                               f"{RANK_NAMES.get(last_rank[1], '?')} (see AGENTS.md order)")
                last_rank = rank if last_rank is None else max(last_rank, rank)
                continue
            if item.kind == "open" and item.sub == "ctrl" and FOR_DECL_RE.match(item.text):
                self.error(item.line, "decl-top", "do not declare variables in a for header")
            if in_decls and item.kind != "close":
                in_decls = False
                if last_rank is not None and not item.blank_before:
                    self.error(item.line, "blank-decl", "add a blank line after the declarations")
            elif in_decls:
                in_decls = False

    @staticmethod
    def assigned_names(text):
        names = set(re.findall(r"(?<![&\w)\]])&\s*([A-Za-z_]\w*)", text))
        match = ASSIGN_RE.match(text)
        if not match or "(" in match.group("lhs") and "->" not in match.group("lhs"):
            match = INCDEC_RE.match(text)
        if match:
            names |= {name for name in IDENT_RE.findall(match.group("lhs")) if name not in KEYWORDS}
        return names

    @staticmethod
    def block_bodies(items):
        """Maps the index of each opening item to the identifiers used inside its block."""
        bodies = {}
        stack = []
        for index, item in enumerate(items):
            for open_index in stack:
                bodies.setdefault(open_index, set()).update(IDENT_RE.findall(item.text))
            if item.kind == "open":
                stack.append(index)
            elif item.kind == "close" and stack:
                stack.pop()
                if item.sub == "else":
                    stack.append(index)
        return bodies

    @staticmethod
    def sibling_branches(a, b):
        """True when two items sit in different branches (#if / #else) of the same directive."""
        for left, right in zip(a, b):
            if left == right:
                continue
            return left[0] == right[0]
        return False

    def check_blank_lines(self, items):
        previous = {1: None}
        bodies = self.block_bodies(items)
        for index, item in enumerate(items):
            depth = item.depth
            prev = previous.get(depth)
            if item.kind == "stmt" and item.sub == "nobrace":
                self.error(item.line, "braces", "always use braces for control statements")
            if (prev is not None and not item.blank_before and prev.kind != "decl" and
                    not self.sibling_branches(prev.pp_path, item.pp_path)):
                if prev.kind == "close" and item.kind not in ("close", "label"):
                    self.error(item.line, "blank-close",
                               "add a blank line after the closing brace of the previous block")
                elif item.kind == "jump" and prev.kind in ("stmt", "jump"):
                    self.error(item.line, "blank-jump",
                               f"add a blank line before '{IDENT_RE.match(item.text).group(0)}'")
                elif item.kind == "open" and item.sub == "ctrl" and prev.kind == "stmt":
                    used = set(IDENT_RE.findall(item.text))
                    if IDENT_RE.match(item.text).group(0) in ("for", "while", "do"):
                        used |= bodies.get(index, set())
                    if not (self.assigned_names(prev.text) & used):
                        self.error(item.line, "blank-ctrl",
                                   "add a blank line before this block (no blank is allowed only "
                                   "when the previous statement assigns a variable it tests)")
            if item.kind == "open":
                previous[depth] = item
                previous[depth + 1] = None
            elif item.kind == "close" and item.sub == "else":
                previous[depth - 1] = item
                previous[depth] = None
            elif item.kind == "close":
                previous[depth - 1] = item
            elif item.kind == "label":
                previous[depth] = None
            else:
                previous[depth] = item


def collect(args):
    """Files and directories (searched recursively); no arguments = the default directories."""
    roots = [pathlib.Path(arg).resolve() for arg in args] if args else [ROOT / d for d in DEFAULT_DIRS]
    files, missing = [], []
    for root in roots:
        if root.is_dir():
            files.extend(p for p in sorted(root.rglob("*")) if p.suffix in (".c", ".h") and p.is_file())
        elif root.exists():
            files.append(root)
        else:
            missing.append(root)
    return files, missing


def main(argv):
    total = 0
    files, missing = collect(argv[1:])
    for path in missing:
        print(f"{path}: no such file or directory", file=sys.stderr)
    if missing:
        return 2
    for path in files:
        if path.suffix not in (".c", ".h") or not path.exists():
            continue
        for line, rule, message in sorted(Checker(path).run()):
            try:
                shown = path.relative_to(ROOT)
            except ValueError:
                shown = path
            print(f"{shown}:{line + 1}: [{rule}] {message}")
            total += 1
    if total:
        print(f"{total} style violation(s); see AGENTS.md コーディング規約", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
