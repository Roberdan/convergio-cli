# Convergio-Edu

Educational web platform with AI-powered voice tutors (Maestri) for personalized learning experiences.

## Features

- **14 AI Maestri** - Historical master teachers with unique personalities and teaching styles
- **Voice Sessions** - Real-time voice conversations using OpenAI Realtime API via Azure
- **Dynamic Tools** - Maestri can create mindmaps, flashcards, quizzes during lessons
- **Safe Web Search** - Kid-friendly filtered search for educational research
- **Accessibility** - Full support for dyslexia, dyscalculia, ADHD, autism, cerebral palsy
- **Gamification** - XP, levels, badges, streaks to motivate learning
- **Dark/Light Theme** - Full theme support with system preference detection
- **FSRS Algorithm** - Scientific spaced repetition for flashcard learning
- **Mastery Learning** - 80% threshold gates before advancing

## Architecture

```
src/
├── app/                    # Next.js App Router
│   ├── api/               # API routes
│   │   ├── realtime/      # Azure OpenAI token endpoint
│   │   ├── search/        # Safe search API
│   │   └── progress/      # Progress sync
│   └── page.tsx           # Main application
├── components/
│   ├── maestros/          # Maestri grid and cards
│   ├── voice/             # Voice session UI
│   ├── education/         # Flashcards, homework help
│   ├── progress/          # Progress tracking UI
│   ├── settings/          # Configuration
│   └── ui/                # Shared components
├── lib/
│   ├── hooks/             # React hooks (voice session)
│   ├── stores/            # Zustand state management
│   ├── utils/             # Utilities
│   ├── fsrs.ts            # Spaced repetition algorithm
│   ├── mastery.ts         # Mastery learning system
│   └── accessibility.ts   # Accessibility runtime
└── data/
    └── maestri-full.ts    # Maestri definitions and prompts
```

## Quick Start

### Prerequisites

- Node.js 18+
- npm or pnpm
- Azure OpenAI account with Realtime API access

### Installation

```bash
cd web
npm install
```

### Configuration

Create `.env.local`:

```env
# Azure OpenAI Realtime (required)
AZURE_OPENAI_REALTIME_ENDPOINT=https://your-resource.openai.azure.com
AZURE_OPENAI_REALTIME_API_KEY=your-api-key
AZURE_OPENAI_REALTIME_DEPLOYMENT=gpt-4o-realtime-preview

# Safe Search (optional - falls back to curated results)
BING_SEARCH_API_KEY=your-bing-key
```

### Development

```bash
npm run dev
```

Open http://localhost:3000

### Production Build

```bash
npm run build
npm start
```

## Deployment

### Vercel (Recommended)

1. Push to GitHub
2. Import in Vercel
3. Add environment variables
4. Deploy

### Docker

```dockerfile
FROM node:18-alpine
WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production
COPY . .
RUN npm run build
EXPOSE 3000
CMD ["npm", "start"]
```

### Self-Hosted

```bash
npm run build
npm start -p 3000
```

## Maestri (AI Tutors)

| Name | Subject | Inspired By |
|------|---------|-------------|
| Pitagora | Mathematics | Pythagoras |
| Leonardo | Art & Science | Leonardo da Vinci |
| Dante | Italian Literature | Dante Alighieri |
| Galileo | Physics | Galileo Galilei |
| Maria | Science | Maria Montessori |
| Mozart | Music | Wolfgang Amadeus Mozart |
| Shakespeare | English | William Shakespeare |
| Marco | Geography | Marco Polo |
| Archimede | Engineering | Archimedes |
| Cleopatra | History | Cleopatra VII |
| Darwin | Biology | Charles Darwin |
| Curie | Chemistry | Marie Curie |
| Socrate | Philosophy | Socrates |
| Fibonacci | Computer Science | Leonardo Fibonacci |

## Tool System

Maestri have access to these tools during lessons:

| Tool | Description |
|------|-------------|
| `create_mindmap` | Generate Mermaid.js concept maps |
| `create_flashcard` | Create spaced repetition cards |
| `create_quiz` | Generate interactive quizzes |
| `web_search` | Safe search for educational content |
| `capture_homework` | Request camera for homework help |

## Accessibility Features

- **Dyslexia**: OpenDyslexic font, increased spacing, simplified layouts
- **Dyscalculia**: Number visualization, step-by-step math breakdowns
- **ADHD**: Reduced animations, focus modes, break reminders
- **Autism**: Predictable layouts, sensory-friendly colors, clear instructions
- **Cerebral Palsy**: Large touch targets, keyboard navigation, voice control

## Testing

```bash
# Run Playwright E2E tests
npx playwright test

# Run with UI
npx playwright test --ui
```

## Tech Stack

- **Framework**: Next.js 16 (App Router)
- **Language**: TypeScript 5
- **Styling**: Tailwind CSS 4
- **State**: Zustand
- **Voice**: OpenAI Realtime API (Azure)
- **Charts**: Chart.js
- **Diagrams**: Mermaid.js
- **Math**: KaTeX
- **Animation**: Framer Motion
- **Testing**: Playwright

## License

Proprietary - Convergio

## Related Projects

- [ConvergioCLI](../../../ConvergioCLI) - CLI version with full agent ecosystem
- [ConvergioApp](../../../ConvergioCLI/ConvergioApp) - Native macOS/iOS app
