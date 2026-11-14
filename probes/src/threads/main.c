// Pure-libnx pthread limit probe.
//
// Chromium's ThreadPool asks the process for several worker threads at once.
// Before blaming our port for the "L5" crash, answer a much simpler question on
// device: how many pthreads can this process create, and what does the launcher
// do when we ask for many? No Chromium, no fs, no malloc.
#include <pthread.h>
#include <stdio.h>

#include <switch.h>

#define MAX_THREADS 24

static int g_started;

static void* ThreadMain(void* arg) {
  (void)arg;
  svcSleepThread(3000000000ULL);
  return NULL;
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  consoleInit(NULL);

  printf("probe_threads: pthread_create x%d\n", MAX_THREADS);
  consoleUpdate(NULL);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 64 * 1024);

  for (int i = 0; i < MAX_THREADS; ++i) {
    pthread_t t;
    int rc = pthread_create(&t, &attr, ThreadMain, NULL);
    if (rc != 0) {
      printf("thread %d: pthread_create failed rc=%d\n", i, rc);
      consoleUpdate(NULL);
      break;
    }
    g_started = i + 1;
    if ((i + 1) % 4 == 0) {
      printf("  started %d\n", g_started);
      consoleUpdate(NULL);
    }
  }

  printf("probe_threads: done, started %d/%d\n", g_started, MAX_THREADS);
  printf("press + to exit\n");
  consoleUpdate(NULL);

  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  PadState pad;
  padInitializeDefault(&pad);
  while (appletMainLoop()) {
    padUpdate(&pad);
    if (padGetButtonsDown(&pad) & HidNpadButton_Plus) {
      break;
    }
    consoleUpdate(NULL);
  }
  return 0;
}
