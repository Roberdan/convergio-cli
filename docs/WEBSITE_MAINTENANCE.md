# Convergio Website Maintenance Guide

## Overview

The Convergio website is a static HTML/CSS/JS site located in the `/website` directory. It showcases the CLI, Education Edition, Native App, and provides competitor comparison.

## Directory Structure

```
website/
├── index.html          # Main HTML file
├── css/
│   └── styles.css      # All styles
├── js/
│   └── main.js         # Interactive behavior
└── assets/
    └── favicon.svg     # Site icon
```

## Running Locally

```bash
cd website
python3 -m http.server 8080
# Open http://localhost:8080
```

## Sections to Keep Updated

### 1. Agent Count
- **Location**: Hero section, Agents section, Compare table
- **Current**: 60+ agents
- **Update when**: New agents are added to the system

### 2. Education Edition - Maestri Count
- **Location**: Education section
- **Current**: 17 Maestri
- **Update when**: New Maestri are added to the education pack

### 3. Comparison Table
- **Location**: Compare section (`#compare`)
- **Features to verify**:
  - Agent count is accurate
  - Local processing status is correct
  - Apple Intelligence status matches current macOS support
  - Competitor pricing is current

### 4. Technology Section
- **Location**: Technology section (`#technology`)
- **Update when**:
  - New local models are supported
  - Apple Silicon generations change (M5, M6, etc.)
  - macOS version requirements change

### 5. Installation Commands
- **Location**: Installation section (`#installation`)
- **Verify**: Homebrew tap and install commands are correct

### 6. Version References
- **Location**: Terminal preview in hero
- **Current**: v2.0.0
- **Update when**: Major version releases

## Pre-Release Checklist

The app-release-manager agent includes a Website Sync Check (Sub-agent B6) that verifies:

1. [ ] Agent count matches actual AGENTS.md count
2. [ ] Version number in terminal preview matches VERSION file
3. [ ] Education Maestri count matches education pack manifest
4. [ ] Comparison data is accurate (check competitor features)
5. [ ] Installation commands work correctly
6. [ ] All internal links are valid
7. [ ] No broken images or assets

## Automated Checks

When running a release, the following are automatically verified:

```bash
# Check agent count in website matches codebase
grep -o "60+" website/index.html | wc -l  # Should match if used consistently

# Check version consistency
grep -oE "v[0-9]+\.[0-9]+\.[0-9]+" website/index.html | sort -u
cat VERSION

# Verify local server works
cd website && python3 -m http.server 8080 &
curl -s http://localhost:8080 | grep -q "Convergio" && echo "OK"
```

## Style Guide

### Colors
- Primary: `#6366f1` (Indigo)
- Secondary: `#10b981` (Emerald/Green)
- Accent: `#f59e0b` (Amber)
- Background: `#0a0a0f` (Near Black)

### Typography
- Sans: Inter
- Mono: JetBrains Mono

### Accessibility
- Minimum contrast ratio: 4.5:1
- Focus indicators on all interactive elements
- Semantic HTML structure
- ARIA labels where needed

## Content Guidelines

### Education Section
- Emphasize accessibility features prominently
- Use person-first language
- Highlight WCAG compliance
- Feature Jenny (accessibility champion agent)

### Comparison Section
- Be honest about limitations
- Focus on unique differentiators
- Update competitor features quarterly

## Deployment

The website is currently static and can be deployed to:
- GitHub Pages
- Vercel
- Netlify
- Any static hosting

### Future Enhancements (Pending)

| ID | Task | Status |
|----|------|--------|
| W5 | Italian language support | Pending |
| W6 | Product tabs in hero | Pending |
| W7 | Testimonials section | Pending |
| W8 | Schema.org for each edition | Pending |

---

**Last Updated**: 2025-12-25
