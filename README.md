### Budgie Daemon v2

Budgie Daemon v2 is the future central hub and orchestrator for Budgie Desktop (with a focus on Budgie 11). Today, it primarily provides Wayland-native display configuration for Budgie 10.10; over time it will coordinate broader desktop logic for Budgie 11.

### Highlights

- **Wayland-native**: Uses `wlr-output-management-unstable-v1` to discover and configure outputs
- **Display Configuration Batch System**: Build atomic display configuration changes and apply them safely in one go
- **Profiles via TOML**: Persist and auto-apply display groups matched to current hardware
- **Future scope**: Will evolve into Budgie 11's core orchestrator beyond displays

### TODO

- [ ] Improve signal handling between meta objects and DBus signals
- [ ] Literally everything else post Budgie 10.10 release, such as...
  - [ ] Implement group swapping
  - [ ] General code refactoring to pave way for more modules
  - [ ] Implement plugin architecture for display system so wlr support can be swapped for alternatives and open door to supporting more compositors than just those based on wlroots or supporting those protocols
  - [ ] Notification (non-graphical) support for 11
  - [ ] Bluetooth
  - [ ] Power management
  - [ ] Surface / window tracking (possibly)

### Displays

#### Configuration file

On startup (after Wayland is ready), the daemon matches current outputs to a preferred `group` and applies the batch described by the entries in that group. If no configuration is present, one will be generated.

Location:

- `$XDG_CONFIG_HOME/budgie-desktop/display-config-v2.toml`, or
- `~/.config/budgie-desktop/display-config-v2.toml`

Schema (subset):

```toml
[preferences]
automatic_attach_outputs_relative_position = "right" # one of: left/right/above/below/none

[[group]]
name = "Laptop + Monitor"
preferred = true
identifiers = ["<laptop_id>", "<monitor_id>"]
primary_output = "<monitor_id>"

  [[group.output]]
  identifier = "<laptop_id>"
  width = 1920
  height = 1080
  refresh = 60.0
  relative_output = "<monitor_id>"        # optional
  horizontal_anchor = "left"              # none/left/right/center
  vertical_anchor   = "middle"            # none/above/top/middle/bottom/below
  scale = 1.0
  rotation = 0
  adaptive_sync = false
  disabled = false

  [[group.output]]
  identifier = "<monitor_id>"
  width = 2560
  height = 1440
  refresh = 144.0
  scale = 1.0
  rotation = 0
  adaptive_sync = true
  disabled = false
```

### Dependencies

- Qt 6 (Core, DBus, WaylandClient) >= 6.7
- KDE Frameworks 6: KWayland >= 6.6
- Wayland, QtWaylandScanner
- Extra CMake Modules (ECM)
- CMake >= 3.20, Ninja
- toml11 >= 4.4.0
  - We use CMake's FetchContent_Declare to pull this in, however will prefer toml11 from pkgconfig (FIND_PACKAGE_ARGS for toml11). If toml11 (header-only library) is available in your distribution's repository and meets our version requirement, we'd prefer you use that!

Optional for development:

- `task` (Taskfile runner), `watchman` (for `build-watch`), `wldbg`

### Build

With plain CMake/Ninja:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Install (autostart + optional systemd user unit depending on CMake options):

```bash
sudo ninja install -C build
```

Using Taskfile:

```bash
task cook     # configure + build
sudo task install
```

### Run

- After install, the binary is `org.buddiesofbudgie.BudgieDaemonV2` and is autostarted for the user session
- For ad-hoc runs from the build tree:

```bash
./build/bin/org.buddiesofbudgie.BudgieDaemonV2
```

Wayland debugging (example from Taskfile):

```bash
wldbg -r ./build/src/org.buddiesofbudgie.BudgieDaemonV2 -g
```

### Development notes

- Generated D-Bus adaptors live in `src/dbus/generated/` and are produced from XML in `src/dbus/schemas/`
  - Regenerate with:
    ```bash
    task qdbus-gen
    ```
  - Do not hand-edit generated files
- Code style: run clang-format
  ```bash
  task fmt
  ```

### Conventional Commits

We strive to follow [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) for commit messages.

- Common types we use: `feat`, `fix`, `docs`, `chore`, `ci`.
- Breaking changes: append `!` after type/scope (e.g., `feat(api)!:`) or include a `BREAKING CHANGE:` footer.
- Examples:

  ```
  feat(displays): add GetPrimaryOutput and GetPrimaryOutputRect methods

  docs(readme): document Conventional Commits usage

  fix(displays): correct refresh rate conversion to qulonglong
  ```

### Status and compatibility

- Project version: `0.0.1` (early). Some features are intentionally stubbed/not yet implemented (e.g., primary output selection, full mirroring semantics).
- Compatible with Budgie 10.10 and 11 on Wayland compositors that implement `wlr-output-management-unstable-v1`.

### License

MPL-2.0. See `COPYING`.
