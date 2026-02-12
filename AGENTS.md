# AGENTS.md

Purpose
- Provide build, lint, and test commands for agents working here.
- Capture the code style and conventions used across this repo.
- Call out any Cursor/Copilot rules that must be respected.

Repository overview
- C++17 + Qt6 desktop GUI for managing MIME defaults.
- Build system: CMake (out-of-source build in `build/`).
- Source layout under `src/` with `ui/`, `models/`, `services/`, `utils/`.

Cursor/Copilot rules
- Cursor rules: none found in `.cursor/rules/` or `.cursorrules`.
- Copilot rules: none found in `.github/copilot-instructions.md`.

Build requirements
- CMake >= 3.16.
- Qt6 Widgets, Gui, Core (development headers and runtime).
- A C++17 compiler (GCC/Clang/MSVC) on Linux.

Build commands (out-of-source)
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- Debug configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- Build: `cmake --build build`
- Clean: `cmake --build build --target clean`
- Reconfigure (fresh): delete `build/` then run configure again.

Build artifacts and compile database
- `build/`, `.cache/`, and `compile_commands.json` are gitignored.
- CMake generates `build/compile_commands.json` when configured.
- If tooling needs a root-level database, copy or symlink it from `build/`.
- Prefer out-of-source builds; do not add build outputs to the repo.

Run commands
- Run from build dir: `./build/mime-settings`
- If needed: `QT_QPA_PLATFORM=xcb ./build/mime-settings`

Test commands
- There is no test target defined in `CMakeLists.txt` yet.
- If tests are added, prefer CTest:
  - All tests: `ctest --test-dir build`
  - Single test by regex: `ctest --test-dir build -R <regex>`

Lint/format commands
- No lint or format targets are defined in this repo today.
- `.clangd` removes `-mno-direct-extern-access` for tooling only.
- If adding formatting, prefer `clang-format` but do not reformat
  unrelated files without request.

Quick repo map
- `src/main.cpp`: app entry, font setup, main window.
- `src/ui/`: widgets and view wiring.
- `src/models/`: Qt models / proxies.
- `src/services/`: app registry + MIME defaults store.
- `src/utils/`: XDG helpers.

Code style (observed)
- Language: C++17 with Qt types (`QString`, `QStringList`, `QVector`).
- Indentation: 2 spaces; no tabs.
- Braces: K&R style on same line.
- Line breaks: wrap long argument lists; align continuation with
  4 spaces or one indent level.
- File headers: `#pragma once` in headers.
- Include style:
  - Local/project headers first (`"ui/..."`, `"services/..."`).
  - Then Qt headers (`<QWidget>`, `<QString>`, ...).
  - Then STL headers if needed (`<algorithm>`, `<QSet>` already used).
- Keep includes minimal and sorted within each group.

Naming conventions
- Types: `PascalCase` (`MainWindow`, `MimeDefaultsStore`).
- Methods/variables: `camelCase` (`setEntry`, `loadData`).
- Members: `m_` prefix (`m_registry`, `m_table`).
- Enums: scoped or unscoped with `PascalCase` values in `enum` blocks.
- File names: `PascalCase` for UI/models/services classes.

Qt patterns
- Constructors use `explicit` and default `parent = nullptr`.
- QObject-derived classes use `Q_OBJECT` and Qt signals/slots.
- Prefer Qt containers and utility types over STL where possible.
- Parent ownership: allocate widgets with a parent, avoid manual delete.
- Use `const QString &` and `const QVector<...> &` for parameters.
- Use `const` locals where appropriate.

Formatting details
- Pointer style: `QWidget *parent`, `auto *layout = new QVBoxLayout(this);`.
- References: `const QString &mime`.
- Initializer lists on separate line when multi-arg.
- Prefer early `return` for guard clauses.

Headers and forward declarations
- Use forward declarations in headers to reduce include churn.
- Keep header includes minimal and rely on forward decls where possible.
- In `.cpp`, include the matching header first.
- Avoid circular includes; move shared types to `services/` or `models/`.

Error handling and robustness
- Favor early exits on invalid state (`if (!item) return;`).
- Check file open success before reading or writing.
- For optional lookups, handle missing data gracefully (`nullptr` checks).
- Use empty structs (e.g., `MimeEntry{}`) for reset/empty state.
- Avoid exceptions; rely on Qt return values and guard checks.

Data handling patterns
- `MimeDefaultsStore` reads `mimeapps.list` and merges user/system data.
- User settings stored in `XDG_CONFIG_HOME`.
- System settings read from XDG config/data dirs.
- Ensure `mimeapps.list` updates preserve existing entries.

Collections and sorting
- Prefer Qt containers (`QVector`, `QList`, `QStringList`, `QHash`, `QSet`).
- Use `QString::localeAwareCompare` for user-facing sort order.
- Avoid unnecessary copies; pass by `const &` where possible.
- Use `QSet` for deduping before emitting `QStringList`.

UI conventions
- Strings are simple English UI labels (no i18n system in repo yet).
- Style is set via `setStyleSheet` in `MainWindow`.
- Layout spacing and margins are set explicitly.
- Default states use placeholder text and disabled actions.
- Keep UI state in widgets and models; avoid global statics.
- Use `QStatusBar` for transient feedback instead of modal dialogs.

File I/O conventions
- Use `QFile` + `QTextStream` for text reads/writes.
- Check file existence and open success before reading.
- Preserve existing user data when updating config files.
- Prefer `QDir` and `QFileInfo` for path operations.

When adding new code
- Match 2-space indentation and existing include order.
- Prefer Qt classes for files, strings, lists, hashing.
- Keep UI logic in `ui/`, data in `services/`, models in `models/`.
- Keep changes focused; avoid broad refactors without request.

Safety and hygiene
- Do not delete user config files during normal operations.
- Avoid blocking UI operations on long scans; use guards if needed.
- Keep changes localized to the feature being worked on.
- Respect existing behavior unless a change is requested.

Adding tests (if introduced)
- Wire tests with CTest in `CMakeLists.txt`.
- Prefer small, deterministic unit tests.
- Use `ctest --test-dir build -R <regex>` for single tests.

Documentation updates
- Update this file when build/test steps or style guidelines change.
