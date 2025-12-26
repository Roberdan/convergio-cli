# Studio View Implementation Plan

## Immediate Fixes (Priority)
1. Fix Gantt chart black bars - CSS not being applied correctly
2. Add admin list to .env and enable Studio 3 access for roberdan@microsoft.com

## Phase 1: Studio View Foundation
- Add ADMIN_EMAILS to .env
- Create Studio sidebar navigation
- Implement Org Chart visualization with D3.js
- Per-person workload indicators (FTE colors)

## Phase 2: Drill-down Views
- Person detail view (their engagements, timeline)
- Engagement navigation from org chart
- Activity navigation

## Phase 3: Caching
- Flask-Caching with Redis backend
- 5-15 min TTL for ADO data
- Static config caching (studios, org charts)

Exiting to fix urgent issues first.
