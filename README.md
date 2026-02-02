# Odyssey

A mobile-first RPG/Adventure game built with Unreal Engine 5.

## Project Status

**Phase 0: Foundation & Learning** - In Progress

## Quick Links

- [Game Design Document](Docs/GDD.md)
- [Technical Design Document](Docs/TDD.md)
- [Setup Guide](Docs/SETUP.md)

## Project Structure

```
Odyssey/
├── Odyssey/           # Unreal Engine project (create via UE5 Editor)
├── Docs/              # Design documents
│   ├── GDD.md         # Game Design Document
│   ├── TDD.md         # Technical Design Document
│   ├── SETUP.md       # Setup instructions
│   └── Art/           # Concept art, references
├── Server/            # Backend services (Phase 3+)
└── Tools/             # Build scripts, utilities
```

## Getting Started

See [SETUP.md](Docs/SETUP.md) for detailed instructions.

### Prerequisites

- Unreal Engine 5.4+
- Xcode (for iOS)
- Android Studio (for Android)

### Quick Start

1. Install Unreal Engine 5.4 via Epic Games Launcher
2. Install mobile SDKs (Android Studio, Xcode)
3. Open UE5 and create project at `~/dev/Odyssey/Odyssey`
4. Configure mobile target platforms
5. Follow the learning path in SETUP.md

## Development Phases

| Phase | Description | Status |
|-------|-------------|--------|
| 0 | Foundation & Learning | In Progress |
| 1 | Core Single-Player Prototype | Not Started |
| 2 | Content & Polish | Not Started |
| 3 | Multiplayer Foundation | Not Started |
| 4 | Expanded Multiplayer | Not Started |

## Tech Stack

- **Engine:** Unreal Engine 5
- **Primary Language:** Blueprints (C++ for optimization later)
- **Platforms:** iOS, Android (PC/Console later)
- **Backend:** Node.js/Go (Phase 3+)
- **Database:** PostgreSQL (Phase 3+)
