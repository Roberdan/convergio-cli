# ConvergioWeb - Development Progress Report
## December 27, 2025

---

## Overview

ConvergioWeb is an educational web application featuring AI-powered tutoring through specialized "Maestri" (teachers). The app uses OpenAI's Realtime API for voice interactions, providing an immersive learning experience for students.

---

## Completed Features

### Core Learning System

#### 1. FSRS Spaced Repetition Algorithm
- **File**: `/web/src/lib/learning/fsrs.ts`
- Implemented Free Spaced Repetition Scheduler (FSRS) algorithm
- Optimal memory retention scheduling for learning content
- Card state management with difficulty, stability, and retrievability

#### 2. Mastery Learning System
- **File**: `/web/src/lib/learning/mastery.ts`
- 80% mastery gate requirement before advancing
- Progress tracking per topic/subject
- Adaptive difficulty based on student performance

#### 3. Quiz System
- **File**: `/web/src/components/quiz/quiz-view.tsx`
- Interactive quiz interface
- Multiple question types support
- Real-time feedback and scoring
- Connected to main page for seamless navigation

### Voice & AI Interaction

#### 4. Maestri System
- **File**: `/web/src/data/index.ts`
- 7 specialized AI tutors with unique personalities:
  - **Pitagora**: Mathematics
  - **Dante**: Italian Language & Literature
  - **Newton**: Science & Physics
  - **Cleopatra**: History
  - **Shakespeare**: English
  - **Frida**: Art & Creativity
  - **Archimede**: Logic & Problem Solving
- Each maestro has:
  - Custom system prompt
  - Voice personality instructions
  - Subject-specific tools
  - Unique avatar and color scheme

#### 5. Voice Session Management
- **File**: `/web/src/lib/hooks/use-voice-session.ts`
- OpenAI Realtime API integration
- WebSocket-based communication
- Audio capture and playback
- Tool execution during conversations

#### 6. Voice Personality Per Maestro
- Each maestro has unique voice characteristics
- Custom speech patterns and teaching styles
- Engaging, age-appropriate communication

#### 7. Auto-Greeting
- Maestri automatically greet students when session starts
- Contextual introductions based on subject

#### 8. Language Settings
- **File**: `/web/src/components/settings/settings-view.tsx`
- User can select preferred language (IT, EN, ES, FR, DE)
- Maestri always speak in the selected language
- Critical language requirement enforced in system prompt

### AI Tools (Main Tools - Available to ALL Maestri)

All maestri have access to these 9 tools, which they use proactively during lessons:

| # | Tool | Description |
|---|------|-------------|
| 1 | **create_mindmap** | Interactive hierarchical mind maps |
| 2 | **create_diagram** | Flowcharts, sequences, ERD, state diagrams, mind maps (Mermaid) |
| 3 | **create_quiz** | Interactive quizzes with multiple question types |
| 4 | **create_flashcard** | Spaced repetition flashcard decks |
| 5 | **create_chart** | Data visualization (line, bar, pie, scatter, area) |
| 6 | **show_formula** | LaTeX math formulas |
| 7 | **run_code** | Execute Python/JavaScript code |
| 8 | **web_search** | Safe, kid-filtered web search |
| 9 | **capture_homework** | Webcam capture for homework help |

#### 9. Mind Maps (Primary Tool)
- **File**: `/web/src/lib/hooks/use-voice-session.ts` (lines 249-294)
- **File**: `/web/src/components/tools/mindmap-renderer.tsx`
- **Primary educational tool** for visual learning
- Two implementations:
  - `create_mindmap` - Interactive hierarchical mind maps
  - `create_diagram` with type 'mindmap' - Mermaid.js based diagrams
- Features:
  - Contextual to conversation topics
  - Printable output for offline study
  - Accessible design (WCAG compliant)
  - Supports nested nodes with colors and icons
- **Proactively used** by maestri when:
  - Introducing complex topics
  - Showing concept relationships
  - Student seems confused about structure

#### 11. Webcam for Homework Help
- **File**: `/web/src/components/voice/webcam-capture.tsx`
- Students can show homework or textbook pages
- Actual base64 image sent to AI (not just text description)
- AI analyzes and helps with visible content
- Privacy-focused: images processed in real-time, not stored

#### 12. Safe Web Search
- **File**: `/web/src/app/api/search/route.ts`
- Kid-filtered search results
- Safe content only
- Educational focus

### Accessibility

