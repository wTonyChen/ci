#!/usr/bin/env bash
# 编译全部 spike 探针
set -uo pipefail

if [ -z "${DEVKITPRO:-}" ]; then
  export DEVKITPRO=/opt/devkitpro
fi
if [ ! -d "$DEVKITPRO" ]; then
  echo "找不到 DEVKITPRO=$DEVKITPRO —— 先跑 setup/wsl-bootstrap.sh" >&2
  exit 1
fi

cd "$(dirname "$0")"

PASS=0; FAIL=0; FAILED=""
for p in ipv6 posix gl mmap; do
  echo
  echo "==================== probe: $p ===================="
  if make -s PROBE="$p" 2>&1; then
    # OUTPUT 是绝对路径，.nro 落在当前目录（不是 build/$p/）
    nro="probe_$p.nro"
    if [ -f "$nro" ]; then
      mkdir -p "build/$p" && cp -f "$nro" "build/$p/$nro"
      sz=$(stat -c%s "$nro" 2>/dev/null || echo '?')
      echo "[ OK ] build/$p/$nro  (${sz} bytes)"
      PASS=$((PASS+1))
    else
      echo "[FAIL] 没有产出 .nro"
      FAIL=$((FAIL+1)); FAILED="$FAILED $p"
    fi
  else
    echo "[FAIL] 编译失败"
    FAIL=$((FAIL+1)); FAILED="$FAILED $p"
  fi
done

echo
echo "=================================================="
echo "成功 $PASS / 失败 $FAIL${FAILED:+   (失败:$FAILED)}"
echo
echo "产物位置（拷到 SD 卡 sd:/switch/ 下，从 hbmenu 启动，建议 application 模式）："
ls -1 build/*/probe_*.nro 2>/dev/null || echo "  （无）"
echo
echo "编译报错请原样发我 —— 首次编译需要按实际头文件微调，这是预期内的。"
