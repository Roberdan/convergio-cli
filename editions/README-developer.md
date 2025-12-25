# Convergio Developer Edition

## Ship Quality Code Faster

Building great software requires more than just writing code. You need code review, architecture decisions, debugging expertise, security awareness, performance optimization, and DevOps skills. **Convergio Developer** brings together specialized agents that work as your senior engineering team.

---

## Why Convergio Developer?

### Senior Engineering Expertise on Demand
Not every team has access to a principal architect, security expert, and performance specialist. Convergio Developer gives you that expertise instantly, for any codebase.

### Context-Aware Code Review
Rex doesn't just check syntax - he understands your codebase patterns, previous decisions, and architectural constraints. Reviews are specific, actionable, and aligned with your team's practices.

### Proactive Security
Luca continuously scans for OWASP Top 10 vulnerabilities, insecure patterns, and security anti-patterns. Catch issues before they become incidents.

### Zero-Context Debugging
Dario applies systematic debugging methodologies (rubber duck, binary search, hypothesis-driven) to help you find root causes faster, even in unfamiliar codebases.

---

## The Dev Team

### Coordination

| Agent | Role | Superpower |
|-------|------|------------|
| **Ali** | Chief of Staff | Technical decisions, cross-team coordination |
| **Anna** | Assistant | Sprint reminders, deadline tracking |

### Code Quality

| Agent | Role | Superpower |
|-------|------|------------|
| **Rex** | Code Reviewer | Design patterns, SOLID principles, clean code |
| **Paolo** | Best Practices Enforcer | Coding standards, linting, quality gates |
| **Dario** | Debugger | Root cause analysis, systematic debugging |

### Architecture & Infrastructure

| Agent | Role | Superpower |
|-------|------|------------|
| **Baccio** | Tech Architect | System design, scalability, DDD, Clean Architecture |
| **Marco** | DevOps Engineer | CI/CD, IaC, Kubernetes, GitOps |
| **Otto** | Performance Optimizer | Profiling, bottleneck analysis, optimization |

### Security

| Agent | Role | Superpower |
|-------|------|------------|
| **Luca** | Security Expert | Pentesting, OWASP, Zero-Trust, risk management |

---

## Key Capabilities

### Code Review
```
@rex Review this PR: gh pr view 123
@rex What design patterns should I use here?
@paolo Is this code following our standards?
```

### Debugging
```
@dario This function is returning null unexpectedly
@dario Help me trace this race condition
@dario Why is this test flaky?
```

### Architecture
```
@baccio Design a microservices architecture for this feature
@baccio Should we use event sourcing here?
@baccio Review our API design
```

### Performance
```
@otto Profile this slow endpoint
@otto Identify bottlenecks in our database queries
@otto What caching strategy should we use?
```

### Security
```
@luca Scan this codebase for vulnerabilities
@luca Review our authentication implementation
@luca Is this input properly sanitized?
```

### DevOps
```
@marco Set up a CI/CD pipeline for this project
@marco Write a Kubernetes deployment for this service
@marco Help me configure ArgoCD for GitOps
```

---

## Supported Languages & Frameworks

| Category | Technologies |
|----------|-------------|
| **Languages** | Python, TypeScript, Go, Rust, Java, C/C++ |
| **Frontend** | React, Vue, Angular, Svelte |
| **Backend** | FastAPI, Express, Django, Spring |
| **Infrastructure** | Terraform, Pulumi, CloudFormation |
| **Containers** | Docker, Kubernetes, Helm |
| **CI/CD** | GitHub Actions, GitLab CI, CircleCI |
| **Databases** | PostgreSQL, MySQL, Redis, MongoDB |

---

## Development Workflows

### Before Commit
1. Write code
2. `@rex` reviews for patterns and quality
3. `@luca` checks for security issues
4. `@paolo` validates against team standards

### Performance Issues
1. Identify slow behavior
2. `@otto` profiles and analyzes
3. `@baccio` suggests architectural improvements
4. `@dario` helps debug specific bottlenecks

### New Feature Design
1. Describe requirements
2. `@baccio` designs the architecture
3. `@marco` plans the infrastructure
4. `@luca` reviews security implications

---

## Integration Points

Works with your existing tools:
- **Git**: GitHub, GitLab, Bitbucket
- **IDE**: Zed (via ACP extension), VS Code
- **CI/CD**: All major platforms
- **Cloud**: AWS, GCP, Azure

---

## Technical Specs

- **Version Suffix**: `-dev`
- **Total Agents**: 11
- **Focus**: Code quality, architecture, DevOps
- **Codebase Awareness**: Indexes and understands your project

---

## Getting Started

```bash
# Build Developer Edition
make EDITION=developer

# Run
./build/bin/convergio

# Start working
@baccio I need to add real-time updates to our app. What's the best approach?
```

---

## Who Is This For?

- **Solo developers** who need senior feedback
- **Small teams** without specialized expertise
- **Open source maintainers** managing contributions
- **Tech leads** who need to scale their review capacity
- **DevOps engineers** automating infrastructure

---

*"Any fool can write code that a computer can understand. Good programmers write code that humans can understand." - Martin Fowler*

*Copyright (c) 2025 Convergio.io - All rights reserved*
