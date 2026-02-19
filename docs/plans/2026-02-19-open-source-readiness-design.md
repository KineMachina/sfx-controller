# Open Source Readiness Design

**Date:** 2026-02-19
**Status:** Approved

## Goal

Prepare the kinemachina-sfx-controller repo for public open source contributions from experienced embedded developers.

## Decisions

- **License:** MIT
- **Target contributors:** Experienced embedded devs (familiar with PlatformIO, ESP32, FreeRTOS)
- **CI:** GitHub Actions running native tests + ESP32 build check on every push/PR
- **Docs:** Move internal planning docs to `docs/`, keep contributor-facing docs at root

## New Files

| File | Purpose |
|------|---------|
| `LICENSE` | MIT license |
| `CONTRIBUTING.md` | Build, test, PR process, code conventions |
| `CODE_OF_CONDUCT.md` | Contributor Covenant v2.1 |
| `.github/ISSUE_TEMPLATE/bug_report.md` | Structured bug reports |
| `.github/ISSUE_TEMPLATE/feature_request.md` | Feature requests |
| `.github/PULL_REQUEST_TEMPLATE.md` | PR checklist |
| `.github/workflows/ci.yml` | PlatformIO native tests + build check |

## File Moves (root -> docs/)

- `ENHANCEMENT_PLAN.md`
- `EXECUTIVE_SUMMARY.md`
- `PRODUCT_SPECIFICATION.md`
- `BOM.md`
- `FREERTOS_OPTIMIZATION.md`
- `FREERTOS_QUEUES_IMPLEMENTED.md`
- `MOTOR_CONTROLLER_MQTT_MIGRATION_PLAN.md`

**Remain at root:** `README.md`, `CLAUDE.md`, `MQTT_API.md`

## .gitignore Updates

Add standard entries: `.DS_Store`, `*.swp`, `*.swo`, `*~`, `Thumbs.db`.

## CI Workflow

- Trigger: push to `main`, all PRs
- Job 1: `pio test -e native -v` (native unit tests)
- Job 2: `pio run` (ESP32 build check)
- Uses `actions/cache` for PlatformIO packages
