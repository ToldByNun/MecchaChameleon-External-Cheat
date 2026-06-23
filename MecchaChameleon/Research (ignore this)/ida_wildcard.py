import re
import sys
import argparse
import difflib
from pathlib import Path
from dataclasses import dataclass, field


LINE_RE = re.compile(
    r"^\.(?:text|data|rdata|idata|pdata):([0-9A-Fa-f]+)\s+"
    r"((?:[0-9A-Fa-f]{2}\s+)+)"
    r"(.*)"
)
BYTE_RE   = re.compile(r"[0-9A-Fa-f]{2}")
LABEL_RE  = re.compile(r"^\.(?:text|data|rdata|idata|pdata):([0-9A-Fa-f]+)\s+((?:loc_|sub_|unk_)\w+)")
XREF_RE   = re.compile(r"CODE XREF:")

# ModRM bytes where mod=00, rm=101 → RIP-relative disp32
RIP_MODRM = frozenset({0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D})


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class Instruction:
    addr:     int
    bytes_:   list[str]
    mnemonic: str


@dataclass
class Block:
    start_addr:   int = 0
    label:        str | None = None
    xref_count:   int = 0
    instructions: list[Instruction] = field(default_factory=list)

    def fingerprint(self) -> tuple:
        first = tuple(self.instructions[0].bytes_[:4])  if self.instructions else ()
        last  = tuple(self.instructions[-1].bytes_[:2]) if self.instructions else ()
        return (self.xref_count, len(self.instructions), first, last)


# ---------------------------------------------------------------------------
# Opcode-aware wildcard mask  (single-file mode)
# ---------------------------------------------------------------------------

def opcode_mask(bytes_int: list[int]) -> list[bool]:
    """
    Return a per-byte bool mask: True = this byte is an address/offset that
    should be wildcarded regardless of what version we are looking at.

    Covers the full x86-64 RIP-relative and branch encoding space:
      E8 rel32          CALL rel32
      E9 rel32          JMP  rel32
      EB rel8           JMP  short
      0F 8x rel32       Jcc  rel32
      7x rel8           Jcc  short
      [REX] 8D /r disp32   LEA  reg, [RIP+disp32]
      [REX] 8B /r disp32   MOV  reg, [RIP+disp32]  (load)
      [REX] 89 /r disp32   MOV  [RIP+disp32], reg  (store)
      [REX] C6 05 disp32 imm8   MOV byte [RIP+disp32], imm8
      [REX] C7 05 disp32 imm32  MOV dword [RIP+disp32], imm32
      [REX] 80 /x disp32 imm8   CMP/ADD/SUB… byte [RIP+disp32], imm
      [REX] 81 /x disp32 imm32  CMP/ADD/SUB… dword [RIP+disp32], imm
      [REX] 83 /x disp32 imm8   CMP/ADD/SUB… dword [RIP+disp32], simm8
      [REX] 38-3B /r disp32     CMP  [RIP+disp32], r  /  CMP r, [RIP+disp32]
    """
    n    = len(bytes_int)
    mask = [False] * n
    i    = 0

    while i < n and 0x40 <= bytes_int[i] <= 0x4F:
        i += 1

    if i >= n:
        return mask

    op  = bytes_int[i]
    op2 = bytes_int[i + 1] if i + 1 < n else None

    def rel32(start: int):
        for j in range(start, min(start + 4, n)):
            mask[j] = True

    def rel8(pos: int):
        if pos < n:
            mask[pos] = True

    def rip_disp32(start: int):
        for j in range(start, min(start + 4, n)):
            mask[j] = True

    # CALL rel32
    if op == 0xE8:
        rel32(i + 1); return mask

    # JMP rel32
    if op == 0xE9:
        rel32(i + 1); return mask

    # JMP short
    if op == 0xEB:
        rel8(i + 1); return mask

    # Jcc near  (0F 8x)
    if op == 0x0F and op2 is not None and 0x80 <= op2 <= 0x8F:
        rel32(i + 2); return mask

    # Jcc short  (70–7F)
    if 0x70 <= op <= 0x7F:
        rel8(i + 1); return mask

    # LEA reg, [RIP+disp32]
    if op == 0x8D and op2 in RIP_MODRM:
        rip_disp32(i + 2); return mask

    # MOV reg, [RIP+disp32]  (load)
    if op == 0x8B and op2 in RIP_MODRM:
        rip_disp32(i + 2); return mask

    # MOV [RIP+disp32], reg  (store: 89 /r)
    if op == 0x89 and op2 in RIP_MODRM:
        rip_disp32(i + 2); return mask

    # MOV byte [RIP+disp32], imm8
    if op == 0xC6 and op2 == 0x05:
        rip_disp32(i + 2); return mask

    # MOV dword [RIP+disp32], imm32
    if op == 0xC7 and op2 == 0x05:
        rip_disp32(i + 2); return mask

    # CMP/ADD/SUB… byte [RIP+disp32], imm8
    if op == 0x80 and op2 in RIP_MODRM:
        rip_disp32(i + 2); return mask

    # CMP/ADD/SUB… dword [RIP+disp32], imm32
    if op == 0x81 and op2 in RIP_MODRM:
        rip_disp32(i + 2); return mask

    # CMP/ADD/SUB… dword [RIP+disp32], simm8
    if op == 0x83 and op2 in RIP_MODRM:
        rip_disp32(i + 2); return mask

    # CMP [RIP+disp32], r  or  CMP r, [RIP+disp32]
    if op in (0x38, 0x39, 0x3A, 0x3B) and op2 in RIP_MODRM:
        rip_disp32(i + 2); return mask

    return mask


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

