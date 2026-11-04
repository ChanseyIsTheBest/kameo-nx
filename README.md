<h1 align=center>Kameo: Elements of Power — Nintendo Switch port</h1>

A native Nintendo Switch port of the Xbox 360 version of **Kameo: Elements of Power** (Rare, 2005), title update 2, made with
the ReXGlue static recompiler. The PC build is its intermediary step. You need your own copy of the game: nothing of the game
is in this repository.

> [!NOTE]
> You need your own copy of the Xbox 360 disc (NTSC-U) and title update 2. The DLC packs are optional.

## How to install

1. Open the installer page: **https://chanseyisthebest.github.io/kameo-nx-installer/**
2. Choose your game (the disc image or an extracted folder), then title update 2's package and any DLC packs, and press
   **Create kameo-nx.zip**. Everything runs in your browser and your game files stay on your computer.
3. Extract `kameo-nx.zip` to the root of your SD card. It makes `switch/kameo-nx/`.
4. In **Sphaira**, find Kameo: Elements of Power, press **X**, choose **Install Forwarder**, set **Address Space** to **39-bit**,
   choose **Create Forwarder**, and start the game from the Home Screen. From the second start on it runs on four CPU cores.

Settings are in `switch/kameo-nx/kameo.toml`. To build everything yourself, see [docs/building.md](docs/building.md).

## Development

Status, findings and decisions: [docs/kameo-notes.md](docs/kameo-notes.md). Progress log: [docs/progress-log.md](docs/progress-log.md).
What goes on the SD card: [dist/switch/README.txt](dist/switch/README.txt).

## Credits

- **reNut's renderer** by masterspike52: the Direct3D-level renderer (its author permits its reuse in these ports).
- **Kameo: RePowered** by MaxDeadBear and BiRabittoh (MIT): the code generation findings this port starts from, and every
  part marked as theirs in the sources and in the notes. Its MIT notice is in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
- **katamari-nx**, **banjo-nx**, **sa2-nx**: the base of this port, its CPU passes, its deko3d work, input.
- **NetherSX2_nx** by NaGaa95 (four-core layout, MIT), **battd_nx** (the GPU arena), **bejeweled3-nx** and the **zuma** ports
  (the hot-function method, version 0x0E shader containers).

GPL-3.0 where the sources are.

## Building

[docs/building.md](docs/building.md) has the tools and the order. In short, with the game extracted to `D:\kameo\game`
(`tools/extract_iso.py`) and the update's `default.xexp` next to its `default.xex` (`tools/stfs_extract.py`):

```bash
tools\codegen.ps1
```

```bash
tools\build_pc.bat
```

```bash
powershell -NoProfile -ExecutionPolicy Bypass -File tools/switch_shaders.ps1
```

```bash
tools\build_switch.bat
```

The game's files are never committed or uploaded. The release's NRO holds the recompiled program, its icon and the 139 shaders
that no level or model file holds, and none of the game's levels, models, textures, sound or video.
