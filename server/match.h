#ifndef MATCH_H
#define MATCH_H

#include <time.h>
#include "protocol.h"
#include "team.h"

#define MAX_MATCHES 16
#define MAX_SHIPS   (MAX_TEAM_SIZE * 2) /* 2 doi, moi doi toi da 3 tau */

/* Trang thai 1 phi thuyen trong tran */
typedef struct {
    char user[USER_NAME_LEN];
    char team[TEAM_NAME_LEN];
    int hp;
    int armor[MAX_ARMOR_LAYERS]; /* Armor con lai cua tung lop giap, 0 = trong */
    int coin;
    int ammo;        /* so vien dan phao 30mm */
    int lasers;      /* so khau phao laze */
    int laser_power; /* so lan ban laze con lai */
    int missiles;    /* so qua ten lua */
    int alive;
} Ship;

typedef struct {
    int used;
    int id;
    char team_a[TEAM_NAME_LEN];
    char team_b[TEAM_NAME_LEN];
    Ship ships[MAX_SHIPS];
    int ship_count;
    time_t started_at;
} Match;

void match_init(void);
void ship_reset(Ship *s, const char *user, const char *team);
Match *match_create(const Team *a, const Team *b);
Match *match_find_by_user(const char *user);
Match *match_find_by_team(const char *team);
Ship *match_find_ship(Match *m, const char *user);

#endif