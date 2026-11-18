# Local Windows deployment

This checkout runs the WebFPV source at commit `0a97f7cb0a7ae4ccbeb8d0199fdaf6b6c481b0e1`, with three.js 0.160.0 served locally. Node.js 22 or newer is required. No npm install or native/WASM rebuild is necessary to fly the committed `dist/sim.wasm`.

From this folder in PowerShell:

```powershell
.\start-local.ps1
# Open http://127.0.0.1:4186/?map=field
.\stop-local.ps1
```

The upstream `scripts/serve.js` listens on loopback only. The helper starts it hidden and stores its PID and logs in ignored `local-runtime/`. It refuses to stop another process using the same port. There is no Windows startup task, firewall change or remote deployment.

Classroom path: **Fly Solo > Career > Start > Fly**. Start with the **Lift Off** lesson in **Angle** mode. W/S adjust throttle, A/D yaw, arrow keys pitch/roll, R resets and Esc pauses. Progress is in each browser's local storage.

The basic solo simulator and its rendering modules are local. The separate leaderboard/community backend is not deployed. Publishing, public tracks and multiplayer rooms are not part of this deployment. Original links to the project, sponsors and donation pages are retained; this is an upstream copy, not a rebranded product.

## Local dependency provenance

`vendor/three@0.160.0/` is the official npm `three@0.160.0` tarball, extracted without running package scripts. Its SHA1 is `cd1e4dbd01aee0719280a9086d75545db52b7a8f`. The package's MIT license is retained. `src/fresh.js` resolves the pinned imports against the site root, and the existing `scripts/gen-preload.js` regenerates preload lists. Physics and course code are unchanged.

The upstream project is GPL-3.0-or-later. Keep `LICENSE` and `NOTICE`, including the separately documented terms and unresolved licenses for third-party art and music. This local preview is not an assertion that every asset has unrestricted redistribution rights.

## Rollback

Run `stop-local.ps1` to stop only the recorded Node process. Leave the checkout and browser storage intact. The upstream baseline is recorded in Git. The local deployment helper changes no other service.
