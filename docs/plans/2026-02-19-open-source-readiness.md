# Open Source Readiness Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Prepare the kinemachina-sfx-controller repo for public open source contributions.

**Architecture:** Add standard OSS files (LICENSE, CONTRIBUTING, CODE_OF_CONDUCT), GitHub templates and CI, move internal docs into `docs/`, and harden `.gitignore`.

**Tech Stack:** GitHub Actions, PlatformIO CLI, Markdown

---

### Task 1: Add MIT LICENSE file

**Files:**
- Create: `LICENSE`

**Step 1: Create LICENSE**

Create `LICENSE` at the project root with the standard MIT license text:

```
MIT License

Copyright (c) 2026 Tom Davidson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

**Step 2: Commit**

```bash
git add LICENSE
git commit -m "Add MIT license"
```

---

### Task 2: Add CODE_OF_CONDUCT.md

**Files:**
- Create: `CODE_OF_CONDUCT.md`

**Step 1: Create CODE_OF_CONDUCT.md**

Use Contributor Covenant v2.1 (the standard). Full text at https://www.contributor-covenant.org/version/2/1/code_of_conduct/. Set contact method to GitHub Issues.

**Step 2: Commit**

```bash
git add CODE_OF_CONDUCT.md
git commit -m "Add Contributor Covenant code of conduct"
```

---

### Task 3: Add CONTRIBUTING.md

**Files:**
- Create: `CONTRIBUTING.md`

**Step 1: Create CONTRIBUTING.md**

Target audience: experienced embedded devs. Include:

1. **Prerequisites** — PlatformIO CLI (or VS Code extension), ESP32-S3 board (optional, for hardware testing)
2. **Build & Test** — exact commands:
   - `pio run` (build for ESP32-S3)
   - `pio test -e native -v` (run native unit tests)
   - `pio run -t upload && pio device monitor` (flash + serial monitor)
3. **Architecture** — brief overview linking to `CLAUDE.md` for details. Mention the controller pattern, FreeRTOS task/queue pattern, and initialization order.
4. **Making Changes** — fork, branch from `main`, one feature per PR, write tests for new logic
5. **Code Style** — Arduino/C++ conventions used in the project: PascalCase for classes, camelCase for methods/variables, `UPPER_SNAKE` for constants/pins. Effects are string-identified. No dynamic allocation in hot paths.
6. **PR Process** — descriptive title, fill in the PR template, ensure `pio test -e native` passes, CI must be green
7. **Good First Issues** — mention that issues tagged `good first issue` are a good starting point

**Step 2: Commit**

```bash
git add CONTRIBUTING.md
git commit -m "Add contributing guidelines"
```

---

### Task 4: Add GitHub issue and PR templates

**Files:**
- Create: `.github/ISSUE_TEMPLATE/bug_report.md`
- Create: `.github/ISSUE_TEMPLATE/feature_request.md`
- Create: `.github/PULL_REQUEST_TEMPLATE.md`

**Step 1: Create bug report template**

`.github/ISSUE_TEMPLATE/bug_report.md`:

```markdown
---
name: Bug Report
about: Report a bug to help improve the project
labels: bug
---

## Description

A clear description of the bug.

## Steps to Reproduce

1. ...
2. ...

## Expected Behavior

What should happen.

## Actual Behavior

What actually happens.

## Environment

- Board: (e.g., ESP32-S3-DevKitC-1)
- PlatformIO version:
- Firmware version/commit:
- LED configuration: (strip length, matrix size)
```

**Step 2: Create feature request template**

`.github/ISSUE_TEMPLATE/feature_request.md`:

```markdown
---
name: Feature Request
about: Suggest a new feature or enhancement
labels: enhancement
---

## Problem

What problem does this solve?

## Proposed Solution

Describe your proposed approach.

## Alternatives Considered

Any other approaches you considered.
```

**Step 3: Create PR template**

`.github/PULL_REQUEST_TEMPLATE.md`:

```markdown
## Summary

What does this PR do?

## Changes

- ...

## Testing

