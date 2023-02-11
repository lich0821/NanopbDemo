# Nanopb Demo
This is a demo ported from [nanopb](https://github.com/nanopb/nanopb) example [simple](https://github.com/nanopb/nanopb/blob/master/examples/simple).

## Quickstart
### Install `nanopb_generator`
To use nanopb, one needs to use `nanopb_generator` to generate `.pb.c` and `.pb.h`.

To "install", just copy files in [generator](https://github.com/nanopb/nanopb/tree/master/generator) into local path: `C:\Tools\nanopb`.

### Install `nanopb`
A simple way is to use [vcpkg](https://github.com/microsoft/vcpkg/blob/master/README_zh_CN.md):
```sh
vcpkg install nanopb:x86-windows-static
```

### Build with VS2019
Like other vs projects.

### Run
Double click `NanopbDemo.exe` under `NanopbDemo\Release`.
