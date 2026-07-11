# osc-macro

A tiny OSC macro daemon.

`osc-macro` listens for incoming OSC UDP messages on port `2223`, matches them against a small macro file, and sends any configured response messages back to the sender. It was primarily developed for use with the Behringer WING and its missing macro system, so you can build your own macros for things like CC buttons. It is still intended to stay small and portable, so it can also run on other platforms.

**This project is work in progress**. The core trigger/response flow is usable, but the feature set is still evolving.

## How it works

- Define one or more macros in a plain text file.
- Each macro has a trigger message and optional response messages.
- When a trigger matches an incoming OSC message, the matching responses are built and sent.
- Responses can be either direct OSC snippets or response factory invocations.
- Response factories let you generate a response dynamically from named arguments, which is useful when the final OSC address or payload depends on runtime input.

## Response Factories

Response factories are registered in linked translation units and can then be referenced from the macro file by name.

The included binary ships an `echo` factory in its own translation unit. It expects the first argument to be an OSC address string, then copies any remaining arguments into the outgoing message.

Example:

```text
/channel/3/test()
> echo("/hello/world" 1 2 3.14f)
```

This keeps the macro file compact while still allowing responses that are assembled dynamically in C. To add another factory, define it in a separate translation unit and register it with `OSC_REGISTER_MACRO_RESPONSE_FACTORY("name", callback);`. This global registration is done using a constructor function, so this is currently only supported by GCC and Clang.

## Main Loop Hooks

For periodic or protocol-specific background work, the daemon exposes a separate main-loop hook API. Hooks are also registered from linked translation units with a constructor, then loaded into the runtime before the main loop starts.

This is a good fit for the Behringer WING keepalive, because the hook can watch `OSC_MAIN_LOOP_EVENT_TICK` and send `/*s` whenever its timer expires without mixing that logic into macro parsing or response factories.

Example:

```c
static bool my_hook(osc_main_loop_event_type event_type, osc_main_loop_context *context)
{
   if (event_type != OSC_MAIN_LOOP_EVENT_TICK)
      return true;

   return true;
}

OSC_REGISTER_MAIN_LOOP_HOOK("my_hook", my_hook);
```

The factory callback is expected to have the following signature:

```c
void my_factory_callback(tosc_message_builder *builder, tosc_message_argument args[], size_t arg_count);
```

## Planned

- OSC argument wildcards in trigger messages.
- Dynamic response arguments derived from wildcarded input arguments.

## Macro format

The included [macros.txt](macros.txt) shows the expected format:

```text
/channel/1/test("hello")
> /channel/1/another()
> /channel/1/third(1 2 3.14f "test")
> echo("/hello/world" 1 2 3.14f)
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

To lock the daemon to a specific peer, pass an optional IPv4 address. In that mode, the daemon only accepts packets from that source address and sends responses back to it:

```sh
./osc-macro macros.txt 192.168.1.50
```

## Building for OpenWRT

As mentioned above, you can build the package for OpenWRT with the default target. The resulting `.ipk` or `.apk` file can then be installed on your OpenWRT device.

1. To build and package for an OpenWRT target, you need to first set up the correct OpenWRT SDK for your OpenWRT version and target platform. You can find precompiled SDKs for many versions and platforms on the [OpenWRT downloads page](https://downloads.openwrt.org/).

2. After downloading and extracting the SDK, navigate to the SDK root directory and copy the `osc-macro` repository into the `package` directory. Inside the respository then run

   ```sh
   ./configure-openwrt.sh
   ```

   to make a build environment for the OpenWRT SDK.

3. From the SDK root directory, run the following commands to update the package feeds and build the package:

   ```sh
   ./scripts/feeds update -a
   ./scripts/feeds install -a
   make defcofig
   make package/osc-macro/compile V=s
   ```

4. The resulting `.ipk` or `.apk` file will be located in the `bin/packages/<architecture>/base/osc-macro_<version>-<architecture>.<ipk|apk>` directory.

5. You can then transfer this file to your OpenWRT device (e.g. using `scp`) and install it using the `opkg` or `apk` package manager:

   ```sh
   opkg install /path/to/osc-macro_<version>-<architecture>.ipk
   ```

   or

   ```sh
   apk add --allow-untrusted /path/to/osc-macro_<version>-<architecture>.apk
   ```

## Acknowledgements

This project incorporates code from:

- [tinyosc](https://github.com/mhroth/tinyosc) - Copyright (C) 2015 mhroth
  Licensed under the ISC License
- [nob.h](https://github.com/tsoding/nob.h) - Copyright (C) 2024 Alexey Kutepov
  Licensed under the MIT License

---

> [matteolutz.de](https://matteolutz.de) &nbsp;&middot;&nbsp;
> GitHub [@matteolutz](https://github.com/matteolutz) &nbsp;&middot;&nbsp;
> Email [info@matteolutz.de](mailto:info@matteolutz.de)
