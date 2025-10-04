### AGENTS: Contributor Guide for Budgie Daemon

This document is a quick, action-oriented guide for agents and contributors working on Budgie Daemon.

It summarizes common workflows (especially around D-Bus schema changes and code generation), build tasks, and conventions. See `README.md` for full background, build prerequisites, and project scope.

### Project quick facts

- Core purpose: Budgie Daemon v2 is the future central hub and orchestrator for Budgie Desktop (with a focus on Budgie 11). Today, it primarily provides Wayland-native display configuration for Budgie 10.10; over time it will coordinate broader desktop logic for Budgie 11.
- Wayland protocol: `wlr-output-management-unstable-v1`.
- DBus API: XML schemas in `src/dbus/schemas/` → generated adaptors in `src/dbus/generated/`.
- Output model: meta heads/modes exposed via services under `org.buddiesofbudgie.BudgieDaemon`.

### Command cheatsheet (Taskfile)

- Task is used as a helpful task runner (single binary, no deps) Invoke tasks with the `task` command. See the Taskfile docs [here](https://taskfile.dev/).

- Configure + build:
  - `task setup`
  - `task build`
  - Or together: `task cook`
- Install (autostart assets, systemd user unit depending on CMake options):
  - `sudo task install`
- Regenerate D-Bus adaptors from XML:
  - `task qdbus-gen`
- Format code:
  - `task fmt`
- Wayland debug (run built binary under `wldbg`):
  - `task wldbg-build`

### Typical D-Bus workflow

1) Edit or add a schema in `src/dbus/schemas/*.xml`.
   - Define interface, properties, methods, signals.
   - For QVariantMap outputs, include the Qt DBus type annotation:
     - `<annotation name="org.qtproject.QtDBus.QtTypeName.Out0" value="QVariantMap"/>`

2) Regenerate adaptor code:
   - `task qdbus-gen`
   - Do not hand-edit files in `src/dbus/generated/`.

3) Implement the service methods/properties:
   - Add declarations to the corresponding header in `src/dbus/*.hpp` under `public slots` or as properties.
   - Implement logic in `src/dbus/*.cpp`.
   - Prefer using `WaylandOrchestrator::instance().getManager()->getHeads()` to enumerate outputs.
   - Primary selection: when asked to resolve a "primary" output, prefer heads where `isPrimary()` is true; fall back to the first available head.

4) Register new objects if needed:
   - See `DisplayObjectManager.cpp` and `main.cpp` for service and object registration patterns.

5) Build and test:
   - `task cook`
   - Optionally run from build tree: `./build/bin/org.buddiesofbudgie.BudgieDaemonV2`

### Example: Adding primary-output helpers (pattern)

- Schema changes (Display service examples):
  - Add `GetPrimaryOutput` returning a string serial/identifier.
  - Add `GetPrimaryOutputRect` returning a QVariantMap with keys: `X`, `Y`, `Width`, `Height`.
- Regenerate adaptors: `task qdbus-gen`.
- Implement methods in `dbus/DisplayService.hpp/.cpp`:
  - Use `WaylandOrchestrator` → `WaylandOutputManager` → heads.
  - Choose primary via `head->isPrimary()`; otherwise first head.
  - For geometry, use `head->getPosition()` and its current mode size (`getCurrentMode()->getSize()`), mirroring the QVariantMap style used by `OutputModeService::GetModeInfo()`.

### Code and API conventions

- C++ style: Prefer clear, descriptive names. Keep methods short and guard conditions early.
- D-Bus types:
  - Use QString, QStringList, bool, int, qulonglong, QVariantMap as appropriate.
  - Add `org.qtproject.QtDBus.QtTypeName.*` annotations when returning complex Qt types.
- Generated code:
  - Never modify `src/dbus/generated/*` by hand. Regenerate from XML.

### Git and commit message conventions

We strive to follow [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) for commit messages.

- Common types we use: `feat`, `fix`, `docs`, `chore`, `ci`.
- Breaking changes: append `!` after type/scope (e.g., `feat(api)!:`) or include a `BREAKING CHANGE:` footer.
- Examples:

  ```
  feat(displays): add GetPrimaryOutput and GetPrimaryOutputRect methods

  docs(readme): document Conventional Commits usage

  fix(displays): correct refresh rate conversion to qulonglong
  ```
### Configuration and profiles (summary)

- On Wayland ready, a display profile group is selected and applied atomically via the batch system.
- Users can store profiles at `$XDG_CONFIG_HOME/budgie-desktop/display-config.toml` (or `~/.config/...`).
- Group schema supports `primary_output`, anchors, relative positioning, scale, rotation, adaptive sync, and enable/disable flags.

### Troubleshooting

- Missing or stale D-Bus methods:
  - Ensure the XML schema is updated and `task qdbus-gen` has been run.
  - Rebuild (`task build`) and restart the daemon.
- Wayland issues:
  - Use `task wldbg-build` (sets `QT_LOGGING_RULES=*.debug=true` and runs under `wldbg` to trace Wayland events).
- Formatting/lints:
  - `task fmt`

### License

- MPL-2.0 (see `COPYING`).


