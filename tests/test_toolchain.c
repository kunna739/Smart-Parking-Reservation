#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <sqlite3.h>

int main(void) {
    printf("=== C11 Toolchain & Library Verification ===\n");

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    printf("[PASS] C11 Standard is active (STDC_VERSION: %ld)\n", __STDC_VERSION__);
#else
    printf("[FAIL] C11 Standard is NOT active (STDC_VERSION: %ld)\n", (long)__STDC_VERSION__);
    return 1;
#endif

    // Test SQLite3 linkage & version
    const char* sqlite_ver = sqlite3_libversion();
    if (sqlite_ver != NULL) {
        printf("[PASS] SQLite3 linked successfully (Version: %s)\n", sqlite_ver);
    } else {
        printf("[FAIL] SQLite3 linkage failed\n");
        return 1;
    }

    // Test Winsock2 initialization
    WSADATA wsa;
    int wsa_res = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (wsa_res == 0) {
        printf("[PASS] Winsock2 initialized successfully (Version %d.%d)\n",
               LOBYTE(wsa.wVersion), HIBYTE(wsa.wVersion));
        WSACleanup();
    } else {
        printf("[FAIL] Winsock2 initialization failed with code: %d\n", wsa_res);
        return 1;
    }

    printf("[SUCCESS] Toolchain and all required C11 native libraries verified!\n");
    return 0;
}
