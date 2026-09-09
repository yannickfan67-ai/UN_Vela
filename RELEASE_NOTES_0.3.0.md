# UN_Vela 0.3.0

UN_Vela 0.3.0 moves the browser shell and Aster runtime from the earlier markup-only prototype into a portable lightweight web runtime.

## Highlights

- Aster 0.3 HTML renderer with lightweight CSS support.
- CSS tag selectors, `*`, inline styles, color, bold weight, size scaling, underline and `display:none`.
- Full/Lite Vela feature profiles so recovery shells can share the engine without enabling JavaScript.
- Controlled JavaScript subset for document title/body/write, logging and location navigation.
- Relative, root-relative and scheme-relative URL resolution plus link activation.
- History, Back/Forward/Reload APIs and viewport-aware scrolling.
- Binary resource ABI and lightweight BMP 24/32-bit + PPM P6 image decoding.
- Windows hosted HTTPS through WinHTTP with operating-system certificate validation.
- Linux/macOS hosted HTTPS through in-process libcurl with peer and hostname verification.
- Hosted runtime tests on Windows, Ubuntu and macOS.
- Portable core compilation on x86_64, AArch64 and RISC-V64.
- UN_Orion integration adds browser navigation buttons, clickable links and IntelliMouse wheel scrolling.
- Orion Browser 0.2 recovery shell reuses the same Vela/Aster core through `VELA_PROFILE_LITE`.

## Security and compatibility

UN_Vela only accepts HTTPS when the selected carrier advertises TLS capability. Hosted Windows/Linux/macOS carriers provide verified TLS. The freestanding UN_Orion kernel does not yet ship a trusted TLS provider and therefore does not advertise TLS; HTTPS is rejected rather than silently downgraded to HTTP.

The CSS and JavaScript implementations are deliberately compact subsets and are not intended to claim HTML/CSS/ECMAScript web-platform conformance. Native image decoding currently focuses on BMP and PPM for low-dependency targets.

## Core versions

- UN_Vela 0.3.0
- Aster Engine 0.3.0
- Vela API 1.2
- Vela Platform ABI 1.1
