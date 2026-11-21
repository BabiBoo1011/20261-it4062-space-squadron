#include "team.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static Team teams[MAX_TEAMS];

void team_init(void) {
    memset(teams, 0, sizeof(teams));
}

/* Doi ma ket qua thanh "<ma_loi> <mo_ta>" de gui kem sau "ERR " */
const char *team_strerror(int rc) {
    switch (rc) {
    case TEAM_ERR_NAME:        return "400 invalid team name";
    case TEAM_ERR_EXISTS:      return "409 team already exists";
    case TEAM_ERR_IN_TEAM:     return "412 already in a team";
    case TEAM_ERR_NO_SLOT:     return "500 server is full";
    default:                   return "500 unknown error";
    }
}

/* Ten doi: 3-20 ky tu, chi gom chu, so va dau '_' */
int team_valid_name(const char *name) {
    int len = (int)strlen(name);
    if (len < 3 || len > TEAM_NAME_LEN - 1) return 0;
    for (int i = 0; i < len; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') return 0;
    }
    return 1;
}

Team *team_find(const char *name) {
    for (int i = 0; i < MAX_TEAMS; i++) {
        if (teams[i].used && strcmp(teams[i].name, name) == 0) return &teams[i];
    }
    return NULL;
}

Team *team_find_by_member(const char *user) {
    for (int i = 0; i < MAX_TEAMS; i++) {
        if (!teams[i].used) continue;
        for (int j = 0; j < teams[i].member_count; j++) {
            if (strcmp(teams[i].members[j], user) == 0) return &teams[i];
        }
    }
    return NULL;
}

int team_create(const char *name, const char *captain) {
    if (!team_valid_name(name)) return TEAM_ERR_NAME;
    if (team_find(name)) return TEAM_ERR_EXISTS;
    if (team_find_by_member(captain)) return TEAM_ERR_IN_TEAM;

    for (int i = 0; i < MAX_TEAMS; i++) {
        if (!teams[i].used) {
            memset(&teams[i], 0, sizeof(Team));
            teams[i].used = 1;
            snprintf(teams[i].name, sizeof(teams[i].name), "%s", name);
            snprintf(teams[i].members[0], USER_NAME_LEN, "%s", captain);
            teams[i].member_count = 1;
            return TEAM_OK;
        }
    }
    return TEAM_ERR_NO_SLOT;
}

/* Ghi vao out: "<team>:<so_nguoi>/3 <team>:<so_nguoi>/3> ..." */
void team_list(char *out, int out_size) {
    int pos = 0;
    out[0] = '\0';
    for (int i = 0; i < MAX_TEAMS; i++) {
        if (!teams[i].used) continue;
        int w = snprintf(out + pos, out_size - pos, "%s%s:%d/%d",
                        pos > 0 ? " " : "", teams[i].name, 
                        teams[i].member_count, MAX_TEAM_SIZE);
        if (w < 0 || w >= out_size - pos) break; /* het cho trong buffer*/
        pos += w;
    }
}

/* Ghi vao out: "captain=<user> members=<u1>,<u2>,<u3>" */
void team_members_str(const Team *t, char *out, int out_size) {
    int pos = snprintf(out, out_size, "captain=%s members=", t->members[0]);
    for (int j = 0; j < t->member_count && pos < out_size; j++) {
        pos += snprintf(out + pos, out_size - pos, "%s%s", 
                        j > 0 ? "," : "", t->members[j]);
    }
}