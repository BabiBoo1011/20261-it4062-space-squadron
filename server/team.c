#include "team.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static Team teams[MAX_TEAMS];
static TeamRequest requests[MAX_TEAM_REQUESTS];
static Challenge challenges[MAX_CHALLENGES];

void team_init(void) {
    memset(teams, 0, sizeof(teams));
    memset(requests, 0, sizeof(requests));
    memset(challenges, 0, sizeof(challenges));
}

/* Doi ma ket qua thanh "<ma_loi> <mo_ta>" de gui kem sau "ERR " */
const char *team_strerror(int rc) {
    switch (rc) {
    case TEAM_ERR_NAME:        return "400 invalid team name";
    case TEAM_ERR_EXISTS:      return "409 team already exists";
    case TEAM_ERR_IN_TEAM:     return "412 already in a team";
    case TEAM_ERR_NO_SLOT:     return "500 server is full";
    case TEAM_ERR_NOT_FOUND:   return "404 not found";
    case TEAM_ERR_NOT_CAPTAIN: return "403 only team captain can do this";
    case TEAM_ERR_NOT_IN_TEAM: return "412 you are not in a team";
    case TEAM_ERR_FULL:        return "421 team is full";
    case TEAM_ERR_DUP_REQ:     return "409 request already pending";
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

int team_is_captain(const Team *t, const char *user) {
    return t->member_count > 0 && strcmp(t->members[0], user) == 0;
}

int team_add_member(Team *t, const char *user) {
    if (t->member_count >= MAX_TEAM_SIZE) return TEAM_ERR_FULL;
    if (team_find_by_member(user)) return TEAM_ERR_IN_TEAM;
    snprintf(t->members[t->member_count], USER_NAME_LEN, "%s", user);
    t->member_count++;
    return TEAM_OK;
}

/* Xoa 1 thanh vien (khong phai doi truong) khoi doi, don cac thanh vien phia sau len */
int team_remove_member(Team *t, const char *user) {
    for (int j = 1; j < t->member_count; j++) {
        if (strcmp(t->members[j], user) == 0) {
            for (int k = j; k < t->member_count - 1; k++) {
                memcpy(t->members[k], t->members[k + 1], USER_NAME_LEN);
            }
            t->member_count--;
            t->members[t->member_count][0] = '\0';
            return TEAM_OK;
        }
    }
    return TEAM_ERR_NOT_FOUND;
}

void team_disband(Team *t) {
    req_clear_team(t->name);
    challenge_clear_team(t->name);
    memset(t, 0, sizeof(Team));
}

TeamRequest *req_find(int type, const char *team, const char *user) {
    for (int i = 0; i < MAX_TEAM_REQUESTS; i++) {
        if (requests[i].used && requests[i].type == type &&
            strcmp(requests[i].team, team) == 0 &&
            strcmp(requests[i].user, user) == 0) {
            return &requests[i];
        }
    }
    return NULL;
}

int req_add(int type, const char *team, const char *user) {
    if (req_find(type, team, user)) return TEAM_ERR_DUP_REQ;
    for (int i = 0; i < MAX_TEAM_REQUESTS; i++) {
        if (!requests[i].used) {
            requests[i].used = 1;
            requests[i].type = type;
            snprintf(requests[i].team, TEAM_NAME_LEN, "%s", team);
            snprintf(requests[i].user, USER_NAME_LEN, "%s", user);
            requests[i].created_at = time(NULL);
            return TEAM_OK;
        }
    }
    return TEAM_ERR_NO_SLOT;
}

void req_remove(TeamRequest *r) {
    if (r) memset(r, 0, sizeof(TeamRequest));
}

void req_clear_team(const char *team) {
    for (int i = 0; i < MAX_TEAM_REQUESTS; i++) {
        if (requests[i].used && strcmp(requests[i].team, team) == 0) {
            req_remove(&requests[i]);
        }
    }
}

void req_clear_user(const char *user) {
    for (int i = 0; i < MAX_TEAM_REQUESTS; i++) {
        if (requests[i].used && strcmp(requests[i].user, user) == 0) {
            req_remove(&requests[i]);
        }
    }
}

TeamRequest *req_at(int idx) {
    if (idx < 0 || idx >= MAX_TEAM_REQUESTS) return NULL;
    return requests[idx].used ? &requests[idx] : NULL;
}

Challenge *challenge_find(const char *from_team, const char *to_team) {
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (challenges[i].used &&
            strcmp(challenges[i].from_team, from_team) == 0 &&
            strcmp(challenges[i].to_team, to_team) == 0) {
            return &challenges[i];
        }
    }
    return NULL;
}

int challenge_add(const char *from_team, const char *to_team) {
    if (challenge_find(from_team, to_team) || challenge_find(to_team, from_team)) {
        return TEAM_ERR_DUP_REQ;
    }
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (!challenges[i].used) {
            challenges[i].used = 1;
            snprintf(challenges[i].from_team, TEAM_NAME_LEN, "%s", from_team);
            snprintf(challenges[i].to_team, TEAM_NAME_LEN, "%s", to_team);
            challenges[i].accepted = 0;
            challenges[i].created_at = time(NULL);
            return TEAM_OK;
        }
    }
    return TEAM_ERR_NO_SLOT;
}

void challenge_remove(Challenge *c) {
    if (c) memset(c, 0, sizeof(Challenge));
}

void challenge_clear_team(const char *team) {
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (challenges[i].used &&
            (strcmp(challenges[i].from_team, team) == 0 ||
             strcmp(challenges[i].to_team, team) == 0)) {
            challenge_remove(&challenges[i]);
        }
    }
}

Challenge *challenge_at(int idx) {
    if (idx < 0 || idx >= MAX_CHALLENGES) return NULL;
    return challenges[idx].used ? &challenges[idx] : NULL;
}