def parse_file(path: Path) -> list[Block]:
    blocks: list[Block] = []
    cur: Block | None = None

    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            line = line.rstrip("\r\n")

            if "---" in line and line.strip().replace("-", "") == "":
                if cur is not None and cur.instructions:
                    blocks.append(cur)
                cur = Block()
                continue

            lm = LABEL_RE.match(line)
            if lm:
                if cur is None:
                    cur = Block()
                if cur.label is None:
                    cur.label = lm.group(2)

            if XREF_RE.search(line):
                if cur is None:
                    cur = Block()
                cur.xref_count += 1

            im = LINE_RE.match(line)
            if im:
                addr     = int(im.group(1), 16)
                bytes_s  = BYTE_RE.findall(im.group(2))
                mnemonic = im.group(3).strip()
                if bytes_s:
                    if cur is None:
                        cur = Block()
                    if not cur.start_addr:
                        cur.start_addr = addr
                    cur.instructions.append(Instruction(addr, bytes_s, mnemonic))

    if cur is not None and cur.instructions:
        blocks.append(cur)

    blocks.sort(key=lambda b: b.start_addr)
    for b in blocks:
        b.instructions.sort(key=lambda i: i.addr)

    return blocks


# ---------------------------------------------------------------------------
# Block alignment (diff mode)
# ---------------------------------------------------------------------------

def align_blocks(
    blocks_a: list[Block],
    blocks_b: list[Block],
) -> tuple[list[tuple[Block, Block]], list[Block], list[Block]]:
    fps_a = [b.fingerprint() for b in blocks_a]
    fps_b = [b.fingerprint() for b in blocks_b]
    matcher = difflib.SequenceMatcher(None, fps_a, fps_b, autojunk=False)

    matched:   list[tuple[Block, Block]] = []
    orphans_a: list[Block] = []
    orphans_b: list[Block] = []

    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            for a, b in zip(blocks_a[i1:i2], blocks_b[j1:j2]):
                matched.append((a, b))
        elif tag == "replace":
            seg_a, seg_b = blocks_a[i1:i2], blocks_b[j1:j2]
            if len(seg_a) == len(seg_b):
                for a, b in zip(seg_a, seg_b):
                    matched.append((a, b))
            else:
                orphans_a.extend(seg_a)
                orphans_b.extend(seg_b)
        elif tag == "delete":
            orphans_a.extend(blocks_a[i1:i2])
        elif tag == "insert":
            orphans_b.extend(blocks_b[j1:j2])

    return matched, orphans_a, orphans_b


# ---------------------------------------------------------------------------
# Pattern building
# ---------------------------------------------------------------------------

def build_single(blocks: list[Block], wildcard: str) -> tuple[list[str], list[dict]]:
    pattern: list[str] = []
    diffs:   list[dict] = []

    for block in blocks:
        for instr in block.instructions:
            bytes_int = [int(x, 16) for x in instr.bytes_]
            mask      = opcode_mask(bytes_int)
            for raw, wc in zip(instr.bytes_, mask):
                if wc:
                    pattern.append(wildcard)
                    diffs.append({
                        "block_label": block.label,
                        "mnemonic":    instr.mnemonic,
                        "byte":        raw,
                    })
                else:
                    pattern.append(raw.upper())

    return pattern, diffs


