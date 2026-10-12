# kobiQ

kobiQ clipboard manager for Linux.  
Built with **Qt 6** — tested on Fedora 42, 43, and 44 (GNOME / Wayland).

No login. No cloud. No telemetry.

## Install on Fedora (no source build)

Pick the RPM that matches **your** Fedora version. Packages are not interchangeable
(`.fc42` / `.fc43` / `.fc44`) because each links against that release’s Qt libraries.

```bash
rpm -E %fedora    # e.g. 44
```

### Option A — download an RPM

1. Open [Releases](https://github.com/kginonredhat/kobiQ/releases)
2. Download `kobiq-*-fcNN.x86_64.rpm` where `NN` is your Fedora version
3. Install:

```bash
sudo dnf install ./kobiq-*-fc$(rpm -E %fedora).x86_64.rpm
```

Then launch **kobiQ** from the app menu, or run `kobiQ`.

### Option B — COPR (recommended for updates)

Once a COPR is enabled for your Fedora release:

```bash
sudo dnf copr enable <owner>/kobiq
sudo dnf install kobiq
```

## Features

- Clipboard history for **text** and **images**
- Search / filter
- Double-click or Enter to paste
- System tray icon
- Resizable, draggable window
- Configurable max history size (default 1000)
- Export / import history as JSON
- Single-instance toggle via CLI
- **☰ → Exit kobiQ** quits completely (× only hides)

## Build from source

### Requirements (Fedora)

```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel
# optional — nicer auto-paste on X11:
sudo dnf install xdotool
```

### Compile

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo cmake --install build
```

Or run without installing:

```bash
./build/kobiQ
```

### Build RPMs

**Your Fedora only** (no containers — recommended on corporate laptops):

```bash
sudo dnf install rpm-build rpmdevtools rsync cmake gcc-c++ qt6-qtbase-devel
chmod +x packaging/build-rpm-native.sh
./packaging/build-rpm-native.sh
sudo dnf install ./dist/fedora-$(rpm -E %fedora)/kobiq-*.x86_64.rpm
```

**Fedora 42, 43, and 44** (needs Podman for non-host releases):

```bash
sudo dnf install podman
chmod +x packaging/build-rpm-multi.sh
./packaging/build-rpm-multi.sh              # builds fc44 natively on F44; uses containers for 42/43
./packaging/build-rpm-multi.sh 44           # only your current Fedora
```

If Podman fails with `x509: certificate signed by unknown authority` (common on corporate networks):

```bash
PODMAN_TLS_VERIFY=false ./packaging/build-rpm-multi.sh 42 43
```

Or use **GitHub Actions**: tag `v0.1.7` and download the `fc42` / `fc43` / `fc44` RPMs from the workflow artifacts or Release page.

Toggle an already-running instance:

```bash
kobiQ --toggle
```

### Global hotkey (GNOME / Wayland)

Wayland apps cannot reliably grab system-wide shortcuts themselves.  
Settings → Keyboard → Custom Shortcuts:

| Field    | Value |
|----------|--------|
| Name     | kobiQ |
| Command  | `kobiq-activate` |
| Shortcut | Ctrl+\` (tilde / backtick key) |

### Clipboard monitoring on GNOME Wayland

GNOME/Mutter does **not** expose the Wayland data-control protocol, so a
background Qt app cannot see Ctrl+C from Chrome, Slack, etc. by itself.

After installing the RPM on GNOME:

```bash
kobiq-enable-gnome-monitor
# then log out and back in once if the extension was just installed
```

Or enable **kobiQ Clipboard Monitor** in Extensions. If GPaste is already
running, kobiQ will also listen to its updates as a fallback.

## Usage

1. Start kobiQ (it stays in the tray / dash)
2. Copy text or images as usual
3. Open history from the tray, or with your custom shortcut / `--toggle`
4. Type to filter; Enter or double-click to paste
5. Use **☰ → Settings / Export / Import / Exit kobiQ** as needed

History is stored locally under:

`~/.local/share/kobiQ/kobiQ/history.json`

## Distribution for outside users

1. **GitHub Releases** — attach `fc42` / `fc43` / `fc44` RPMs per tag  
2. **Fedora COPR** — enable once, then `dnf install kobiq` on each supported release  
3. **Flathub** — later; clipboard/tray sandboxing needs care  
4. **Official Fedora** — package review process

## License

MIT — see [LICENSE](LICENSE).

## Contributing

Issues and pull requests are welcome.
