#ifndef TEAM_H
#define TEAM_H

#include <time.h>
#include "protocol.h"

#define MAX_TEAMS         32
#define MAX_TEAM_REQUESTS 64
#define TEAM_NAME_LEN     21 /* toi da 20 ky tu + '\0' */
#define USER_NAME_LEN     32

/* Ma ket qua cua cac ham trong team.c */
#define TEAM_OK               0
#define TEAM_ERR_NAME        -1 /* ERR 400 */
#define TEAM_ERR_EXISTS      -2 /* ERR 409 */
#define TEAM_ERR_IN_TEAM     -3 /* ERR 412 */
#define TEAM_ERR_NO_SLOT     -4 /* ERR 500 */
#define TEAM_ERR_NOT_FOUND   -5 /* ERR 404 */
#define TEAM_ERR_NOT_CAPTAIN -6 /* ERR 403 */
#define TEAM_ERR_NOT_IN_TEAM -7 /* ERR 412 */
#define TEAM_ERR_FULL        -8 /* ERR 421 */
#define TEAM_ERR_DUP_REQ     -9 /* ERR 409 */

#define REQ_JOIN   1
#define REQ_INVITE 2

typedef struct {
    int used;
    char name[TEAM_NAME_LEN];
    /* members[0] luon la doi truong */
    char members[MAX_TEAM_SIZE][USER_NAME_LEN];
    int member_count;
} Team;

typedef struct {
    int used;
    int type; /* REQ_JOIN hoac REQ_INVITE */
    char team[TEAM_NAME_LEN];
    char user[USER_NAME_LEN];
    time_t created_at;
} TeamRequest;

void team_init(void);
const char *team_strerror(int rc);
int team_valid_name(const char *name);
Team *team_find(const char *name);
Team *team_find_by_member(const char *user);
int team_create(const char *name, const char *captain);
void team_list(char *out, int out_size);
void team_members_str(const Team *t, char *out, int out_size);
int team_is_captain(const Team *t, const char *user);
int team_add_member(Team *t, const char *user);
int team_remove_member(Team *t, const char *user);
void team_disband(Team *t);

int req_add(int type, const char *team, const char *user);
TeamRequest *req_find(int type, const char *team, const char *user);
void req_remove(TeamRequest *r);
void req_clear_team(const char *team);
void req_clear_user(const char *user);
TeamRequest *req_at(int idx);

#endif