def build_diff(
    matched:  list[tuple[Block, Block]],
    wildcard: str,
) -> tuple[list[str], list[dict]]:
    pattern: list[str] = []
    diffs:   list[dict] = []

    for block_a, block_b in matched:
        instrs_a = block_a.instructions
        instrs_b = block_b.instructions

        if len(instrs_a) != len(instrs_b):
            flat_a = [b for i in instrs_a for b in i.bytes_]
            flat_b = [b for i in instrs_b for b in i.bytes_]
            for idx in range(max(len(flat_a), len(flat_b))):
                ba = flat_a[idx] if idx < len(flat_a) else None
                bb = flat_b[idx] if idx < len(flat_b) else None
                if ba is None or bb is None or ba.upper() != bb.upper():
                    pattern.append(wildcard)
                    diffs.append({"block_label": block_a.label, "byte_a": ba, "byte_b": bb})
                else:
                    pattern.append(ba.upper())
            continue

        for instr_a, instr_b in zip(instrs_a, instrs_b):
            for idx in range(max(len(instr_a.bytes_), len(instr_b.bytes_))):
                ba = instr_a.bytes_[idx] if idx < len(instr_a.bytes_) else None
                bb = instr_b.bytes_[idx] if idx < len(instr_b.bytes_) else None
                if ba is None or bb is None or ba.upper() != bb.upper():
                    pattern.append(wildcard)
                    diffs.append({
                        "block_label": block_a.label,
                        "byte_index":  idx,
                        "byte_a":      ba,
                        "byte_b":      bb,
                        "mnemonic_a":  instr_a.mnemonic,
                        "mnemonic_b":  instr_b.mnemonic,
                    })
                else:
                    pattern.append(ba.upper())

    return pattern, diffs


def build_smart(
    matched:  list[tuple[Block, Block]],
    blocks_a: list[Block],
    wildcard: str,
) -> tuple[list[str], list[dict]]:
    """
    Opcode-aware pass first: wildcard all RIP-relative offsets and branch
    targets unconditionally. Then OR in any bytes that still differ between
    the two versions.
    """
    pattern: list[str] = []
    diffs:   list[dict] = []

    for block_a, block_b in matched:
        instrs_a = block_a.instructions
        instrs_b = block_b.instructions

        if len(instrs_a) != len(instrs_b):
            flat_a = [b for i in instrs_a for b in i.bytes_]
            flat_b = [b for i in instrs_b for b in i.bytes_]
            flat_int = [int(x, 16) for x in flat_a]
            for idx in range(max(len(flat_a), len(flat_b))):
                ba  = flat_a[idx] if idx < len(flat_a) else None
                bb  = flat_b[idx] if idx < len(flat_b) else None
                wc  = (idx < len(flat_int) and opcode_mask(flat_int)[idx]) if flat_int else False
                if wc or ba is None or bb is None or ba.upper() != bb.upper():
                    pattern.append(wildcard)
                    diffs.append({"block_label": block_a.label, "byte_a": ba, "byte_b": bb, "opcode_wc": wc})
                else:
                    pattern.append(ba.upper())
            continue

        for instr_a, instr_b in zip(instrs_a, instrs_b):
            bytes_int = [int(x, 16) for x in instr_a.bytes_]
            mask      = opcode_mask(bytes_int)
            for idx in range(max(len(instr_a.bytes_), len(instr_b.bytes_))):
                ba  = instr_a.bytes_[idx] if idx < len(instr_a.bytes_) else None
                bb  = instr_b.bytes_[idx] if idx < len(instr_b.bytes_) else None
                wc  = mask[idx] if idx < len(mask) else False
                if wc or ba is None or bb is None or (ba is not None and bb is not None and ba.upper() != bb.upper()):
                    pattern.append(wildcard)
                    diffs.append({
                        "block_label": block_a.label,
                        "byte_index":  idx,
                        "byte_a":      ba,
                        "byte_b":      bb,
                        "mnemonic":    instr_a.mnemonic,
                        "opcode_wc":   wc,
                    })
                else:
                    pattern.append((ba or "??").upper())

    return pattern, diffs


# ---------------------------------------------------------------------------
# Formatting
# ---------------------------------------------------------------------------

