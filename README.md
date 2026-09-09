# UN_Vela

UN_Vela is the primary browser shell for UN_Orion and a portable hosted browser core for conventional desktop operating systems.

Current development line: **0.3.1-dev**.  
Rendering engine ABI: **Aster Engine 0.3.1 / API 1.3**.

UN_Vela owns browser-shell behavior: URL input, navigation state, page title/status, history, scrolling, viewport state, feature profiles and the host-service boundary. HTML parsing/layout/painting belongs to Aster Engine.

## Architecture

```text
UN_Vela core -> Aster Engine -> host graphics backend
             -> Vela Platform ABI -> host services
```

UN_Orion supplies its own native carrier. Hosted builds can instead use the desktop carrier layer without changing browser-core code.

## Browser/runtime features

The 0.3.x line includes:

- markup-preserving HTML navigation
- Back / Forward / Reload and scroll APIs
- relative/root/scheme-relative URL resolution
- Full and Lite feature profiles
- a controlled JavaScript subset in Full mode
- binary resource loading for images
- Aster lightweight CSS and image layout
- simple CSS selectors in Aster 0.3.1: `*`, tag, `.class`, `#id`, `tag.class`, `tag#id`, and comma-separated simple selector lists

Aster remains intentionally lightweight. Descendant, child, attribute and pseudo selectors are not yet implemented.

## Hosted operating-system compatibility

The repository contains first-class hosted carrier implementations for:

- **Linux** — libcurl transport/TLS
- **macOS** — libcurl transport/TLS
- **Windows** — WinHTTP transport/TLS

Hosted carriers provide HTTP/HTTPS, resources, host logging and time services. TLS is advertised only where the carrier performs certificate validation:

- Windows uses WinHTTP and the operating-system certificate store.
- Linux/macOS use libcurl with peer and hostname verification enabled.

Redirects and normal HTTP response handling are delegated to the native/system hosted transport. The freestanding UN_Orion carrier remains separate and deliberately does not advertise TLS until Orion has a trusted kernel TLS provider.

## CPU architecture portability

The freestanding browser core compiles for:

- x86_64
- AArch64
- RISC-V64

This is separate from hosted OS compatibility: the architecture matrix checks freestanding core portability, while the hosted matrix checks real desktop operating-system toolchains and carrier initialization.

## CI coverage

Two compatibility layers are tested:

1. `portable-core` compiles the core for x86_64, AArch64 and RISC-V64.
2. `hosted-os` configures, builds and runs a hosted carrier smoke test on Ubuntu, macOS and Windows.

## Build hosted carrier

```bash
cmake -S . -B build-hosted -DCMAKE_BUILD_TYPE=Release
cmake --build build-hosted --config Release
ctest --test-dir build-hosted -C Release --output-on-failure
```

## Layout

- `include/vela.h` — browser-shell ABI
- `include/vela_platform.h` — stable host-service ABI
- `include/vela_host.h` — hosted desktop carrier wrapper
- `src/vela.c` — portable browser-shell core
- `host/vela_host_posix.c` — Linux/macOS libcurl carrier
- `host/vela_host_win32.c` — Windows WinHTTP carrier
- `tests/host_smoke.c` — hosted ABI/runtime smoke test
- `vendor/aster/include/aster.h` — synchronized Aster public ABI

`Orion-Browser` is maintained as a lightweight recovery shell over the Vela Lite profile rather than as a second divergent rendering engine.
