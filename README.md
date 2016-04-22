# ARN Updater

A `arm9loaderhax.bin` updater for AuReiNand as a 3DS homebrew (no more SD swaps!)

## Requirements

- Your usual 3DS compilation environment
- Latest ctrulib (the one currently bundled with devKitPro won't cut it)

You can use `make CITRA=1` to disable features that aren't properly emulated on Citra (HTTPc) for easier testing

## What's missing (aka TODO)

- Support for [astronautlevel's hourlies](https://astronautlevel2.github.io/AuReiNand/)
- CIA compiling (with its own shiny banner and everything)
