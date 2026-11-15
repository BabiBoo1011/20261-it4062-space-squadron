#ifndef AUTH_H
#define AUTH_H

#define MAX_ACCOUNTS 256

typedef struct {
    char username[32];
    char password[64];
    int wins;
    int losses;
} Account;

void auth_init(void);
int auth_register(const char *user, const char *pass);
int auth_verify(const char *user, const char *pass);

#endif
