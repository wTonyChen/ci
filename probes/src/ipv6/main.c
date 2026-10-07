// probe_ipv6 -- spike #2: does bsd support AF_INET6?
// Starboard 17+ assumes IPv6 works; switchbrew says bsd:u registers only AF_INET/AF_ROUTE.
// ASCII ONLY (libnx console font is ASCII). Press + to exit.
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <switch.h>

static int n_ok = 0, n_fail = 0;

static void try_socket(const char *dom, int domain, int type)
{
    const char *tn = (type == SOCK_STREAM) ? "STREAM" : "DGRAM";
    errno = 0;
    int fd = socket(domain, type, 0);
    if (fd >= 0) { printf("  [ OK ] socket(%-8s,%-6s) fd=%d\n", dom, tn, fd); close(fd); n_ok++; }
    else { printf("  [FAIL] socket(%-8s,%-6s) errno=%d %s\n", dom, tn, errno, strerror(errno)); n_fail++; }
    consoleUpdate(NULL);
}

static void try_resolve(const char *host, int family)
{
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    errno = 0;
    int rc = getaddrinfo(host, "443", &hints, &res);
    if (rc == 0 && res) {
        char buf[INET6_ADDRSTRLEN] = {0};
        void *a = NULL;
        if (res->ai_family == AF_INET)  a = &((struct sockaddr_in  *)res->ai_addr)->sin_addr;
        if (res->ai_family == AF_INET6) a = &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr;
        const char *fam = (res->ai_family == AF_INET) ? "AF_INET"
                        : (res->ai_family == AF_INET6) ? "AF_INET6" : "other";
        if (a) inet_ntop(res->ai_family, a, buf, sizeof(buf));
        printf("  [ OK ] resolve %-18s fam=%-7s %s\n", host, fam, buf);
        n_ok++; freeaddrinfo(res);
    } else {
        printf("  [FAIL] resolve %-18s rc=%d %s\n", host, rc, gai_strerror(rc));
        n_fail++;
    }
    consoleUpdate(NULL);
}

static void try_connect6(const char *ip, int port)
{
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd < 0) { printf("  [FAIL] connect6 socket() errno=%d\n", errno); n_fail++; consoleUpdate(NULL); return; }
    struct sockaddr_in6 sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin6_family = AF_INET6;
    sa.sin6_port   = htons(port);
    if (inet_pton(AF_INET6, ip, &sa.sin6_addr) != 1) {
        printf("  [FAIL] connect6 inet_pton(%s)\n", ip); n_fail++; close(fd); consoleUpdate(NULL); return;
    }
    errno = 0;
    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) == 0) {
        printf("  [ OK ] connect [%s]:%d OK -- IPv6 WORKS\n", ip, port); n_ok++;
    } else {
        printf("  [FAIL] connect [%s]:%d errno=%d %s\n", ip, port, errno, strerror(errno)); n_fail++;
    }
    close(fd); consoleUpdate(NULL);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    consoleInit(NULL);
    printf("=== probe_ipv6 ===\n");
    printf("gcc %s  ptr=%dbit\n\n", __VERSION__, (int)(sizeof(void *) * 8));

    Result nifm = nifmInitialize(NifmServiceType_User);
    Result bsd  = socketInitializeDefault();
    printf("nifmInit=0x%08x socketInit=0x%08x\n", nifm, bsd);
    if (R_FAILED(bsd)) printf("!! bsd init failed\n");
    consoleUpdate(NULL);

    printf("\n-- 1) socket() domain matrix --\n");
    printf("   bsd:u registers only AF_INET / AF_ROUTE\n");
    try_socket("AF_INET",  AF_INET,  SOCK_STREAM);
    try_socket("AF_INET",  AF_INET,  SOCK_DGRAM);
    try_socket("AF_INET6", AF_INET6, SOCK_STREAM);
    try_socket("AF_INET6", AF_INET6, SOCK_DGRAM);
#ifdef AF_UNIX
    try_socket("AF_UNIX",  AF_UNIX,  SOCK_STREAM);
#endif
#ifdef AF_ROUTE
    try_socket("AF_ROUTE", AF_ROUTE, SOCK_RAW);
#endif

    printf("\n-- 2) socketpair --\n");
    {
        int sv[2] = {-1, -1};
        errno = 0;
        if (socketpair(AF_INET, SOCK_STREAM, 0, sv) == 0) {
            printf("  [ OK ] socketpair = %d,%d\n", sv[0], sv[1]);
            close(sv[0]); close(sv[1]); n_ok++;
        } else { printf("  [FAIL] socketpair errno=%d %s\n", errno, strerror(errno)); n_fail++; }
        consoleUpdate(NULL);
    }

    printf("\n-- 3) getaddrinfo --\n");
    try_resolve("example.com", AF_UNSPEC);
    try_resolve("example.com", AF_INET);
    try_resolve("example.com", AF_INET6);
    try_resolve("ipv6.google.com", AF_INET6);

    printf("\n-- 4) real IPv6 connect --\n");
    try_connect6("2606:4700:4700::1111", 443);

    printf("\n========================\n");
    printf("OK=%d FAIL=%d\n", n_ok, n_fail);
    if (n_fail == 0)   printf(">> IPv6 OK, risk cleared\n");
    else if (n_ok > 0) printf(">> partial, check which failed\n");
    else               printf(">> IPv6 NOT usable\n");
    printf("\nPress + to exit\n");
    consoleUpdate(NULL);

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
    while (appletMainLoop()) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_Plus) break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    socketExit();
    nifmExit();
    return 0;
}
