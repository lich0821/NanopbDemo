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

## Change log
### V1
Demo `oneof` message.
```proto
// Check demo.proto for detail.
message Request
{
    Functions func = 1;
    oneof msg
    {
        Empty empty    = 2;
        String string  = 3;
        TextMsg txt    = 4;
        ImageMsg img   = 5;
        DbQuery query  = 6;
        Verification v = 7;
    }
}

message Response
{
    Functions func = 1;
    oneof msg
    {
        int32 status      = 2;
        String string     = 3;
        WxMsg wxmsg       = 4;
        MsgTypes types    = 5;
        Contacts contacts = 6;
        DbNames dbs       = 7;
        DbTables tables   = 8;
        DbRows rows       = 9;
    };
}
```

### V0
```proto
syntax = "proto3";

message Response {
  int32 code = 1;
}
```
