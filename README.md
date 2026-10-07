# ci —— Chromium-NX 的 CI 与移植负载

这个仓库只做两件事：

1. **`probes` 工作流**（push 自动触发）：装 devkitPro，编译 `probes/` 下全部 Switch 探针，产出 `.nro` 作为 artifact。
   用来保证移植 shim 不回归，并给真机验证提供产物。
2. **`gn-gen` 工作流**（手动触发 `workflow_dispatch`）：在 GitHub 机房做 `gclient sync`（**不走你的梯子，不花你的流量**），
   把 `port/tree/` 覆盖进 Cobalt 源码树，跑 `gn gen`，**并把完整日志提交回本仓库的 `out-logs/`**。

## 为什么日志要提交回仓库

这样任何人在本地 `git pull` 就能拿到 CI 的完整输出，不需要 PAT、不需要翻 Actions 页面。

## 首次使用需要做的一次设置

仓库 **Settings → Actions → General → Workflow permissions** 选 **Read and write permissions**
（否则 `gn-gen` 最后一步把日志提交回来会 403）。

## 目录

| 路径 | 说明 |
|---|---|
| `probes/` | Switch 侧探针（libnx，ASCII 输出，真机跑出 `OK=n FAIL=m`） |
| `probes/src/mmap/` | **已在真机验证通过（OK=10 FAIL=0）**的 `mmap`/`munmap`/`mprotect` shim |
| `port/README.md` | POSIX shim 分层架构与交付顺序 |
| `port/tree/` | 覆盖进 Cobalt 源码树的内容（Starboard `nx` 平台文件等） |
| `out-logs/` | CI 写回：`gn-gen.log`、`meta.txt` |
