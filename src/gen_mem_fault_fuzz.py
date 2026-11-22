#!/usr/bin/env python3
"""Generate src/mem_fault_fuzz.S, a self-checking memory-fault fuzz test.

Each case performs one load, store or instruction fetch (translated through a
static Sv39 page table, or untranslated in M-mode) and the trap handler checks
mcause/mtval against a reference model before resuming at the next case.  See
black-parrot#312 and #313.

    python3 src/gen_mem_fault_fuzz.py --seed 1 --count 100 -o src/mem_fault_fuzz.S

The output only depends on --seed, --count and --io-data, so it is
reproducible.  Standard library only.

Expectations that are RISC-V privileged-spec rules (Sv39 translation):
  * VA bits 63:39 must equal bit 38, else a page fault of the access type.
  * PTE invalid (V=0), W=1 with R=0, or a reserved bit 60:54 set: page fault.
    (Bits 63:61 are Svnapot/Svpbmt when implemented, so they are not used.)
  * Non-leaf PTE with A, D or U set (reserved for future standard use,
    treated like the other reserved bits): page fault.  Non-leaf at level 0:
    page fault.
  * Leaf permissions: fetch needs X; load needs R, or X with MXR; store needs
    W.  U-mode needs U=1.  S-mode with U=1 needs SUM for data and is never
    allowed for fetch.  MXR and SUM do not affect fetch / stores respectively.
  * Misaligned superpage (ppn[i-1:0] != 0): page fault.
  * mtval is the faulting VA, mcause 12/13/15 (page) or 1/5/7 (access); mepc
    is the address of the faulting instruction.
  * A fetch that succeeds executes an `ecall`: cause 8 (U), 9 (S), 11 (M),
    mtval 0.
  * mtval is not compared when the address does not fit in 39 bits sign
    extended: the spec lets an implementation convert such an address into
    another invalid address, so BlackParrot's truncation is allowed.  The
    trap cause is still checked.

Expectations that are BlackParrot platform behaviour (bp_mmu.sv, bp_pma.sv,
docs/platform_guide.md), not privileged-spec rules:
  (a) A=0, or D=0 on a store, raises a page fault: the core does not update
      the A/D bits (Svade style), which the spec permits.
  (b) A fetch from a high-I/O domain (address bits 39:33 nonzero) is an
      access fault.
  (c) Any access at or above 2^40, and any PTE whose PPN does not fit a 40-bit
      physical address (leaf PA, or a pointer to the next table), is an access
      fault.  For a PTE this is checked after its page-fault conditions.
  * 0x8000_0000 DRAM (the marker page of this test): no trap.

Excluded by default, enable with --io-data:
  platform_guide.md says a load/store to a high-I/O domain that is not enabled
  in hio_mask (reset value 0) faults.  The RTL does not fault today:
  bp_mmu.sv computes hio_fault_v but only applies it to the instruction access
  fault.  With --io-data those cases are generated and the test fails there.
"""
import argparse
import copy
import random

PT_BASE = 0x80010000          # .pt in misaligned_fault.ld
TRAMP = 0x80000004            # `ecall` right after `_start: j main`
MARKER = 0x5eed5eed5eed5eed
M64 = (1 << 64) - 1
PHYS_BITS = 40
PTE_V, PTE_R, PTE_W, PTE_X, PTE_U, PTE_G, PTE_A, PTE_D = (1 << i for i in range(8))
RWX = PTE_R | PTE_W | PTE_X
CAUSE_PF = {"load": 13, "store": 15, "fetch": 12}
CAUSE_AF = {"load": 5, "store": 7, "fetch": 1}
CAUSE_ECALL = {"U": 8, "S": 9, "M": 11}
MSTATUS_MASK = 0xE1800        # MPRV | SUM | MXR | MPP
MPP = {"U": 0x0, "S": 0x800, "M": 0x1800}


class MarkerPage(dict):
    """The data page read as a page table: every PTE is the marker word."""

    def get(self, key, default=None):
        return MARKER


class Pages:
    """Static page tables, keyed by physical page address."""

    def __init__(self):
        self.names = []
        self.pages = {}
        self.next = PT_BASE

    def new(self, name):
        addr = self.next
        self.next += 0x1000
        self.names.append((name, addr))
        self.pages[addr] = MarkerPage() if name == "data_page" else {}
        return addr


def canon(va):
    return va & M64 if va < (1 << 38) else (va | (M64 << 39)) & M64


