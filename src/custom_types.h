#ifndef CUSTOM_TYPES_H
#define CUSTOM_TYPES_H

#include <stdint.h>
#include <netinet/in.h>

#define MAX_PATH_LEN  1024
#define COLOR_LOG     "\x1b[32m"
#define COLOR_ERROR   "\x1b[31m"
#define COLOR_RESET   "\x1b[0m"

typedef enum {
    UPLOAD,
    MODIFY,
    DELETE_,
    MOVE,
    RENAME
} FileOp;

typedef enum {
    ADMIN,
    USER,
    GUEST
} UserRole;

typedef struct {
    int year, month, day, hour, min, sec;
    char type[50];
    char prev[100];
    char aft[100];
} Log;

typedef struct {
    int      serverid;
    int      serverip;
    char     name[50];
    uint8_t  password[32];
} Server;

typedef union {
    struct in_addr  ipv4;
    struct in6_addr ipv6;
    char            hostname[100];
} Address;

typedef union {
    char message[256];
    int  error_code;
} EventData;

#endif