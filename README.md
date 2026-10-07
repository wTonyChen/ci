# ci

CI and port payload for the Chromium-NX project (porting Chromium/Cobalt to Nintendo Switch homebrew).

This repository does two things:

1. **`probes` workflow** (auto-triggered on push): builds every Switch probe under `probes/` with
   the official `devkitpro/devkita64` container and uploads the resulting `.nro` files as artifacts.
   These probes are run on real hardware and report `OK=n FAIL=m`.
2. **`gn-gen` workflow** (manual `workflow_dispatch`): runs `gclient sync` + `gn gen` on GitHub's
   network, so the heavy Chromium/Cobalt checkout never goes through the local proxy.
   It also overlays `port/tree/` onto the Cobalt checkout before running gn.

Logs are **not** written back to this repository. Read them with the GitHub CLI:

```bash
gh run list  -R wTonyChen/ci -L 5
gh run view <run-id> -R wTonyChen/ci --log-failed
```

## Layout

| Path | Purpose |
|---|---|
| `probes/` | Switch-side probes (libnx, ASCII-only console output) |
| `probes/src/mmap/` | `mmap`/`munmap`/`mprotect` shim, validated on hardware (`OK=10 FAIL=0`) |
| `port/README.md` | POSIX shim architecture and delivery order |
| `port/tree/` | Files copied over the Cobalt source tree (Starboard `nx` platform files) |

## Requirements

- `probes` needs no configuration.
- `gn-gen` needs Actions enabled for the repository. No secrets or PATs are required.