def walk(pages, root, va, access, priv, sum_, mxr):
    """Reference model.  Returns (cause, tval, why); cause None means no trap."""
    pf, af = CAUSE_PF[access], CAUSE_AF[access]
    if (va >> 38) not in (0, (1 << 26) - 1):
        return pf, va, "noncanonical"
    vpn = [(va >> (12 + 9 * i)) & 0x1FF for i in range(3)]
    a = root
    for i in (2, 1, 0):
        pte = pages[a].get(vpn[i], 0)
        ppn = (pte >> 10) & ((1 << 44) - 1)
        leaf = bool(pte & (PTE_R | PTE_X))
        if not pte & PTE_V:
            return pf, va, "V=0"
        if pte & PTE_W and not pte & PTE_R:
            return pf, va, "W without R"
        if pte >> 54:
            return pf, va, "reserved bits"
        if not leaf and pte & (PTE_A | PTE_D | PTE_U):
            return pf, va, "non-leaf A/D/U"
        if leaf:
            break
        if i == 0:
            return pf, va, "non-leaf at L0"
        if ppn >> (PHYS_BITS - 12):
            return af, va, "pointer PPN out of range"
        a = ppn << 12
    u = bool(pte & PTE_U)
    if access == "fetch":
        ok = pte & PTE_X
    elif access == "load":
        ok = pte & PTE_R or (mxr and pte & PTE_X)
    else:
        ok = pte & PTE_W
    if not ok:
        return pf, va, "permission"
    if not (u if priv == "U" else (not u or (sum_ and access != "fetch"))):
        return pf, va, "U/SUM"
    if i and ppn & ((1 << 9 * i) - 1):
        return pf, va, "misaligned superpage"
    if not pte & PTE_A:
        return pf, va, "A=0"
    if access == "store" and not pte & PTE_D:
        return pf, va, "D=0 store"
    pa = (ppn << 12) & ~((1 << 12 + 9 * i) - 1) | (va & ((1 << 12 + 9 * i) - 1))
    if pa >> PHYS_BITS:
        return af, va, "leaf PA out of range"
    return None, va, "ok"


def umodel(addr, access, io_data):
    """Untranslated M-mode access to a DRAM/oob/I-O address."""
    if addr >> PHYS_BITS:
        return CAUSE_AF[access]
    if (addr >> 33) & 0x7F and (access == "fetch" or io_data):
        return CAUSE_AF[access]
    return None


