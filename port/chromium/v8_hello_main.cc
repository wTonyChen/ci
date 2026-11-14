// v8_hello_main.cc —— M3 前哨：最小的 V8 启动探针。
//
// 目标：证明 "v8::Isolate 能在 Horizon 上初始化，并且能跑一段 JS"。
// 不碰 Chromium 的 content/，只用 v8_monolithic + libplatform，所以失败点很干净：
// 平台初始化、Isolate、Context、编译、执行 —— 五步各一个面包屑。
//
// 崩溃时的判读：nxlog.txt 的最后一行就是出事的那步。
#include <cstdarg>
#include <cstdio>
#include <memory>

#include <switch.h>

#include "libplatform/libplatform.h"
#include "v8.h"

namespace {

void Report(const char* fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  printf("%s\n", buf);
  consoleUpdate(NULL);
  if (FILE* f = fopen("sdmc:/nxlog.txt", "a")) {
    fprintf(f, "%s\n", buf);
    fclose(f);
  }
}

}  // namespace

int main(int argc, char** argv) {
  consoleInit(NULL);
  Report("V8 P1 console up");

  Report("V8 P2 NewDefaultPlatform");
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());

  Report("V8 P3 V8::Initialize");
  v8::V8::Initialize();

  Report("V8 P4 Isolate::New");
  v8::Isolate::CreateParams params;
  params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate* isolate = v8::Isolate::New(params);

  int result = -1;
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    Report("V8 P5 Context::New");
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);

    Report("V8 P6 compile + run '1+2'");
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8Literal(isolate, "1+2");
    v8::Local<v8::Script> script =
        v8::Script::Compile(context, source).ToLocalChecked();
    v8::Local<v8::Value> value = script->Run(context).ToLocalChecked();
    result = value->Int32Value(context).FromJust();
    Report("V8 P7 JS result = %d (expected 3)", result);
  }

  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::DisposePlatform();

  Report(result == 3 ? "V8 PASS" : "V8 FAIL (wrong result)");
  Report("V8 done, press + to exit");
  while (appletMainLoop()) {
    hidScanInput();
    if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS)
      break;
    consoleUpdate(NULL);
    svcSleepThread(50000000ULL);
  }
  consoleExit(NULL);
  return result == 3 ? 0 : 1;
}
