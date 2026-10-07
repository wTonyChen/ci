# port/tree

Everything in this directory **mirrors the root of the Cobalt source tree**.
The `gn-gen` workflow copies it over the checkout:

```bash
cp -rv port/tree/. cobalt-checkout/src/
```

So a file placed here maps directly into `cobalt/src/`:

```
port/tree/starboard/nx/arm64/                    -> src/starboard/nx/arm64/
port/tree/cobalt/build/configs/nx-arm64/args.gn  -> src/cobalt/build/configs/nx-arm64/args.gn
```

## TODO (stage 5)

- [ ] `starboard/nx/arm64/BUILD.gn` (model: `starboard/linux/x64x11/BUILD.gn`)
- [ ] `starboard/nx/arm64/configuration_public.h` (Switch: screen, memory, threads, no IPv6)
- [ ] `starboard/nx/arm64/platform_configuration/{BUILD.gn,configuration.gni}`
- [ ] `starboard/nx/arm64/run_starboard_main.cc`
- [ ] `cobalt/build/configs/nx-arm64/args.gn`
- [ ] register the `nx` platform in `starboard/build/platforms.py`