class Gen:
    def __init__(self, seed, count, io_data):
        self.rng = random.Random(seed)
        self.count, self.io_data = count, io_data
        self.pg = Pages()
        self.root = self.pg.new("pt_root")
        self.p_a1, self.p_a0 = self.pg.new("pt_a1"), self.pg.new("pt_a0")
        self.p_b1 = self.pg.new("pt_b1")
        self.p_c1, self.p_c0 = self.pg.new("pt_c1"), self.pg.new("pt_c0")
        self.data = self.pg.new("data_page")
        self.data_off = self.data - 0x80000000
        # root slots: 1,2,3 are shared tables, the rest are handed out once
        self.slots = list(range(4, 512))
        self.rng.shuffle(self.slots)
        r = self.pg.pages
        r[self.root][1] = self.ptr(self.p_a1)
        r[self.root][2] = self.ptr(self.p_b1)
        r[self.root][3] = self.ptr(self.p_c1)
        r[self.p_a1][0] = self.ptr(self.p_a0)
        # pt_c0: ok leaves used by 'ok' pointer variants: (kind, priv) -> index
        self.c0 = {}
        for n, (kind, priv) in enumerate([(k, p) for k in ("data", "fetch") for p in "SU"]):
            self.c0[kind, priv] = n
            flags = PTE_V | PTE_A | (PTE_U if priv == "U" else 0)
            flags |= (PTE_R | PTE_W | PTE_D) if kind == "data" else PTE_X
            ppn = self.data >> 12 if kind == "data" else 0x80000
            r[self.p_c0][n] = ppn << 10 | flags
        self.k_next, self.m_next, self.mc_next = 0, 0, 1
        self.cases = []
        self.last = None

    @staticmethod
    def ptr(page):
        return (page >> 12) << 10 | PTE_V

    # ---- case builders -------------------------------------------------
    def leaf_target(self, kind, level, rng):
        """(ppn, va offset) of the physical word a leaf at `level` maps."""
        if kind == "fetch":
            return 0x80000, 4
        w = rng.randrange(512) * 8
        if level == 0:
            return self.data >> 12, w
        return 0x80000, self.data_off + w

    def good_flags(self, kind, priv, sum_, mxr, rng):
        f = PTE_V | PTE_A
        if kind == "fetch":
            f |= PTE_X | rng.choice([0, PTE_R, PTE_R | PTE_W])
            f |= PTE_U if priv == "U" else 0
        elif kind == "load":
            f |= rng.choice([PTE_R, PTE_R | PTE_W, PTE_R | PTE_X, RWX]) if not mxr or rng.random() < .5 else PTE_X
            f |= PTE_D if rng.random() < .5 else 0
            f |= PTE_U if priv == "U" or (sum_ and rng.random() < .5) else 0
        else:
            f |= rng.choice([PTE_R | PTE_W, RWX]) | PTE_D
            f |= PTE_U if priv == "U" or (sum_ and rng.random() < .5) else 0
        return f

    def random_flags(self, rng):
        f = PTE_V if rng.random() < .93 else 0
        f |= rng.choice([PTE_R, PTE_R | PTE_W, PTE_X, PTE_R | PTE_X, RWX, PTE_W, 0])
        f |= PTE_U if rng.random() < .5 else 0
        f |= PTE_A if rng.random() < .85 else 0
        f |= PTE_D if rng.random() < .6 else 0
        return f | (PTE_G if rng.random() < .2 else 0) | rng.randrange(4) << 8

    def mutate(self, pte, level, rng):
        m = rng.choice(["v", "u", "a", "d", "perm", "w", "res", "mis", "oob", "oob", "mis"])
        if m == "v":
            pte &= ~PTE_V
        elif m == "u":
            pte ^= PTE_U
        elif m == "a":
            pte &= ~PTE_A
        elif m == "d":
            pte &= ~PTE_D
        elif m == "perm":
            pte &= ~RWX
        elif m == "w":
            pte = pte & ~PTE_R | PTE_W
        elif m == "res":
            pte |= 1 << rng.randrange(54, 61)
        elif m == "mis" and level:
            pte |= 1 << (10 + rng.randrange(9 * level))
        else:   # "oob", or "mis" at level 0 (no superpage alignment): PPN beyond 40 bits
            pte |= 1 << (10 + rng.randrange(28, 44))
        return pte

    def build_leaf(self, kind, priv, sum_, mxr, access):
        rng = self.rng
        level = rng.choices([0, 1, 2], [5, 2, 2])[0]
        ppn, off = self.leaf_target(kind, level, rng)
        if rng.random() < .7:
            pte = ppn << 10 | self.good_flags(access, priv, sum_, mxr, rng)
            if rng.random() < .4:
                pte = self.mutate(pte, level, rng)
        else:
            pte = ppn << 10 | self.random_flags(rng)
        if level and pte & PTE_V and not pte & (PTE_R | PTE_X):
            # a pointer: make it walk the data page, not the code page
            pte = pte & ~(0xFFFFFFF << 10) | (self.data >> 12) << 10
        if level == 0:
            k = self.k_next
            self.k_next += 1
            self.pg.pages[self.p_a0][k] = pte
            base = 1 << 30 | 0 << 21 | k << 12
        elif level == 1:
            m = self.m_next
            self.m_next += 1
            self.pg.pages[self.p_b1][m] = pte
            base = 2 << 30 | m << 21
        else:
            s = self.slots.pop()
            self.pg.pages[self.root][s] = pte
            base = s << 30
        # off already holds the in-superpage / in-page offset, plus low bits
        return canon(base) | off, level, pte

    def build_ptrcase(self, kind, priv):
        """Faulty or good pointer PTE at root or L1; VA picks the leaf."""
        rng = self.rng
        where = rng.choice(["root", "l1", "l1"])
        var = rng.choice(["ok", "inv", "wonly", "adu", "res", "oob"] if where == "l1"
                         else ["inv", "wonly", "adu", "res", "oob"])
        pte = self.ptr(self.p_c0 if where == "l1" else self.p_c1)
        if var == "inv":
            pte &= ~PTE_V
        elif var == "wonly":
            pte |= PTE_W
        elif var == "adu":
            pte |= rng.choice([PTE_A, PTE_D, PTE_U, PTE_A | PTE_D | PTE_U])
        elif var == "res":
            pte |= 1 << rng.randrange(54, 61)
        elif var == "oob":
            pte |= 1 << (10 + rng.randrange(28, 44))
        k = self.c0[kind, priv]
        off = 4 if kind == "fetch" else rng.randrange(512) * 8
        if where == "l1":
            m = self.mc_next
            self.mc_next += 1
            self.pg.pages[self.p_c1][m] = pte
            va = 3 << 30 | m << 21 | k << 12 | off
        else:
            s = self.slots.pop()
            self.pg.pages[self.root][s] = pte
            va = canon(s << 30 | k << 12 | off)
        return va, "ptr-" + where + "-" + var, pte

    def draw(self, n):
        rng = self.rng
        c = {"n": n, "sum": rng.random() < .5, "mxr": rng.random() < .5}
        if rng.random() < .15:
            self.untranslated(c)
        else:
            self.translated(c)
        # mtval is not compared for addresses that do not fit in 39 bits
        # sign extended: the spec lets the implementation turn an invalid
        # address into another invalid address (see the header)
        c["skip_tval"] = (c["cause"] is not None
                          and canon(c["va"] & ((1 << 39) - 1)) != c["va"])
        return c

    def classes(self, c):
        """Coverage classes a case belongs to (see gen_cases)."""
        k = {("what", c["what"])} if c["mode"] != "trans" or c["level"] < 0 else set()
        out = ("ok" if c["cause"] in (None, 8, 9, 11) else
               "af" if c["cause"] in CAUSE_AF.values() else "pf")
        k.add(("combo", c["priv"], c["mode"], c["access"], out))
        if c["mode"] != "trans":
            return k
        k.add(("why", c["why"]))
        if c["why"] in ("permission", "U/SUM"):
            k.add(("deny", c["access"], c["why"], c["priv"]))
        if c["why"] == "ok" and c["level"] >= 0:
            k |= {("ok", c["level"], c["priv"]), ("ok", c["access"])}
        if c["access"] != "fetch":
            def cause_with(sum_, mxr):
                return walk(self.pg.pages, self.root, c["va"], c["access"], c["priv"], sum_, mxr)[0]
            if c["priv"] == "S" and cause_with(not c["sum"], c["mxr"]) != c["cause"]:
                k.add(("sum-decisive",))
            if cause_with(c["sum"], not c["mxr"]) != c["cause"]:
                k.add(("mxr-decisive",))
        return k

    def required(self):
        r = {("combo", p, "trans", a, o) for p in "SU" for a in CAUSE_PF for o in ("ok", "af", "pf")}
        r |= {("combo", "M", m, a, o) for m in ("phys", "mprv-m") for a in CAUSE_PF
              for o in ("ok", "af") if not (m == "mprv-m" and a == "fetch")}
        r |= {("why", w) for w in ("noncanonical", "V=0", "W without R", "reserved bits",
              "non-leaf A/D/U", "non-leaf at L0", "pointer PPN out of range", "permission",
              "U/SUM", "misaligned superpage", "A=0", "D=0 store", "leaf PA out of range", "ok")}
        r |= {("deny", a, "permission", p) for a in CAUSE_PF for p in "SU"}
        r |= {("ok", lvl, p) for lvl in (0, 1, 2) for p in "SU"} | {("ok", a) for a in CAUSE_PF}
        r |= {("sum-decisive",), ("mxr-decisive",)}
        r |= {("what", "ptr-%s-%s" % (w, v)) for w in ("root", "l1")
              for v in ("inv", "wonly", "adu", "res", "oob")} | {("what", "ptr-l1-ok"),
              ("what", "noncanonical")}
        r |= {("what", w) for w in ("dram", "oob")} | {("what", "tramp"), ("what", "io")}
        return r

    def gen_cases(self):
        """One case of every class first (a draw is kept only if it covers a
        class still missing), then random cases up to --count."""
        missing = self.required()
        for n in range(1, self.count + 1):
            for _ in range(2000):
                state = copy.deepcopy((self.pg, self.slots, self.k_next, self.m_next,
                                       self.mc_next, self.last))
                c = self.draw(n)
                if self.classes(c) & missing or not missing:
                    break
                self.pg, self.slots, self.k_next, self.m_next, self.mc_next, self.last = state
            else:
                raise RuntimeError("cannot cover %s" % sorted(missing))
            missing -= self.classes(c)
            self.cases.append(c)

    def translated(self, c):
        rng = self.rng
        access = rng.choice(["load", "load", "store", "store", "fetch", "fetch", "fetch"])
        kind = "fetch" if access == "fetch" else "data"
        priv = rng.choice("SU")
        c.update(access=access, priv=priv, mode="trans")
        reuse = self.last if self.last and self.last["kind"] == kind and rng.random() < .35 else None
        if reuse:
            lvl = reuse["level"]
            off = 4 if kind == "fetch" else (0 if lvl == 0 else self.data_off) + rng.randrange(512) * 8
            c["va"] = (reuse["va"] & ~((1 << 12 + 9 * lvl) - 1)) | off
            c["level"], c["what"] = lvl, "reuse " + reuse["what"]
        elif access != "fetch" and rng.random() < .08:
            base, _, _ = self.build_leaf("data", priv, c["sum"], c["mxr"], access)
            hi = ((base >> 39) ^ rng.randrange(1, 1 << 25)) & ((1 << 25) - 1)
            c["va"] = (hi << 39 | base & ((1 << 39) - 1)) & M64
            c["level"], c["what"] = -1, "noncanonical"
        elif rng.random() < .18:
            c["va"], c["what"], _ = self.build_ptrcase(kind, priv)
            c["level"] = -1
        else:
            c["va"], c["level"], pte = self.build_leaf(kind, priv, c["sum"], c["mxr"], access)
            c["what"] = "L%d pte=0x%x" % (c["level"], pte)
        c["kind"] = kind
        c["cause"], c["tval"], c["why"] = walk(self.pg.pages, self.root, c["va"], access,
                                               priv, c["sum"], c["mxr"])
        if c["cause"] is None and access == "fetch":
            c["cause"], c["tval"] = CAUSE_ECALL[priv], 0
        self.last = c if c["level"] >= 0 else None

    def untranslated(self, c):
        rng = self.rng
        access = rng.choice(["load", "store", "fetch"])
        mprv_m = access != "fetch" and rng.random() < .4   # MPRV=1 with MPP=M
        kinds = ["dram", "oob", "oob"] if access != "fetch" else ["tramp", "io", "io"]
        if access != "fetch" and self.io_data:
            kinds.append("io")
        k = rng.choice(kinds)
        if k == "dram":
            addr = self.data + rng.randrange(512) * 8
        elif k == "tramp":
            addr = TRAMP
        elif k == "io":
            addr = rng.randrange(1, 1 << (5 if access == "fetch" else 7)) << 33 | 0x1000 * rng.randrange(256)
        else:
            addr = (rng.randrange(1, 1 << 24) << 40 | 0x80000000 + rng.randrange(256) * 8) & M64
            if rng.random() < .4:
                addr = (1 << rng.randrange(40, 64)) | self.data
        c.update(access=access, priv="M", mode="mprv-m" if mprv_m else "phys", va=addr,
                 what=k, level=-1, kind="phys")
        c["cause"] = umodel(addr, access, self.io_data)
        c["tval"], c["why"] = addr, "untranslated"
        if access == "fetch" and c["cause"] is None:
            c["cause"], c["tval"] = CAUSE_ECALL["M"], 0
        self.last = None

    # ---- output --------------------------------------------------------
    def case_asm(self, c):
        a, n = c["access"], c["n"]
        sumb, mxrb = c["sum"], c["mxr"]
        cause = -1 if c["cause"] is None else c["cause"]
        tval = c["tval"] if c["cause"] is not None else 0
        what = "%s %s %s va=0x%x %s" % (a, c["priv"], c["mode"], c["va"], c["what"])
        if c["mode"] == "trans":
            what += " sum=%d mxr=%d" % (sumb, mxrb)
        exp = "no trap" if c["cause"] is None else "cause %d tval 0x%x" % (cause, tval)
        out = ["    # %d: %s -> %s" % (n, what, exp),
               "    li      s7, %d" % n,
               "    li      s9, %d" % cause,
               "    li      s10, 0x%x" % (tval & M64),
               "    li      s4, %d" % (0 if c["skip_tval"] else -1),
               # s8: expected mepc, the faulting instruction
               "    li      s8, 0x%x" % c["va"] if a == "fetch" else "    la      s8, 2f",
               "    la      s11, 1f",
               "    li      t0, 0x%x" % MSTATUS_MASK,
               "    csrc    mstatus, t0"]
        if a == "fetch":
            ms = MPP[c["priv"]] if c["mode"] != "phys" else 0x1800
            ms |= (sumb << 18) | (mxrb << 19) if c["mode"] == "trans" else 0
            out += ["    li      t0, 0x%x" % ms, "    csrs    mstatus, t0",
                    "    li      t0, 0x%x" % c["va"], "    csrw    mepc, t0", "    mret"]
        else:
            ms = 0
            if c["mode"] == "trans":
                ms = 0x20000 | MPP[c["priv"]] | (sumb << 18) | (mxrb << 19)
            elif c["mode"] == "mprv-m":
                ms = 0x20000 | 0x1800 | (sumb << 18) | (mxrb << 19)
            if ms:
                out += ["    li      t0, 0x%x" % ms, "    csrs    mstatus, t0"]
            out.append("    li      a1, 0x%x" % c["va"])
            out.append("2:  ld      a0, 0(a1)" if a == "load" else "2:  sd      s6, 0(a1)")
            if c["cause"] is not None:
                out.append("    j       fail_notrap")
            elif a == "load":
                out.append("    bne     a0, s6, fail_data")
        out.append("1:")
        return out

    def emit(self, argv_note):
        L = []
        w = L.append
        w("# Memory fault fuzz (black-parrot#312, #313): %d generated cases." % self.count)
        w("# GENERATED by src/gen_mem_fault_fuzz.py %s -- do not edit." % argv_note)
        w("#")
        w("# Every case does one load, store or instruction fetch, either translated")
        w("# through a static Sv39 table (data: M-mode with MPRV=1, MPP=S or U; fetch: a")
        w("# real S/U-mode mret to the target) or untranslated in M-mode.  Valid and")
        w("# faulting targets are mixed.  The M-mode trap handler compares mcause, mtval")
        w("# and mepc (the faulting instruction) with the values the generator's")
        w("# reference model computed, then resumes at the next case.  A load that does")
        w("# not trap must return the marker word.  A fetch that does not fault")
        w("# executes an `ecall`, so every fetch case ends in the handler.")
        w("#")
        w("# Layout: the tables live in .pt (misaligned_fault.ld, 0x%x) as static data;" % PT_BASE)
        for name, addr in self.pg.names:
            w("#   0x%x %s" % (addr, name))
        w("# Leaves map the marker page (data) or the `ecall` at 0x%x (fetch), at 4K," % TRAMP)
        w("# 2M and 1G level; superpage leaves use PA 0x80000000 and select the target")
        w("# through the VA offset.  Every translated case uses its own VA page, or")
        w("# reuses one earlier case's page to exercise TLB hits.")
        w("#")
        w("# Result: store 0 to the host finish register -> PASS, otherwise the 1-based")
        w("# case number (255: setup check) is the low byte.  A failure also prints")
        w("# seven 64-bit hex words on the host putint port: case, expected cause,")
        w("# got cause (-1: no trap, -2: load data mismatch), expected mtval, got mtval,")
        w("# expected mepc, got mepc.")
        w("# The comment above each case tells the parameters and expected result.")
        w("#")
        w("# Privileged-spec rules checked: Sv39 walk (canonical VA, V/R/W/X/U, reserved")
        w("# bits 60:54, non-leaf A/D/U, misaligned superpage, SUM, MXR), mcause, mtval")
        w("# = faulting VA, mepc.  mtval is not compared when the address does not fit")
        w("# in 39 bits sign extended: the spec lets an implementation turn such an")
        w("# address into another invalid address.  The trap cause is still checked.")
        w("#")
        w("# BlackParrot platform behaviour checked (not spec rules): (a) A=0, or D=0")
        w("# on a store, raises a page fault, the core does not update A/D (Svade")
        w("# style); (b) a fetch from a high-I/O domain is an access fault; (c) any")
        w("# access at or above 2^40, and any PTE whose PPN does not fit a 40-bit")
        w("# physical address, is an access fault.")
        w("#")
        if self.io_data:
            w("# (--io-data) loads/stores to I/O domains not enabled in hio_mask ARE")
            w("# generated and expected to fault (platform_guide.md).")
        else:
            w("# Excluded: loads/stores to a high-I/O domain not enabled in hio_mask.")
            w("# platform_guide.md says they fault, but the RTL does not today")
            w("# (bp_mmu.sv applies hio_fault_v to the instruction access fault only).")
            w("# Generate them with --io-data.")
        w("")
        w("    .section .text.init, \"ax\"")
        w("    .globl _start")
        w("_start:")
        w("    j       main")
        w("tramp:                           # PA 0x%x, fetch target" % TRAMP)
        w("    ecall")
        w("")
        w("    .align 2")
        w("trap_handler:")
        w("    li      t0, 0x20000")
        w("    csrc    mstatus, t0         # MPRV off")
        w("    csrr    s2, mcause")
        w("    csrr    s3, mtval")
        w("    csrr    s1, mepc")
        w("    bne     s2, s9, fail")
        w("    xor     t0, s3, s10")
        w("    and     t0, t0, s4          # s4 = 0: mtval not compared (see header)")
        w("    bnez    t0, fail")
        w("    bne     s1, s8, fail        # mepc")
        w("    li      t0, 0x1800")
        w("    csrs    mstatus, t0         # MPP = M")
        w("    csrw    mepc, s11")
        w("    mret")
        w("")
        w("main:")
        w("    la      t0, pt_root")
        w("    li      t1, 0x%x" % PT_BASE)
        w("    bne     t0, t1, fail_layout")
        w("    la      t0, tramp")
        w("    li      t1, 0x%x" % TRAMP)
        w("    bne     t0, t1, fail_layout")
        w("    la      t0, trap_handler")
        w("    csrw    mtvec, t0")
        w("    csrw    medeleg, zero")
        w("    csrw    mideleg, zero")
        w("    li      s6, 0x%x" % MARKER)
        w("    li      t0, 0x%x" % (8 << 60 | self.root >> 12))
        w("    csrw    satp, t0")
        w("    sfence.vma")
        w("")
        for c in self.cases:
            L += self.case_asm(c)
        w("    li      t0, 0x20000")
        w("    csrc    mstatus, t0         # MPRV off")
        w("    li      s7, 0               # PASS")
        w("    j       done")
        w("fail:")
        w("    li      t0, 0x20000")
        w("    csrc    mstatus, t0         # MPRV off")
        w("    li      t0, 0x00105000      # host putint")
        for r in ("s7", "s9", "s2", "s10", "s3", "s8", "s1"):
            w("    sd      %s, 0(t0)" % r)
        w("done:")
        w("    li      t0, 0x00102000      # host finish")
        w("    sd      s7, 0(t0)")
        w("2:  j       2b")
        w("fail_layout:")
        w("    li      s7, 255")
        w("    li      s1, 0")
        w("    j       fail")
        w("fail_notrap:                     # expected a trap, none taken")
        w("    li      s2, -1")
        w("    li      s3, 0")
        w("    li      s1, 0")
        w("    j       fail")
        w("fail_data:                       # load returned the wrong word")
        w("    mv      s2, a0")
        w("    li      s3, -2")
        w("    li      s1, 0")
        w("    j       fail")
        w("    .section .pt, \"aw\", @progbits")
        for name, addr in self.pg.names:
            w("    .align 12")
            w("%s:" % name)
            if name == "data_page":
                w("    .rept 512")
                w("    .quad 0x%x" % MARKER)
                w("    .endr")
                continue
            pos = 0
            for idx, pte in sorted(self.pg.pages[addr].items()):
                if pte is None:
                    continue
                if idx > pos:
                    w("    .zero %d" % ((idx - pos) * 8))
                w("    .quad 0x%x" % pte)
                pos = idx + 1
            if pos < 512:
                w("    .zero %d" % ((512 - pos) * 8))
        return "\n".join(L) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--count", type=int, default=100, help="cases, at most 250")
    ap.add_argument("--io-data", action="store_true",
                    help="also generate loads/stores to high-I/O domains not enabled in "
                         "hio_mask; platform_guide.md says they fault, the RTL does not "
                         "(bp_mmu.sv applies hio_fault_v to fetches only), so the test "
                         "fails on those cases")
    ap.add_argument("-o", "--output", default="-")
    a = ap.parse_args()
    if not 1 <= a.count <= 250:
        ap.error("--count must be between 1 and 250")
    g = Gen(a.seed, a.count, a.io_data)
    g.gen_cases()
    text = g.emit("--seed %d --count %d%s" % (a.seed, a.count, " --io-data" if a.io_data else ""))
    if a.output == "-":
        print(text, end="")
    else:
        with open(a.output, "w") as f:
            f.write(text)


if __name__ == "__main__":
    main()
