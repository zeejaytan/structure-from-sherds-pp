# 10: Dead-code removal (audit-confirmed, uncompiled or uncalled)

**Answers:** S1

**Blocked by:** nothing — deletion only; rebuild is the test

**Status:** resolved 2026-10-02 — both groups deleted, in-container rebuild
exit 0, binary verified current (no source newer). Per-item proof stands
(uncompiled files cannot execute; uncalled code verified-again-uncalled
at deletion time). Default-path numbers waived by construction: every
deletion is unreachable code or print-only.

**Needs-eye:** none — no behavior exists to witness; the proof is the
build plus the pre-deletion reachability cites below (re-checked at build
time, not trusted from this ticket).

## Why this ticket exists

Ticket 05's liveness pass found code that cannot execute. Dead code in a
matcher is not neutral: the backup file defines the SAME entry symbol and
would be a duplicate-symbol collision if ever compiled (load-bearing
hygiene, not tidiness). Delete in two groups — never mixed, so a failure
attributes cleanly.

## Group A — uncompiled files (delete files, rebuild)

- `class/multi_hypothesis_optimizer_backup.cpp` (shadow entry symbol)
- `class/multi_hypothesis_optimizer_test_patch.cpp` (patch draft)
- `class/auto_agglomerative_assembler.{cpp,h}` (absent from CMakeLists)
- `class/physics_based_optimizer.{cpp,h}` + `class/physics_integration.cpp`
  (unbuilt shadows of the compiled physics pair)
- Root `surface_overlap_detection.{cpp,h}` (AGENTS.md already records
  supersession; nothing includes it)
- NOT in this group: `main.cpp` / `main_headless.cpp` (alternate mains —
  recorded, left alone), `*_branch_*.cpp` (gitignored generated copies)

## Group B — uncalled code in live files (delete lines, rebuild)

- `ProfileCheckingWithInlier` def (`feature_matching.cpp:965`) + decl
  (`feature_matching.h:98`) — zero callers
- mode-1 `Q_size` branch (`feature_matching.cpp:1588–1591` — all three
  callers pass mode=0) — delete branch or prove a caller; no third option
- Hub-guidance disabled comment block (`main_headless_correct.cpp:442–456`)
- Dead `EnhancedStateManager manager;` (`main_headless_correct.cpp:260`)
- NOT in this group: commented `ExclusivelyPickEdge` remnants (done in
  ticket 02), disabled `CheckOpposingNormals` (:1915–1919 — behavior-adjacent,
  needs its own reason to remove; record here, delete elsewhere)

## Acceptance criteria

- [ ] Group A deleted; full in-container rebuild (build_sif pattern) exit 0
- [ ] Group B deleted line-by-line; rebuild exit 0; binary strings checked
      for no orphan references where applicable
- [ ] Reachability re-checked at build time for every item (a caller added
      since the audit voids that item's deletion — check, don't assume)
- [ ] Default-path Pot_A/Juglet numbers waived by construction ONLY with
      the per-item proof (uncompiled → cannot execute; uncalled →
      verified-again-uncalled); any item touching a live path gets the
      full per-pair A/B instead
