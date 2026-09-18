# The Spiky Channel

Public Wii homebrew side project.

Phase 1 is split into a tiny Homebrew Channel forwarder and a larger USB core.
It does not install a channel, write to NAND, patch IOS, or modify the Wii
System Menu.

## Phase 1 Goal

Build and run a small Wii homebrew app that shows:

```text
The Spiky Channel
Development Build

USB: Connected / Not Connected

A = Continue
HOME = Exit
```

## Layout

```text
TheSpikyChannel/
├── Makefile
├── source/
│   └── main.c
├── apps/
│   └── the_spiky_channel/
│       └── meta.xml
└── docs/
    └── wad-analysis.md
```

After a successful build:

- `forwarder/apps/the_spiky_channel/boot.dol` is the small Homebrew Channel
  loader.
- `boot.dol` at the repository root is the Spiky core and belongs at
  `USB:/spiky/core/boot.dol`.

The intended test USB layout is:

```text
USB:/
├── apps/
│   └── the_spiky_channel/
│       ├── boot.dol
│       └── meta.xml
└── spiky/
    └── core/
        └── boot.dol
```

## Build Requirements

- devkitPro
- devkitPPC
- libogc
- libfat

On macOS with a configured devkitPro install, this should build with:

```sh
make
make hbc
```

The current machine does not have `DEVKITPRO`, `DEVKITPPC`, or
`powerpc-eabi-gcc` configured yet, so compilation cannot be completed locally
until the Wii toolchain is installed.

## Safety Rules

- Do not install WADs automatically.
- Do not write to NAND in Phase 1.
- Do not modify IOS or the System Menu.
- Test forwarder and WAD work in Dolphin before real hardware.
- Keep USB functionality separate from future NAND/channel packaging.
