# Repository Guidelines

## Project Structure & Module Organization
Core code lives in `src/`. The main entry point is `src/bbcp.C`; most supporting modules follow the `bbcp_<Component>.C` and `bbcp_<Component>.h` pattern. Shared C support for NetLogger lives beside it in `src/NetLogger.c` and `src/NetLogger.h`. Build outputs are written to platform-specific directories under `bin/<OSVER>/` and `obj/<OSVER>/`. Packaging metadata currently lives in `utils/bbcp.spec`.

## Build, Test, and Development Commands
Build from `src/`, not the repository root:

```sh
cd src
make
make clean
make Darwin OPT='-g'
```

`make` auto-detects the platform with `../MakeSname` and writes `bbcp` to `bin/<OSVER>/`. `make clean` removes object files and the generated binary for the current platform. Use an explicit target such as `make Darwin` when you need to override platform detection or pass `OPT` flags.

For a quick smoke test after building:

```sh
./bin/_darwin_/bbcp -h
```

## Coding Style & Naming Conventions
Match the existing C/C++ style in `src/`: 3-space indentation inside functions, K&R-style braces, aligned declarations, and large banner comments only where the file already uses them. Preserve the established naming scheme: `bbcp_`-prefixed types and files, `CamelCase` method names, and uppercase macros like `Same`. There is no formatter configuration in the repo, so keep changes narrow and visually consistent with adjacent code.

## Testing Guidelines
There is no dedicated automated test suite in this repository today. Every change should, at minimum, compile cleanly and pass a manual smoke test using `bbcp -h` or a targeted transfer scenario that exercises the touched path. When fixing protocol, filesystem, or security behavior, note the exact command used for verification in the PR description.

## Commit & Pull Request Guidelines
Recent commits use short, imperative summaries such as `hardening` and `only allow expected peers`. Follow that pattern: one concise subject line focused on the behavior changed. Pull requests should explain the user-visible impact, list build and manual test steps, and call out any security-sensitive changes or compatibility risks. Include logs or terminal snippets when they clarify transfer, auth, or networking behavior.
