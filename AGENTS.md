# Repository Guidelines

## Project Structure & Module Organization
- Core engine code lives in `src/` (display, input, audio, filesystem, net, LLM) with Meson build files alongside.
- Ruby MRI bindings are under `binding/`; preload wrappers that run before user scripts sit in `scripts/preload/`.
- GLSL shaders are in `shader/`; shared assets (fonts, controller DB, icons) live in `assets/`.
- Platform glue and dependency build scripts are split into `windows/`, `linux/`, and `macos/` (MSYS2 Makefiles, Meson toolchains, Xcode project).
- Sample RGSS regression cases are under `tests/` (Ruby scripts referenced via `mkxp.json` `customScript`), and default runtime configuration is in `mkxp.json`.

## Build, Test, and Development Commands
- Windows (MSYS2): `cd windows && make` to build deps, then `source windows/vars.sh` and `meson setup build && ninja -C build && strip build/mkxp-z.exe`.
- Linux native: `cd linux && make` for deps, then `source linux/vars.sh` and `meson setup build --bindir=. --prefix=$PWD/build/local && ninja -C build && ninja -C build install`.
- Cross Linux targets: `source linux/target-<arch>.sh` first, then `meson setup --cross-file linux/$ARCH_MESON_TOOLCHAIN build ...` followed by `ninja -C build`.
- macOS: use Xcode (`cd macos && xcodebuild -project mkxp-z.xcodeproj -configuration Release -scheme Universal`).
- After changing options in `meson_options.txt`, reconfigure with `meson setup build --reconfigure`; use `ninja -C build` for incremental compiles.

## Coding Style & Naming Conventions
- C++14 codebase; follow existing 4-space indentation, CamelCase methods (`setBitmap`, `update`) and PascalCase classes (`Sprite`).
- Keep headers minimal and ordered; prefer RAII over raw ownership and avoid introducing new global state.
- No enforced formatter—mirror surrounding brace and spacing style and keep comments concise (ASCII).

## Testing Guidelines
- Automated cases are Ruby RGSS scripts in `tests/`; run one by pointing `mkxp.json` `customScript` to `tests/<suite>/<name>-test.rb` and inspect the `test-results/` output it writes.
- For HTTP/LLM or networked features, exercise against local stubs where possible to avoid external dependencies.
- When reporting failures, note platform, `gfx_backend`, and any toggled options (`enable-https`, `use_miniffi`, `workdir_current`).

## Commit & Pull Request Guidelines
- Commit history favors short, imperative subjects (`Add Ollama PoC`, `Fix typo in otherDirs`); follow the same tone and scope.
- Target active branches (`dev`/`autobuild`); describe which platform(s) you built, which options were set, and which test scripts ran.
- Include screenshots or logs for rendering/input changes, and call out license implications when enabling HTTPS/OpenSSL or bundling fonts/Steamworks.
