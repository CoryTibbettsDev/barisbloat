# barisbloat
A statusbar for the X windowing system using the XCB library

# Dependencies
## Libraries
- libxcb
- libxcb-util
- libxcb-randr
## Packages (Names Are From Arch Linux Repositories)
- libxcb
- xcb-util

# Configuration and Cuztimization
Use `make config` to create the config.h file and change the values as you see fit.

# Installation
Simply `make install` or `make uninstall` to install and uninstall respectively. The config.h file must be generated before compilation.

# Thanks
Thank you, in no paticular order to, lemonbar, i3WM, AwesomeWM, suckless, and the many people whose articles, source code and stack overflow answers helped me with this project.

# Useful Links
[Great Article and Code on Xorg Fonts and How They Relate to xcb](https://venam.nixers.net/blog/unix/2018/09/02/fonts-xcb.html)<br  />
[Why the Default Make Location is /usr/local](https://unix.stackexchange.com/questions/8656/usr-bin-vs-usr-local-bin-on-linux)<br  />
