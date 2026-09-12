# devkitPro Setup Notes

This project needs the Wii devkitPro stack:

- devkitPPC
- libogc
- libfat
- Wii tools such as `elf2dol`

On macOS, use the official devkitPro pacman installer, then install the Wii
development package group. Do this manually; it writes to `/opt/devkitpro`.

Typical environment once installed:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=$DEVKITPRO/devkitPPC
export PATH=$DEVKITPRO/tools/bin:$DEVKITPPC/bin:$PATH
```

Then build:

```sh
cd /Users/gianschreiner/Documents/Playground/TheSpikyChannel
make
make hbc
```

Expected Homebrew Channel SD or USB layout:

```text
apps/
└── the_spiky_channel/
    ├── boot.dol
    └── meta.xml
```

Current local status:

```text
DEVKITPRO: not set
DEVKITPPC: not set
powerpc-eabi-gcc: not found
```

