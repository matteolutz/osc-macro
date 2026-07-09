# osc-macro

A tiny OSC macro daemon.

`osc-macro` listens for incoming OSC UDP messages on port `2223`, matches them against a small macro file, and sends any configured response messages back to the sender. It was primarily developed for use with the Behringer WING and its missing macro system, so you can build your own macros for things like CC buttons. It is still intended to stay small and portable, so it can also run on other platforms.

**This project is work in progress**. The core trigger/response flow is usable, but the feature set is still evolving.

## How it works

- Define one or more macros in a plain text file.
- Each macro has a trigger message and optional response messages.
- When a trigger matches an incoming OSC message, the matching responses are built and sent.

## Planned

- OSC argument wildcards in trigger messages.
- Dynamic response arguments derived from wildcarded input arguments.

## Macro format

The included [macros.txt](macros.txt) shows the expected format:

```text
/channel/1/test("hello")
> /channel/1/another()
> /channel/1/third(1 2 3.14f "test")
```

## Build and run

For OpenWrt, build with the default target:

```sh
make
./osc-macro macros.txt
```

For other platforms, or when you want a local build without `-lrt`, use:

```sh
make dev
./osc-macro macros.txt
```

## Acknowledgements

This project incorporates code from:

- [tinyosc](https://github.com/mhroth/tinyosc) - Copyright (C) 2026 mhroth  
  Licensed under the ISC License

---

> [matteolutz.de](https://matteolutz.de) &nbsp;&middot;&nbsp;
> GitHub [@matteolutz](https://github.com/matteolutz) &nbsp;&middot;&nbsp;
> Email [info@matteolutz.de](mailto:info@matteolutz.de)
