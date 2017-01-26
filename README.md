Luma Updater

**WORK IN PROGRESS** *This is the "reboot" branch, a complete rewrite of Luma Updater, expect  missing features and bugs! (more than standard Luma Updater, at least)*

Updater for Luma3DS as a 3DS homebrew!

## Usage

Refer to the [wiki](https://github.com/Hamcha/lumaupdate/wiki) for usage information.

## Requirements

- Your usual 3DS compilation environment
- Cmake for generating the project
- Latest* [ctrulib](https://github.com/smealum/ctrulib) (the one currently bundled with devKitPro won't cut it)
- [makerom](http://3dbrew.org/wiki/Makerom) and [bannertool](https://github.com/Steveice10/bannertool) somewhere in your `PATH` 
- [sftdlib](https://github.com/xerpi/sftdlib) and [sf2dlib](https://github.com/xerpi/sf2dlib)
- zlib from [Portlibs](https://github.com/devkitPro/3ds_portlibs)

<sup>* ctrulib has breaking changes every once in a while so if you have trouble compiling, the latest tested working commit is [ada9559](https://github.com/smealum/ctrulib/commit/ada9559c11ab1870a9f25ac86c66bbacba206735)</sup>

## Compiling

TODO

## License

The assets and code for the homebrew (code under `source/` and assets under `meta/`) are licensed under the **WTFPL**.  
Refer to `LICENSE.txt` for the full license copy.

The Cmake file uses files from the [3ds-cmake](https://github.com/Lectem/3ds-cmake) project, licensed under **MIT**.
Refer to `LICENSE-3dscmake.txt` for the full license copy.

## Credits

- Luma3DS builds (and development) by [Aurora Wright](https://github.com/AuroraWright), [TuxSH](https://github.com/TuxSH) and [other contributors](https://github.com/AuroraWright/Luma3DS/graphs/contributors)
- Hourlies built and provided by [astronautlevel](https://github.com/astronautlevel2)
- Cmake based on Lectem's [3ds-cmake](https://github.com/Lectem/3ds-cmake) templates
- CIA jingle by [Cydon @ FreeSound](https://www.freesound.org/people/cydon/)