def format_pattern(pattern: list[str], group_size: int = 16, style: str = "hex") -> str:
    tokens = list(pattern)

    if style == "ida":
        tokens = ["?" if t == "??" else t for t in tokens]
    elif style == "escaped":
        return " ".join("??" if t == "??" else f"\\x{t}" for t in tokens)

    if group_size > 0:
        groups = [" ".join(tokens[i:i + group_size]) for i in range(0, len(tokens), group_size)]
        result = "\n".join(groups)
    else:
        result = " ".join(tokens)

    if style == "yara":
        result = "{ " + result + " }"

    return result


# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------

def print_stats(pattern: list[str], matched: int, wildcard: str) -> None:
    total = len(pattern)
    wc    = sum(1 for t in pattern if t == wildcard)
    fixed = total - wc
    print(f"\n{'═'*70}")
    print(f"  Pattern stats")
    print(f"{'═'*70}")
    if matched:
        print(f"  Matched blocks:  {matched}")
    print(f"  Total bytes:     {total}")
    print(f"  Fixed bytes:     {fixed}  ({fixed/total*100:.1f} %)")
    print(f"  Wildcards:       {wc}  ({wc/total*100:.1f} %)")
    print(f"{'═'*70}")


def print_diff_summary(diffs: list[dict]) -> None:
    if not diffs:
        print("\n  No differences — files are byte-identical!")
        return

    print(f"\n{'─'*70}")
    print(f"  Differences: {len(diffs)} byte(s) wildcarded")
    print(f"{'─'*70}")

    by_block: dict[str | None, list[dict]] = {}
    for d in diffs:
        by_block.setdefault(d.get("block_label"), []).append(d)

    for label, ds in by_block.items():
        header = label or "(unlabeled block)"
        opcode_count = sum(1 for d in ds if d.get("opcode_wc"))
        diff_count   = len(ds) - opcode_count
        parts = []
        if opcode_count: parts.append(f"{opcode_count} opcode-wc")
        if diff_count:   parts.append(f"{diff_count} diff-wc")
        print(f"\n  {header}  ({', '.join(parts) or str(len(ds)) + ' wildcards'})")
        for d in ds[:5]:
            mn  = d.get("mnemonic") or d.get("mnemonic_a", "")
            tag = "[opcode]" if d.get("opcode_wc") else "[diff]  "
            ba  = d.get("byte_a", "?")
            bb  = d.get("byte_b", "?")
            if ba != bb:
                print(f"    {tag}  {ba} → {bb}  |  {mn}")
            else:
                print(f"    {tag}  {ba}  |  {mn}")
        if len(ds) > 5:
            print(f"    ... and {len(ds) - 5} more")

    print(f"{'─'*70}")



def format_block_patterns(block_patterns: list[tuple[str, list[str]]], group_size: int, style: str) -> str:
    parts: list[str] = []
    for label, pattern in block_patterns:
        parts.append(f"; {label}")
        parts.append(format_pattern(pattern, group_size, style))
        parts.append("")
    return "\n".join(parts).rstrip()


def build_block_patterns_single(blocks: list[Block], wildcard: str) -> tuple[list[tuple[str, list[str]]], list[dict]]:
    out: list[tuple[str, list[str]]] = []
    all_diffs: list[dict] = []
    for block in blocks:
        pattern, diffs = build_single([block], wildcard)
        label = block.label or hex(block.start_addr)
        out.append((label, pattern))
        all_diffs.extend(diffs)
    return out, all_diffs


def build_block_patterns_diff(
    matched: list[tuple[Block, Block]],
    wildcard: str,
    smart: bool,
) -> tuple[list[tuple[str, list[str]]], list[dict]]:
    out: list[tuple[str, list[str]]] = []
    all_diffs: list[dict] = []
    for block_a, block_b in matched:
        if smart:
            pattern, diffs = build_smart([(block_a, block_b)], [block_a], wildcard)
        else:
            pattern, diffs = build_diff([(block_a, block_b)], wildcard)
        label = block_a.label or hex(block_a.start_addr)
        out.append((label, pattern))
        all_diffs.extend(diffs)
    return out, all_diffs


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="IDA dump byte pattern wildcard generator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
modes:
  single   opcode-aware wildcarding from one file (no comparison needed)
  diff     byte-diff between two files (original behaviour)
  smart    diff + opcode-aware combined (recommended for two-file input)