- [ ] `pio test -e native -v` passes
- [ ] `pio run` builds successfully
- [ ] Tested on hardware (if applicable)

## Notes

Any additional context.
```

**Step 4: Commit**

```bash
git add .github/
git commit -m "Add GitHub issue and PR templates"
```

---

### Task 5: Add GitHub Actions CI workflow

**Files:**
- Create: `.github/workflows/ci.yml`

**Step 1: Create CI workflow**

`.github/workflows/ci.yml` with two jobs:

1. **test** — runs `pio test -e native -v`
2. **build** — runs `pio run` (ESP32 target build check)

Both use:
- `ubuntu-latest`
- `actions/checkout@v4`
- `actions/cache@v4` for `~/.platformio` (keyed on `platformio.ini` hash)
- `pip install platformio` to get `pio`
- Trigger on `push` to `main` and all `pull_request` events

**Step 2: Verify workflow syntax**

```bash
# No local validation needed — GitHub will validate on push
# But review the YAML visually for correctness
```

**Step 3: Commit**

```bash
git add .github/workflows/ci.yml
git commit -m "Add GitHub Actions CI for native tests and build check"
```

---

### Task 6: Move internal docs to docs/

**Files:**
- Move: `ENHANCEMENT_PLAN.md` -> `docs/ENHANCEMENT_PLAN.md`
- Move: `EXECUTIVE_SUMMARY.md` -> `docs/EXECUTIVE_SUMMARY.md`
- Move: `PRODUCT_SPECIFICATION.md` -> `docs/PRODUCT_SPECIFICATION.md`
- Move: `BOM.md` -> `docs/BOM.md`
- Move: `FREERTOS_OPTIMIZATION.md` -> `docs/FREERTOS_OPTIMIZATION.md`
- Move: `FREERTOS_QUEUES_IMPLEMENTED.md` -> `docs/FREERTOS_QUEUES_IMPLEMENTED.md`
- Move: `MOTOR_CONTROLLER_MQTT_MIGRATION_PLAN.md` -> `docs/MOTOR_CONTROLLER_MQTT_MIGRATION_PLAN.md`

**Step 1: Move files**

```bash
mkdir -p docs
git mv ENHANCEMENT_PLAN.md docs/
git mv EXECUTIVE_SUMMARY.md docs/
git mv PRODUCT_SPECIFICATION.md docs/
git mv BOM.md docs/
git mv FREERTOS_OPTIMIZATION.md docs/
git mv FREERTOS_QUEUES_IMPLEMENTED.md docs/
git mv MOTOR_CONTROLLER_MQTT_MIGRATION_PLAN.md docs/
```

**Step 2: Commit**

```bash
git commit -m "Move internal planning docs to docs/"
```

---

### Task 7: Harden .gitignore

**Files:**
- Modify: `.gitignore`

**Step 1: Add standard entries**

Append to `.gitignore`:

```
# OS artifacts
.DS_Store
Thumbs.db

# Editor swap files
*.swp
*.swo
*~
```

**Step 2: Commit**

```bash
git add .gitignore
git commit -m "Add OS and editor artifacts to .gitignore"
```

---

### Task 8: Final verification

**Step 1: Confirm repo structure looks clean**

```bash
ls -1 *.md
# Expected: CLAUDE.md, CONTRIBUTING.md, CODE_OF_CONDUCT.md, MQTT_API.md, README.md

ls -1 docs/*.md
# Expected: BOM.md, ENHANCEMENT_PLAN.md, EXECUTIVE_SUMMARY.md, FREERTOS_OPTIMIZATION.md,
#           FREERTOS_QUEUES_IMPLEMENTED.md, MOTOR_CONTROLLER_MQTT_MIGRATION_PLAN.md,
#           PRODUCT_SPECIFICATION.md

ls .github/
# Expected: ISSUE_TEMPLATE/, PULL_REQUEST_TEMPLATE.md, workflows/
```

**Step 2: Run native tests to confirm nothing broke**

```bash
pio test -e native -v
```

Expected: All tests pass.

**Step 3: Push**

```bash
git push
```
