// Persistent, allocation-free logging for early bring-up code.
//
// Why: the interesting failures (partition_alloc CHECKs) happen *before* the
// console works, and console/printf need malloc which may itself be routed
// through the very allocator that is still initialising. So keep the messages
// in a static buffer and append them to sdmc:/nxlog.txt with the libnx fs API,
// which only needs caller-provided storage.
#include <string.h>

#include <switch.h>

namespace {
char g_buf[6 * 1024];
size_t g_len;

// libnx fs state lives in static storage as well (no malloc).
FsFileSystem g_fs;
bool g_fs_ready;
}  // namespace

extern "C" {

void nx_log(const char* text, unsigned long length) {
  if (!text || length == 0) {
    return;
  }
  if (g_len + length >= sizeof(g_buf)) {
    // Keep the tail (most recent messages are the interesting ones).
    const size_t drop = g_len + length - sizeof(g_buf) + 1;
    if (drop >= g_len) {
      g_len = 0;
    } else {
      memmove(g_buf, g_buf + drop, g_len - drop);
      g_len -= drop;
    }
  }
  const size_t room = sizeof(g_buf) - 1 - g_len;
  const size_t n = length < room ? length : room;
  memcpy(g_buf + g_len, text, n);
  g_len += n;
  g_buf[g_len] = '\0';
}

void nx_log_str(const char* text) {
  if (!text) {
    return;
  }
  size_t n = 0;
  while (text[n]) {
    ++n;
  }
  nx_log(text, n);
}

int nx_log_flush(void) {
  if (!g_fs_ready) {
    if (R_FAILED(fsOpenSdCardFileSystem(&g_fs))) {
      return -1;
    }
    g_fs_ready = true;
  }

  FsFile file;
  if (R_FAILED(fsFsOpenFile(&g_fs, "/nxlog.txt",
                            FsOpenMode_Write | FsOpenMode_Append, &file))) {
    if (R_FAILED(fsFsCreateFile(&g_fs, "/nxlog.txt", 0, 0))) {
      return -2;
    }
    if (R_FAILED(fsFsOpenFile(&g_fs, "/nxlog.txt",
                              FsOpenMode_Write | FsOpenMode_Append, &file))) {
      return -3;
    }
  }

  s64 size = 0;
  if (R_FAILED(fsFileGetSize(&file, &size))) {
    size = 0;
  }
  if (g_len > 0) {
    fsFileWrite(&file, size, g_buf, g_len, FsWriteOption_Flush);
  }
  fsFileClose(&file);
  g_len = 0;
  return 0;
}

}  // extern "C"
