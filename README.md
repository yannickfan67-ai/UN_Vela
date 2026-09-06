# UN_Vela

UN_Vela is the primary native browser for UN_Orion.

Current version: **0.1.0**.
Rendering engine: **Aster Engine 0.1.0**.

UN_Vela owns browser-shell behavior: URL input, navigation state, page title/status, viewport and integration with the UN_Orion network stack. HTML parsing/layout/painting belongs to Aster Engine.

## Architecture

`UN_Vela -> Aster Engine -> Orion graphics`

`UN_Vela -> Orion HTTP/TCP/IP stack`

## Layout

- `include/vela.h` — browser-shell ABI
- `src/vela.c` — current UN_Orion integration

The lightweight historical Orion Browser is intentionally maintained separately in `Orion-Browser` as a fallback/recovery browser rather than being deleted.
