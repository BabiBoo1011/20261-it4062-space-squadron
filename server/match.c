#include "match.h"
#include <stdio.h>
#include <string.h>

static Match matches[MAX_MATCHES];
static int next_match_id = 1;

void match_init(void) {
    memset(matches, 0, sizeof(matches));
    next_match_id = 1;
}

/* Dua tau ve trang thai ban dau theo de bai:
   HP = 1000, Armor = 0, coin = 0, 1 phao 30mm voi 50 vien, khong laze / ten lua / giap */
void ship_reset(Ship *s, const char *user, const char *team) {
    memset(s, 0, sizeof(Ship));
    snprintf(s->user, USER_NAME_LEN, "%s", user);
    snprintf(s->team, TEAM_NAME_LEN, "%s", team);
    s->hp = INIT_HP;
    s->ammo = INIT_AMMO_30MM;
    s->alive = 1;
}

static void add_team_ships(Match *m, const Team *t) {
    for (int j = 0; j < t->member_count && m->ship_count < MAX_SHIPS; j++) {
        ship_reset(&m->ships[m->ship_count], t->members[j], t->name);
        m->ship_count++;
    }
}

/* Tao tran moi giua 2 doi. Tra ve NULL neu server het cho */
Match *match_create(const Team *a, const Team *b) {
    for (int i = 0; i < MAX_MATCHES; i++) {
        if (!matches[i].used) {
            Match *m = &matches[i];
            memset(m, 0, sizeof(Match));
            m->used = 1;
            m->id = next_match_id++;
            snprintf(m->team_a, TEAM_NAME_LEN, "%s", a->name);
            snprintf(m->team_b, TEAM_NAME_LEN, "%s", b->name);
            add_team_ships(m, a);
            add_team_ships(m, b);
            m->started_at = time(NULL);
            return m;
        }
    }
    return NULL;
}

Ship *match_find_ship(Match *m, const char *user) {
    for (int k = 0; k < m->ship_count; k++) {
        if (strcmp(m->ships[k].user, user) == 0) return &m->ships[k];
    }
    return NULL;
}

Match *match_find_by_user(const char *user) {
    for (int i = 0; i < MAX_MATCHES; i++) {
        if (matches[i].used && match_find_ship(&matches[i], user)) return &matches[i];
    }
    return NULL;
}

Match *match_find_by_team(const char *team) {
    for (int i = 0; i < MAX_MATCHES; i++) {
        if (matches[i].used &&
            (strcmp(matches[i].team_a, team) == 0 || strcmp(matches[i].team_b, team) == 0)) {
            return &matches[i];
        }
    }
    return NULL;
}