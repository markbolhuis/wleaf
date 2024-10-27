# wleaf

wleaf is a C23 abstraction library for writing Wayland clients.

## Building

```shell
git clone --recurse-submodules https://github.com/markbolhuis/wleaf.git
cd wleaf
meson setup build
meson compile -C build
```

If building on a distribution with up-to-date packages, such as ArchLinux
then `--recurse-submodules` can be omitted.