#### 12. Comprehensive Accessibility Runtime
- **File**: `/web/src/lib/accessibility/accessibility.ts`
- **Styles**: `/web/src/app/globals.css`
- Features:
  - OpenDyslexic font support
  - High contrast mode
  - Reduced motion mode
  - Large text mode
  - Color blind friendly palette
  - ADHD distraction-free mode
  - Keyboard navigation indicators
  - Screen reader support (sr-only classes)
  - Skip to content link (keyboard accessible)
  - Minimum touch target sizes (WCAG 2.1 AA)
  - Print styles for educational content

### Settings & Configuration

#### 13. Azure OpenAI Configuration
- **File**: `/web/src/lib/stores/app-store.ts`
- Azure endpoint configuration
- API key management
- Deployment name settings
- API version selection

#### 14. Theme System
- Light/Dark/System modes
- next-themes integration
- Persisted preference

#### 15. Appearance Settings
- Theme selection
- Accent color customization
- Language preference
- All settings persisted via Zustand

### Testing

#### 16. Playwright E2E Tests
- **Directory**: `/web/tests/`
- 5 test files with 40+ test cases
- Coverage for:
  - Navigation
  - Maestri selection
  - Settings
  - Voice sessions
  - Accessibility

### Developer Experience

#### 17. Auto-Sync from CLI
- `make sync-web` command
- Syncs maestri data from CLI to webapp
- Ensures consistency across platforms

#### 18. UX Improvement: Direct Voice Access
- Clicking a maestro goes directly to voice session
- Removed intermediate choice screen
- Faster user experience

---

## Pending Tasks

### High Priority

#### 1. Make Webapp Fully Autonomous from CLI
**Status**: Pending
**Description**: The webapp should function completely independently without requiring the CLI tool.
- Remove any remaining CLI dependencies
- Self-contained configuration
- Independent deployment capability

#### 2. Backend for Persistent Gamification
**Status**: Pending
**Description**: Implement persistent storage for gamification features.
- Database design (PostgreSQL recommended)
- API endpoints for:
  - User progress tracking
  - Achievement system
  - Leaderboards
  - XP and level progression
- Consider:
  - Supabase or PlanetScale for managed DB
  - Vercel KV for caching
  - Edge functions for low latency

---

## Architecture

```
web/
├── src/
│   ├── app/                    # Next.js App Router
│   │   ├── api/               # API routes
│   │   │   └── search/        # Safe search endpoint
│   │   ├── globals.css        # Global styles + accessibility
│   │   ├── layout.tsx         # Root layout
│   │   └── page.tsx           # Main page
│   ├── components/
│   │   ├── quiz/              # Quiz components
│   │   ├── settings/          # Settings UI
│   │   ├── tools/             # AI tool renderers
│   │   └── voice/             # Voice session UI
│   ├── data/
│   │   └── index.ts           # Maestri definitions
│   └── lib/
│       ├── accessibility/     # Accessibility runtime
│       ├── hooks/             # React hooks
│       ├── learning/          # FSRS, mastery algorithms
│       └── stores/            # Zustand stores
├── tests/                      # Playwright E2E tests
└── public/                     # Static assets
```

---

## Tech Stack

- **Framework**: Next.js 16.1.1
- **React**: 19.2.3
- **State Management**: Zustand 5.0.9
- **Styling**: Tailwind CSS 4
- **UI Components**: Radix UI
- **Animations**: Framer Motion
- **Charts**: Chart.js + react-chartjs-2
- **Diagrams**: Mermaid.js
- **Math**: KaTeX
- **Testing**: Playwright
- **AI**: OpenAI Realtime API

---

## Environment Variables

```env
# Required
OPENAI_API_KEY=your-openai-api-key

# Optional (Azure)
AZURE_OPENAI_ENDPOINT=https://your-resource.openai.azure.com
AZURE_OPENAI_API_KEY=your-azure-key
AZURE_OPENAI_DEPLOYMENT=your-deployment-name
AZURE_OPENAI_API_VERSION=2024-02-15-preview
```

---

## Running the App

```bash
# Install dependencies
npm install

# Development
npm run dev

# Build
npm run build

# Production
npm start

# Tests
npm test
npm run test:ui      # Interactive UI
npm run test:headed  # With browser
npm run test:debug   # Debug mode
```

---

## Contributors

Roberto with the help of a team of AI agents.

---

*Last updated: December 27, 2025*
