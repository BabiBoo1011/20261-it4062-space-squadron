#include "auth.h"
#include <stdio.h>
#include <string.h>

#define ACCOUNTS_FILE "data/accounts.txt"

static Account accounts[MAX_ACCOUNTS];
static int account_count = 0;

void auth_init(void) {
    account_count = 0;
    FILE *f = fopen(ACCOUNTS_FILE, "r");
    if (!f) return;

    char line[160];
    while (fgets(line, sizeof(line), f) && account_count < MAX_ACCOUNTS) {
        Account a;
        if (sscanf(line, "%31[^:]:%63[^:]:%d:%d", a.username, a.password, &a.wins, &a.losses) == 4) {
            accounts[account_count++] = a;
        }
    }
    fclose(f);
}

static int find_account(const char *user) {
    for (int i = 0; i < account_count; i++) {
        if (strcmp(accounts[i].username, user) == 0) return i;
    }
    return -1;
}

int auth_register(const char *user, const char *pass) {
    if (!user || !pass || strlen(user) < 2 || strlen(pass) < 3) return -1;
    if (strchr(user, ':') || strchr(pass, ':')) return -1;
    if (find_account(user) >= 0 || account_count >= MAX_ACCOUNTS) return -1;

    Account a;
    snprintf(a.username, sizeof(a.username), "%s", user);
    snprintf(a.password, sizeof(a.password), "%s", pass);
    a.wins = 0;
    a.losses = 0;
    accounts[account_count++] = a;

    FILE *f = fopen(ACCOUNTS_FILE, "a");
    if (f) {
        fprintf(f, "%s:%s:%d:%d\n", a.username, a.password, a.wins, a.losses);
        fclose(f);
    }
    return 0;
}

int auth_verify(const char *user, const char *pass) {
    int idx = find_account(user);
    if (idx < 0) return 0;
    return strcmp(accounts[idx].password, pass) == 0;
}
