# kobiQ

Local-first clipboard manager for Linux, inspired by [Ditto](https://sabrogden.github.io/Ditto/).  
Built with **Qt 6** — tested on Fedora (GNOME / Wayland).

No login. No cloud. No telemetry.

## Features

- Clipboard history for **text** and **images**
- Search / filter
- Double-click or Enter to paste
- System tray icon
- Resizable, draggable window
- Configurable max history size (default 1000)
- Export / import history as JSON
- Single-instance toggle via CLI

## Requirements (Fedora)

```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel
# optional — nicer auto-paste on X11:
sudo dnf install xdotool
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

## Run

```bash
./build/kobiQ
```

Toggle an already-running instance:

```bash
./build/kobiQ --toggle
```

### Global hotkey (GNOME / Wayland)

Wayland apps cannot reliably grab system-wide shortcuts themselves.  
Settings → Keyboard → Custom Shortcuts:

| Field    | Value |
|----------|--------|
| Name     | kobiQ |
| Command  | `/full/path/to/kobiQ --toggle` |
| Shortcut | Ctrl+\` (tilde / backtick key) |

## Usage

1. Start kobiQ (it stays in the tray)
2. Copy text or images as usual
3. Open history from the tray, or with your custom shortcut / `--toggle`
4. Type to filter; Enter or double-click to paste
5. Use **☰ → Settings / Export / Import** as needed

History is stored locally under:

`~/.local/share/kobiQ/kobiQ/history.json`

## License

MIT — see [LICENSE](LICENSE).

## Contributing

Issues and pull requests are welcome.
