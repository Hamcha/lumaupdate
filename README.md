# Luma3DS Updater

Formerly known as "ARN Updater"

A `arm9loaderhax.bin` updater for Luma3DS (formerly AuReiNand) as a 3DS homebrew (no more SD swaps!)

## Requirements

- Your usual 3DS compilation environment
- Latest ctrulib (the one currently bundled with devKitPro won't cut it)
- [makerom](http://3dbrew.org/wiki/Makerom) and [bannertool](https://github.com/Steveice10/bannertool) somewhere in your $PATH
- zlib (get it from [devkitPro/3ds_portlibs](https://github.com/devkitPro/3ds_portlibs))

#### Optional

- `zip` binary for generating release archives (`make pkg`)

#### Extra flags

You can use `make CITRA=1` to disable features that aren't properly emulated on Citra (HTTPc) for easier testing