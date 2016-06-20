# Luma Updater

Formerly known as "ARN Updater"

A `arm9loaderhax.bin` updater for Luma3DS (formerly AuReiNand) as a 3DS homebrew (no more SD swaps!)

## Usage

Refer to the [wiki](https://github.com/Hamcha/lumaupdate/wiki) for usage information.

## Requirements

- Your usual 3DS compilation environment
- Latest* ctrulib (the one currently bundled with devKitPro won't cut it)
- [makerom](http://3dbrew.org/wiki/Makerom) and [bannertool](https://github.com/Steveice10/bannertool) somewhere in your `PATH` environment
- zlib (get it from [devkitPro/3ds_portlibs](https://github.com/devkitPro/3ds_portlibs))

<sup>* ctrulib has breaking changes every once in a while so if you have trouble compiling, the latest tested working commit is [56a1119](https://github.com/smealum/ctrulib/commit/56a11197c7543daa10e2cb27238d36b210e91a83)</sup>

#### Optional

- `zip` binary for generating release archives (`make pkg`)

## Compiling

`make` should create the output folder and fill it with both the 3dsx and the cia builds

`make 3dsx` will only build the 3dsx version

#### Extra flags

`make CITRA=1` disables features that aren't properly emulated on Citra (HTTPc) for easier testing

`make DEBUG=1` will disable compile-time optimizations entirely

## License

The assets and code for the homebrew (code under `source/` and assets under `meta/`) are licensed under the **WTFPL**.  
Refer to `LICENSE.txt` for the full text.

This project uses [jsmn](https://github.com/zserge/jsmn), which is licensed under the MIT license.  
Refer to `LICENSE.jsmn.txt` for the full text.

This project uses [minizip](https://github.com/nmoinvaz/minizip), which is licensed under the zlib license.  
Refer to `LICENSE.minizip.txt` for the full text.

This project uses [libmd5-rfc](https://sourceforge.net/projects/libmd5-rfc/), which is licensed under the BSD license.  
Refer to `LICENSE.libmd5-rfc.txt` for the full text.

## Credits

- Luma3DS builds (and development) by [Aurora Wright](https://github.com/AuroraWright), [TuxSH](https://github.com/TuxSH) and [other contributors](https://github.com/AuroraWright/Luma3DS/graphs/contributors)
- Hourlies built and provided by [astronautlevel](https://github.com/astronautlevel2)
- Makefiles are somewhat derived by stripping down [Steveice10](https://github.com/Steveice10)'s buildtools
- CIA jingle by [Cydon @ FreeSound](https://www.freesound.org/people/cydon/)