examples:
  python ida_wildcard.py single dump.txt
  python ida_wildcard.py diff   dump_v1.txt dump_v2.txt
  python ida_wildcard.py smart  dump_v1.txt dump_v2.txt
  python ida_wildcard.py smart  dump_v1.txt dump_v2.txt --style yara --group 16
  python ida_wildcard.py smart  dump_v1.txt dump_v2.txt --per-block --output patterns.txt
        """,
    )
    parser.add_argument("mode", choices=["single", "diff", "smart"])
    parser.add_argument("file_a", type=Path, help="First (or only) IDA dump")
    parser.add_argument("file_b", type=Path, nargs="?", default=None, help="Second IDA dump (diff/smart only)")
    parser.add_argument("--output", "-o", type=Path, default=None)
    parser.add_argument(
        "--style", "-s",
        choices=["hex", "yara", "escaped", "ida"],
        default="hex",
        help="hex (default/CE) | yara ({ ... }) | escaped (\\xNN) | ida (single ?)",
    )
    parser.add_argument("--group", "-g", type=int, default=16, metavar="N", help="Bytes per output line (default: 16, 0 = single line)")
    parser.add_argument("--per-block", action="store_true", help="Print/save one pattern per matched block instead of one giant pattern")
    parser.add_argument("--wildcard", "-w", default="??")
    parser.add_argument("--no-diff", action="store_true", help="Suppress per-block diff table")

    args = parser.parse_args()

    if args.mode in ("diff", "smart") and args.file_b is None:
        parser.error(f"mode '{args.mode}' requires two input files")

    for p in ([args.file_a] + ([args.file_b] if args.file_b else [])):
        if not p.exists():
            print(f"[ERROR] File not found: {p}", file=sys.stderr)
            sys.exit(1)

    print(f"[*] Parsing {args.file_a} ...")
    blocks_a = parse_file(args.file_a)
    print(f"    → {len(blocks_a)} blocks, {sum(len(b.instructions) for b in blocks_a)} instructions")

    pattern: list[str] = []
    diffs: list[dict] = []
    pattern_text = ""
    matched_count = 0

    if args.mode == "single":
        if args.per_block:
            block_patterns, diffs = build_block_patterns_single(blocks_a, args.wildcard)
            pattern = [tok for _, pat in block_patterns for tok in pat]
            pattern_text = format_block_patterns(block_patterns, args.group, args.style)
            matched_count = len(block_patterns)
        else:
            pattern, diffs = build_single(blocks_a, args.wildcard)
            pattern_text = format_pattern(pattern, args.group, args.style)

    else:
        print(f"[*] Parsing {args.file_b} ...")
        blocks_b = parse_file(args.file_b)
        print(f"    → {len(blocks_b)} blocks, {sum(len(b.instructions) for b in blocks_b)} instructions")

        print(f"[*] Aligning blocks ...")
        if len(blocks_a) == len(blocks_b):
            matched = list(zip(blocks_a, blocks_b))
            orphans_a, orphans_b = [], []
            print(f"    → {len(matched)} blocks matched (equal count, direct zip)")
        else:
            matched, orphans_a, orphans_b = align_blocks(blocks_a, blocks_b)
            print(f"    → {len(matched)} matched, {len(orphans_a)} orphan(s) in A, {len(orphans_b)} orphan(s) in B")
            for b in orphans_a:
                print(f"        skip A: {b.label or hex(b.start_addr)} ({len(b.instructions)} instrs)")
            for b in orphans_b:
                print(f"        skip B: {b.label or hex(b.start_addr)} ({len(b.instructions)} instrs)")

        print(f"[*] Building pattern ({args.mode}) ...")
        if args.per_block:
            block_patterns, diffs = build_block_patterns_diff(matched, args.wildcard, smart=(args.mode == "smart"))
            pattern = [tok for _, pat in block_patterns for tok in pat]
            pattern_text = format_block_patterns(block_patterns, args.group, args.style)
        elif args.mode == "diff":
            pattern, diffs = build_diff(matched, args.wildcard)
            pattern_text = format_pattern(pattern, args.group, args.style)
        else:
            pattern, diffs = build_smart(matched, blocks_a, args.wildcard)
            pattern_text = format_pattern(pattern, args.group, args.style)
        matched_count = len(matched)

    print_stats(pattern, matched_count, args.wildcard)
    print(f"\n  Pattern [{args.style}]:\n")
    print(pattern_text)

    if not args.no_diff:
        print_diff_summary(diffs)

    if args.output:
        args.output.write_text(pattern_text, encoding="utf-8")
        print(f"\n[✓] Pattern saved: {args.output}")


if __name__ == "__main__":
    main()