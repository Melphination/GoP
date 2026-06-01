#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_PATH "config.dat"

static void hash_password(const char *pw, uint8_t out[32]) {
    memset(out, 0, 32);
    unsigned long h = 5381;
    for (int i = 0; pw[i]; i++)
        h = ((h << 5) + h) + (unsigned char)pw[i];
    memcpy(out, &h, sizeof(h));
}

int check_pw(const char *password, Server *sv) {
    uint8_t hashed[32];
    hash_password(password, hashed);
    return memcmp(hashed, sv->password, 32) == 0;
}

int load_servers(Server **out, int *count) {
    FILE *fp = fopen(CONFIG_PATH, "rb");
    if (!fp) { *out = NULL; *count = 0; return -1; }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    *count = (int)(size / sizeof(Server));
    *out = malloc(size);
    if (!*out) { fclose(fp); return -1; }

    fread(*out, sizeof(Server), *count, fp);
    fclose(fp);
    return 0;
}

int save_server(const Server *sv) {
    FILE *fp = fopen(CONFIG_PATH, "ab");
    if (!fp) return -1;
    fwrite(sv, sizeof(Server), 1, fp);
    fclose(fp);
    return 0;
}
