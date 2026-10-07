# port/tree —— 覆盖到 Cobalt 源码树的内容

这里的目录结构**镜像 Cobalt 源码树的根**，CI 的 `gn-gen` 任务会执行：

```bash
cp -rv port/tree/. cobalt-checkout/src/
```

因此本目录下放的文件，就等价于直接写进 `cobalt/src/`：

```
port/tree/starboard/nx/arm64/                    → src/starboard/nx/arm64/
port/tree/cobalt/build/configs/nx-arm64/args.gn  → src/cobalt/build/configs/nx-arm64/args.gn
port/tree/starboard/build/platforms.py           → src/starboard/build/platforms.py（如需打补丁）
```

## 待办（阶段 5）

- [ ] `starboard/nx/arm64/BUILD.gn`（照 `starboard/linux/x64x11/BUILD.gn`）
- [ ] `starboard/nx/arm64/configuration_public.h`（Switch：屏幕/内存/线程/无 IPv6 等约束）
- [ ] `starboard/nx/arm64/platform_configuration/{BUILD.gn,configuration.gni}`
- [ ] `starboard/nx/arm64/run_starboard_main.cc`
- [ ] `cobalt/build/configs/nx-arm64/args.gn`
- [ ] 在 `starboard/build/platforms.py` 注册 `nx` 平台
