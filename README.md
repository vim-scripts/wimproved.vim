## :sparkles: wimproved.vim

### Introduction
An effort to create a better editing experience for Vim on Windows.<br>
Supports fullscreen while taking care to fix visual glitches seen in
other plugins.

Put the following in your .vimrc and enjoy!

    autocmd GUIEnter * WToggleClean

### Commands
`:WCenter {scale}`
  - Centers the window on the current monitor.  If scale is non-zero, resizes<br>
    the window as a percentage (scale / 100) of the current monitor size.

`:WSetAlpha {alpha}`
  - Sets the alpha of the window to the given value.

`:WToggleFullscreen`
  - Toggles full-screen support.

`:WToggleClean`
  - Toggles between the default and 'clean' window styling.

### Installation
Install using your favorite plugin manager.  The plugin expects wimproved.dll to exist in the root plugin folder.
If you have `cmake` and Visual Studio installed run the following:

```
cmake -G "NMake Makefiles" . && nmake
```


### Contributions
Contributions and pull requests are welcome.

### License

This software is licensed under the [MIT license](http://en.wikipedia.org/wiki/MIT_License).
© 2015 Killian Koenig &lt;<killiankoenig@gmail.com>&gt;.


