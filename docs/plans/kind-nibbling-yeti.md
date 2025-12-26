# Executive Reports Web App - Implementation Plan

**Date:** 2025-12-18
**Status:** Ready for Implementation
**Goal:** Dynamic web interface for executive reports with studio/industry selection

## Summary

Transform the static executive report generator into a dynamic Flask web app where users can select studio and industry from dropdowns and generate reports on-demand.

## Architecture

```
[Web UI] --> [POST /api/executive-reports/generate]
                |
                v
    [ExecutiveReportService]
                |
                +---> [Load studio config from YAML]
                +---> [Fetch engagements from ADO (fresh)]
                +---> [Optional: Generate LLM narratives]
                +---> [Generate HTML]
                |
                v
    [Return HTML Response]
```

## Implementation Phases

### Phase 1: Service Layer
**File:** `/scripts/python/executive_reports/service.py` (NEW)

Create `ExecutiveReportService` class that wraps `generate_report.py`:
- `__init__(studio)` - load studio config
- `load_engagements_from_ado()` - fetch fresh from ADO
- `generate_html_report(engagements, include_llm, theme)` - generate HTML
- `get_available_studios()` - list available studios
- `get_industries_for_studio(studio)` - list industries

### Phase 2: Flask Blueprint
**File:** `/webapp/routes/executive_reports.py` (NEW)

Endpoints:
- `GET /api/executive-reports/studios` - list available studios
- `GET /api/executive-reports/studios/<id>/industries` - list industries for studio
- `POST /api/executive-reports/generate` - generate report on-demand
- `GET /api/executive-reports/view/<studio_id>` - view report with query params
- `GET /api/executive-reports/` - UI selector page

### Phase 3: Blueprint Registration
**Files to modify:**
- `/webapp/app.py` - register blueprint in `_register_blueprints()`
- `/webapp/routes/__init__.py` - export new module

### Phase 4: UI Page
**File:** `/webapp/static/executive-reports.html` (NEW)

Simple HTML page with:
- Studio dropdown (studio3, studio4)
- Industry dropdown (dynamic based on studio)
- Options: Include AI Narratives, Self-Contained, Theme
- Generate button
- Report display area (iframe or new tab)

### Phase 5: API Entry Point
**File:** `/scripts/python/executive_reports/generate_report.py` (MINIMAL CHANGE)

Add single function:
```python
def generate_report_api(engagements, studio, llm_narratives=None, self_contained=True) -> str:
    """API entry point for generating reports without CLI."""
```

## File Structure

```
webapp/
├── routes/
│   ├── __init__.py              # Add executive_reports
│   ├── executive_reports.py      # NEW: Blueprint
│   └── ... existing routes ...
├── static/
│   └── executive-reports.html    # NEW: UI page
└── app.py                        # Register blueprint

scripts/python/executive_reports/
├── service.py                    # NEW: API-friendly wrapper
├── generate_report.py            # Add API entry point
└── studios/
    ├── studio3.yml
    └── studio4.yml
```

## Security

- Use existing `@admin_required()` decorator
- Validate studio ID against allowed list
- Apply Flask-Limiter rate limiting
- No path traversal - fixed config paths

## Execution Order

1. [ ] Create `service.py` with `ExecutiveReportService` class
2. [ ] Add `generate_report_api()` to `generate_report.py`
3. [ ] Create `executive_reports.py` blueprint
4. [ ] Register blueprint in `app.py` and `__init__.py`
5. [ ] Create `executive-reports.html` UI page
6. [ ] Test end-to-end
7. [ ] Update `docs/plans/ExecReportPlan.md` with progress

## Notes

- Use fresh ADO data (NO-CACHE policy)
- LLM narratives optional via checkbox
- Self-contained HTML by default (embedded logos)
- Follow existing Flask patterns from `webapp/routes/reports.py`
