# UN_Vela roadmap

## Transport
- receive raw HTTP entity bodies from UN_Orion networking
- redirects and HTTP header handling
- URL resolution and relative links
- HTTPS once Orion TLS is available

## Browser shell
- clickable Aster links
- back/forward history
- reload/stop
- downloads into Orion Files
- bookmarks and session restore
- low-memory failure pages that can fall back to Orion-Browser

## Engine integration
- Aster owns all markup parsing; Vela owns navigation/UI only
- scrolling and link hit-testing through Aster paint metadata
- forms/input events
- images and style resources
- future script runtime only behind a separate capability boundary

## Platform
- ORX-facing browser intents / open-url association
- downloads and permissions through Orion services
- networking should become asynchronous after the kernel socket API exists
