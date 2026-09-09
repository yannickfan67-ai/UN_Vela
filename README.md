# UN_Vela

UN_Vela is the primary browser shell for UN_Orion and a portable hosted browser core for conventional desktop operating systems.

Current development line: **0.2.0-dev**.  
Rendering engine ABI: **Aster Engine 0.2.0-dev**.

UN_Vela owns browser-shell behavior: URL input, navigation state, page title/status, history, scrolling, viewport state and the host-service boundary. HTML parsing/layout/painting belongs to Aster Engine.

## Architecture

```text
UN_Vela core -> Aster Engine -> host graphics backend
             -> Vela Platform ABI -> host services
```

UN_Orion supplies its own native carrier. Hosted builds can instead use the desktop carrier layer without changing browser-core code.

## Hosted operating-system compatibility

The repository now contains first-class hosted carrier implementations for:

- **Linux** — POSIX/BSD sockets + `getaddrinfo`
- **macOS** — POSIX/BSD sockets + `getaddrinfo`
- **Windows** — WinSock 2 + `getaddrinfo`

All three hosted carriers expose Platform ABI 1 and currently provide:

- HTTP transport
- monotonic/wall-clock style millisecond timing for browser-host coordination
- host logging
- IPv4/IPv6-capable hostname resolution through the OS resolver
- socket send/receive timeouts
- HTTP markup-preserving body delivery
- CRLF and LF-only header termination support
- chunked-body fallback decoding
- identity content-encoding requests

The hosted carriers intentionally do **not** advertise TLS yet. HTTPS is therefore rejected by the browser core unless a future host implementation explicitly provides `VELA_PLATFORM_CAP_TLS`.

## CPU architecture portability

The freestanding browser core continues to compile for:

- x86_64
- AArch64
- RISC-V64

This is separate from hosted OS compatibility: the architecture matrix checks freestanding core portability, while the hosted matrix checks real desktop operating-system toolchains and carrier initialization.

## CI coverage

Two compatibility layers are tested:

1. `portable-core` compiles the core for x86_64, AArch64 and RISC-V64.
2. `hosted-os` configures, builds and runs a hosted carrier smoke test on Ubuntu, macOS and Windows.

The hosted test verifies Platform ABI compatibility and required HTTP/time capabilities after native carrier initialization.

## Build hosted carrier

```bash
cmake -S . -B build-hosted -DCMAKE_BUILD_TYPE=Release
cmake --build build-hosted --config Release
ctest --test-dir build-hosted -C Release --output-on-failure
```

CMake automatically chooses the WinSock carrier on Windows and the POSIX carrier on Linux/macOS.

## Layout

- `include/vela.h` — browser-shell ABI
- `include/vela_platform.h` — stable host-service ABI
- `include/vela_host.h` — hosted desktop carrier wrapper
- `src/vela.c` — portable browser-shell core
- `host/vela_host_posix.c` — Linux/macOS carrier
- `host/vela_host_win32.c` — Windows carrier
- `host/vela_http_common.h` — shared HTTP response compatibility helpers
- `tests/host_smoke.c` — hosted ABI/runtime smoke test
- `vendor/aster/include/aster.h` — synchronized Aster public ABI

The lightweight historical Orion Browser is intentionally maintained separately in `Orion-Browser` as a fallback/recovery browser rather than being deleted.
