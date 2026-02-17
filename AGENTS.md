# AGENTS.md

## Repository overview
- C++17 + Qt6 desktop GUI for managing MIME defaults on Linux.
- Build system: CMake with a helper Makefile (out-of-source build in `build/`).
- Source layout under `src/` with `ui/`, `models/`, `services/`, `utils/`, `assets/`.
- Theming: Catppuccin palette loaded from `src/assets/palette.json` (Qt resource).

## IMPORTANT: Never run the build
**Do NOT run `make build`, `cmake --build`, or `make run`.**
The `make build` target installs via `sudo` to `/opt/mime-settings/` and registers
a `.desktop` file — it has side effects beyond compilation. Write code and stop;
the user builds and runs manually.

## Commands
- Configure (release): `make configure BUILD_DIR=build BUILD_TYPE=Release`
- Configure (debug):   `make configure-debug BUILD_DIR=build`
- Clean:               `make clean BUILD_DIR=build`
- Format:              `make format` (runs `clang-format -i` on all `src/**/*.h` and `src/**/*.cpp`)
- No test target exists yet; if added, use CTest: `ctest --test-dir build -R <regex>`

## Clang-format rules (from .clang-format)
- `BasedOnStyle: LLVM`, `IndentWidth: 2`, `UseTab: Never`, `ColumnLimit: 100`.
- `BreakBeforeBraces: Attach` — K&R/same-line braces.
- `PointerAlignment: Right` — `int *p`, `QWidget *parent`.
- No short if-statements, loops, functions, or blocks on a single line.
- `SortIncludes: true`, `IncludeBlocks: Regroup` — includes auto-sorted into groups.

## Quick repo map
- `src/main.cpp` — app entry, font setup, creates `MainWindow`.
- `src/ui/MainWindow.cpp/.h` — top-level window; owns all subsystems; Catppuccin theming.
- `src/ui/DetailsPane.cpp/.h` — right-hand MIME detail panel; emits `requestSetDefault`.
- `src/models/MimeTypeModel.cpp/.h` — two-level `QAbstractItemModel` (category → MIME type).
- `src/models/MimeTypeFilterProxy.cpp/.h` — live-search `QSortFilterProxyModel`.
- `src/services/AppRegistry.cpp/.h` — scans XDG app dirs, parses `.desktop` files.
- `src/services/MimeDefaultsStore.cpp/.h` — reads/writes `mimeapps.list`.
- `src/services/MimeAssociationService.cpp/.h` — combines registry + store into `MimeEntry` records.
- `src/utils/XdgPaths.cpp/.h` — XDG Base Directory resolution (config/data/app dirs).
- `src/assets/palette.json` — Catppuccin color data (Latte, Frappé, Macchiato, Mocha).

## Include order
Every `.cpp` follows this exact order (matches `.clang-format` `IncludeCategories`):
1. The matching header for this `.cpp` (e.g. `"ui/MainWindow.h"`).
2. Other local/project headers (`"services/..."`, `"models/..."`, `"utils/..."`).
3. Qt headers (`<QWidget>`, `<QString>`, …).
4. STL headers if needed (`<algorithm>`, `<cmath>`).

## Naming conventions
- Types/classes: `PascalCase` (`MainWindow`, `MimeDefaultsStore`).
- Methods/variables: `camelCase` (`setEntry`, `loadData`).
- Private members: `m_` prefix (`m_registry`, `m_table`, `m_currentThemeId`).
- Enum values: `PascalCase` (`MimeColumn`, `DefaultAppColumn`).
- File names: `PascalCase` for class files (`AppRegistry.cpp`).

## Pointer and allocation style
- Pointer on type side: `QWidget *parent`, `int *p`.
- Use `auto *` for all heap-allocated Qt objects:
  ```cpp
  auto *layout = new QVBoxLayout(this);
  auto *label  = new QLabel("text", this);
  ```
- Always pass a parent at construction; rely on Qt ownership — no manual `delete`.

## Qt patterns
- `#pragma once` in every header — never `#ifndef` guards.
- `explicit` constructor on every QObject-derived class; default `parent = nullptr`.
- `Q_OBJECT` macro in every class using signals/slots.
- `const QString &` and `const QVector<...> &` for read-only parameters.
- Use `QSignalBlocker` when programmatically updating widgets to suppress signal loops
  (e.g. `populateThemePicker`, `populateAccentPicker`).
- Use `Qt::CaseInsensitive` for filtering; `QString::localeAwareCompare` for sort order.
- File-local helpers go in `namespace { ... }` in the `.cpp` — not in the header.

## Error handling
- Early-exit on invalid state: `if (!item) return;`, `if (!index.isValid()) return QVariant();`.
- No exceptions; rely on Qt return values and guard checks throughout.

## Collections
- Use `QSet<QString>` to deduplicate, then `.values()` + sort — never deduplicate in a loop.
- Prefer Qt containers: `QVector`, `QList`, `QStringList`, `QHash`, `QSet`.

## File I/O
- Use `QFile` + `QTextStream` for XDG config files — **not** `QSettings`.
  Rationale: `QSettings(IniFormat)` misparses semicolon-separated `MimeType=` values in
  `.desktop` files; manual line-by-line parsing is required for correctness.
- `QSettings(IniFormat)` is acceptable for the app's own `settings.ini`.
- Preserve all existing entries when updating `mimeapps.list` in-place.

## Data and architecture
- `MimeAssociationService` is the only layer that combines registry + store into `MimeEntry`.
- User settings: `$XDG_CONFIG_HOME/mimeapps.list` and `mime-settings/settings.ini`.
- System settings: read from XDG config/data dirs; never written.

## Model internals
- `MimeTypeModel` encodes tree levels in `internalId`:
  `0` = category row; `N ≥ 1` = MIME entry under category index `N-1`.
  This avoids raw pointer storage and dangling-pointer risk on model reset.
- Use `Qt::FontRole` returning bold for category rows; do not use custom delegates for this.
- `MimeTypeFilterProxy` uses `beginFilterChange()`/`endFilterChange()` (Qt 6.9+) rather
  than `invalidateFilter()` for live search updates.

## UI conventions
- All stylesheet theming is applied via `setStyleSheet` in `MainWindow::applyTheme()`.
- Layout spacing and margins are set explicitly; do not rely on platform defaults.
- Use `QStatusBar::showMessage(..., 3000)` for transient non-modal feedback.
- No i18n system; plain English UI labels only.

## Safety
- Do not delete or truncate user config files during normal operation.
- Keep changes localized to the feature being worked on.
- When adding new source files, register them in `CMakeLists.txt` `add_executable`.
