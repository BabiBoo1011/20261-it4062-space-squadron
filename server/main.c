#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

#include "auth.h"
#include "framing.h"
#include "logger.h"
#include "protocol.h"
#include "team.h"

typedef struct {
    int fd;
    int logged_in;
    char username[32];
    LineBuffer lb;
} ClientSlot;

static ClientSlot clients[MAX_CLIENTS];

static int is_user_online(const char *user) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd >= 0 && clients[i].logged_in &&
            strcmp(clients[i].username, user) == 0) {
            return 1;
        }
    }
    return 0;
}

/* ===================== QUAN LY DOI ===================== */

static void send_team_err(int fd, int rc) {
    char resp[128];
    snprintf(resp, sizeof(resp), "ERR %s", team_strerror(rc));
    send_line(fd, resp);
}

/* Gui 1 dong toi nguoi choi dang online (bo qua neu nguoi do offline) */
static void send_to_user(const char *user, const char *msg) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd >= 0 && clients[i].logged_in &&
            strcmp(clients[i].username, user) == 0) {
            send_line(clients[i].fd, msg);
            return;
        }
    }
}

/* Gui 1 dong toi moi thanh vien trong doi, tru nguoi "except" (co the NULL) */
static void notify_team(const Team *t, const char *except, const char *msg) {
    for (int j = 0; j < t->member_count; j++) {
        if (except && strcmp(t->members[j], except) == 0) continue;
        send_to_user(t->members[j], msg);
    }
}

/* Nguoi choi roi doi hien tai (tu roi, dang xuat hoac mat ket noi ).
   Neu la doi truong thi giai tan doi. Tra ve 0 neu nguoi do khong o doi nao. */
static int leave_current_team(const char *user) {
    Team *t = team_find_by_member(user);
    if (!t) return 0;

    char msg[128];
    if (team_is_captain(t, user)) {
        snprintf(msg, sizeof(msg), "EVT TEAM_DELETED %s", t->name);
        notify_team(t, user, msg);
        snprintf(msg, sizeof(msg), "team %s disbanded (captain %s left)", t->name, user);
        log_event("TEAM", msg);
        team_disband(t);
    } else {
        team_remove_member(t, user);
        snprintf(msg, sizeof(msg), "EVT MEMBER_LEFT %s", user);
        notify_team(t, NULL, msg);
        snprintf(msg, sizeof(msg), "%s left team %s", user, t->name);
        log_event("TEAM", msg);
    }
    return 1;
}

/* TEAM_CREATE <team> */
static void cmd_team_create(ClientSlot *cli, int n, const char *team_name) {
    if (n < 2) {
        send_line(cli->fd, "ERR 400 usage: TEAM_CREATE <team>");
        return;
    }
    int rc = team_create(team_name, cli->username);
    if (rc != TEAM_OK) {
        send_team_err(cli->fd, rc);
        return;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "%s created team %s", cli->username, team_name);
    log_event("TEAM", msg);

    char resp[64];
    snprintf(resp, sizeof(resp), "OK TEAM_CREATE %s", team_name);
    send_line(cli->fd, resp);
}

/* TEAM_LIST */
static void cmd_team_list(ClientSlot *cli) {
    char list[BUF_SIZE - 32];
    team_list(list, sizeof(list));

    char resp[BUF_SIZE];
    snprintf(resp, sizeof(resp), "OK TEAM_LIST%s%s", list[0] ? " " : "", list);
    send_line(cli->fd, resp);
}

/*TEAM_MEMBERS <team> */
static void cmd_team_members(ClientSlot *cli, int n, const char *team_name) {
    if (n < 2) {
        send_line(cli->fd, "ERR 400 usage: TEAM_MEMBERS <team>");
        return;
    }
    Team *t = team_find(team_name);
    if (!t) {
        send_line(cli->fd, "ERR 404 team not found");
        return;
    }

    char info[256];
    team_members_str(t, info, sizeof(info));

    char resp[512];
    snprintf(resp, sizeof(resp), "OK TEAM_MEMBERS %s %s", t->name, info);
    send_line(cli->fd, resp);
}

/* TEAM_DELETE (chi doi truong) */
static void cmd_team_delete(ClientSlot *cli) {
    Team *t = team_find_by_member(cli->username);
    if (!t) {
        send_team_err(cli->fd, TEAM_ERR_NOT_IN_TEAM);
        return;
    }
    if (!team_is_captain(t, cli->username)) {
        send_team_err(cli->fd, TEAM_ERR_NOT_CAPTAIN);
        return;
    }
    leave_current_team(cli->username); /* doi truong roi doi = giai tan */
    send_line(cli->fd, "OK TEAM_DELETE");
}

/* TEAM_LEAVE */
static void cmd_team_leave(ClientSlot *cli) {
    if (!leave_current_team(cli->username)) {
        send_team_err(cli->fd, TEAM_ERR_NOT_IN_TEAM);
        return;
    }
    send_line(cli->fd, "OK TEAM_LEAVE");
}

/* TEAM_KICK <user> (chi doi truong) */
static void cmd_team_kick(ClientSlot *cli, int n, const char *target) {
    if (n < 2) {
        send_line(cli->fd, "ERR 400 usage: TEAM_KICK <user>");
        return;
    }
    Team *t = team_find_by_member(cli->username);
    if (!t) {
        send_team_err(cli->fd, TEAM_ERR_NOT_IN_TEAM);
        return;
    }
    if (!team_is_captain(t, cli->username)) {
        send_team_err(cli->fd, TEAM_ERR_NOT_CAPTAIN);
        return;
    }
    /* team_remove_member khong xoa duoc members[0], nen doi truong khong tu kick minh */
    if (team_remove_member(t, target) != TEAM_OK) {
        send_line(cli->fd, "ERR 404 member not found");
        return;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "EVT KICKED %s", t->name);
    send_to_user(target, msg);
    snprintf(msg, sizeof(msg), "EVT MEMBER_LEFT %s", target);
    notify_team(t, cli->username, msg);
    snprintf(msg, sizeof(msg), "%s kicked %s from team %s", cli->username, target, t->name);
    log_event("TEAM", msg);

    char resp[64];
    snprintf(resp, sizeof(resp), "OK TEAM_KICK %s", target);
    send_line(cli->fd, resp);
}

/* TEAM_JOIN <team> */
static void cmd_team_join(ClientSlot *cli, int n, const char *team_name) {
    if (n < 2) {
        send_line(cli->fd, "ERR 400 usage: TEAM_JOIN <team>");
        return;
    }
    if (team_find_by_member(cli->username)) {
        send_team_err(cli->fd, TEAM_ERR_IN_TEAM);
        return;
    }
    Team *t = team_find(team_name);
    if (!t) {
        send_line(cli->fd, "ERR 404 team not found");
        return;
    }
    if (t->member_count >= MAX_TEAM_SIZE) {
        send_team_err(cli->fd, TEAM_ERR_FULL);
        return;
    }
    int rc = req_add(REQ_JOIN, t->name, cli->username);
    if (rc != TEAM_OK) {
        send_team_err(cli->fd, rc);
        return;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "EVT JOIN_REQUEST %s", cli->username);
    send_to_user(t->members[0], msg);
    snprintf(msg, sizeof(msg), "%s requested to join team %s", cli->username, t->name);
    log_event("TEAM", msg);

    char resp[64];
    snprintf(resp, sizeof(resp), "OK TEAM_JOIN %s", t->name);
    send_line(cli->fd, resp);
}

/* TEAM_APPROVE <user> (chi doi truong) */
static void cmd_team_approve(ClientSlot *cli, int n, const char *target) {
    if (n < 2) {
        send_line(cli->fd, "ERR 400 usage: TEAM_APPROVE <user>");
        return;
    }
    Team *t = team_find_by_member(cli->username);
    if (!t) {
        send_team_err(cli->fd, TEAM_ERR_NOT_IN_TEAM);
        return;
    }
    if (!team_is_captain(t, cli->username)) {
        send_team_err(cli->fd, TEAM_ERR_NOT_CAPTAIN);
        return;
    }
    TeamRequest *r = req_find(REQ_JOIN, t->name, target);
    if (!r) {
        send_line(cli->fd, "ERR 404 join request not found");
        return;
    }
    int rc = team_add_member(t, target);
    if (rc != TEAM_OK) {
        if (rc == TEAM_ERR_IN_TEAM) req_remove(r);
        send_team_err(cli->fd, rc);
        return;
    }
    req_clear_user(target);

    char msg[128];
    snprintf(msg, sizeof(msg), "EVT JOIN_ACCEPTED %s", t->name);
    send_to_user(target, msg);
    snprintf(msg, sizeof(msg), "EVT MEMBER_JOINED %s", target);
    notify_team(t, target, msg);
    snprintf(msg, sizeof(msg), "%s approved %s into team %s", cli->username, target, t->name);
    log_event("TEAM", msg);

    char resp[64];
    snprintf(resp, sizeof(resp), "OK TEAM_APPROVE %s", target);
    send_line(cli->fd, resp);
}

/* TEAM_REJECT <user> (chi doi truong) */
static void cmd_team_reject(ClientSlot *cli, int n, const char *target) {
    if (n < 2) {
        send_line(cli->fd, "ERR 400 usage: TEAM_REJECT <user>");
        return;
    }
    Team *t = team_find_by_member(cli->username);
    if (!t) {
        send_team_err(cli->fd, TEAM_ERR_NOT_IN_TEAM);
        return;
    }
    if (!team_is_captain(t, cli->username)) {
        send_team_err(cli->fd, TEAM_ERR_NOT_CAPTAIN);
        return;
    }
    TeamRequest *r = req_find(REQ_JOIN, t->name, target);
    if (!r) {
        send_line(cli->fd, "ERR 404 join request not found");
        return;
    }
    req_remove(r);

    char msg[128];
    snprintf(msg, sizeof(msg), "EVT JOIN_REJECTED %s", t->name);
    send_to_user(target, msg);
    snprintf(msg, sizeof(msg), "%s rejected %s from team %s", cli->username, target, t->name);
    log_event("TEAM", msg);

    char resp[64];
    snprintf(resp, sizeof(resp), "OK TEAM_REJECT %s", target);
    send_line(cli->fd, resp);
}

/* ======================================================= */

static void handle_line(ClientSlot *cli, const char *line) {
    int fd = cli->fd;
    char cmd[32], arg1[64], arg2[64];
    int n = sscanf(line, "%31s %63s %63s", cmd, arg1, arg2);
    if (n <= 0) return;

    if (strcmp(cmd, CMD_PING) == 0) {
        send_line(fd, "PONG");
    } else if (strcmp(cmd, CMD_REGISTER) == 0 && n == 3) {
        if (auth_register(arg1, arg2) == 0) {
            log_event("AUTH", "New account registered");
            send_line(fd, "OK register success");
        } else {
            send_line(fd, "ERR register failed");
        }
    } else if (strcmp(cmd, CMD_LOGIN) == 0 && n == 3) {
        if (cli->logged_in) {
            send_line(fd, "ERR already logged in");
        } else if (is_user_online(arg1)) {
            send_line(fd, "ERR account already online");
        } else if (auth_verify(arg1, arg2)) {
            cli->logged_in = 1;
            snprintf(cli->username, sizeof(cli->username), "%.31s", arg1);
            log_event("AUTH", "User logged in");
            send_line(fd, "OK login success");
        } else {
            send_line(fd, "ERR invalid credentials");
        }
    } else if (strcmp(cmd, CMD_LOGOUT) == 0) {
        if (cli->logged_in) {
            leave_current_team(cli->username);
            cli->logged_in = 0;
            cli->username[0] = '\0';
            log_event("AUTH", "User logged out");
            send_line(fd, "OK logged out");
        } else {
            send_line(fd, "ERR not logged in");
        }
    } else if (!cli->logged_in) {
        log_event("ACCESS", "Denied unauthenticated command");
        send_line(fd, "ERR unauthorized: login required");
    } else if (strcmp(cmd, CMD_WHOAMI) == 0) {
        char resp[64];
        snprintf(resp, sizeof(resp), "OK user:%s", cli->username);
        send_line(fd, resp);
    } else if (strcmp(cmd, CMD_TEAM_CREATE) == 0) {
        cmd_team_create(cli, n, arg1);
    } else if (strcmp(cmd, CMD_TEAM_LIST) == 0) {
        cmd_team_list(cli);
    } else if (strcmp(cmd, CMD_TEAM_MEMBERS) == 0) {
        cmd_team_members(cli, n, arg1);
    } else if (strcmp(cmd, CMD_TEAM_DELETE) == 0) {
        cmd_team_delete(cli);
    } else if (strcmp(cmd, CMD_TEAM_LEAVE) == 0) {
        cmd_team_leave(cli);
    } else if (strcmp(cmd, CMD_TEAM_KICK) == 0) {
        cmd_team_kick(cli, n, arg1);
    } else if (strcmp(cmd, CMD_TEAM_JOIN) == 0) {
        cmd_team_join(cli, n, arg1);
    } else if (strcmp(cmd, CMD_TEAM_APPROVE) == 0) {
        cmd_team_approve(cli, n, arg1);
    } else if (strcmp(cmd, CMD_TEAM_REJECT) == 0) {
        cmd_team_reject(cli, n, arg1);
    } else {
        char resp[BUF_SIZE];
        snprintf(resp, sizeof(resp), "ERR unknown command: %s", line);
        send_line(fd, resp);
    }
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 16) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }

    auth_init();
    team_init();
    printf("Server listening on port %d...\n", port);
    log_event("SERVER", "Started listening");

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);
        int max_fd = listen_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd >= 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd) max_fd = clients[i].fd;
            }
        }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(listen_fd, &readfds)) {
            int cfd = accept(listen_fd, NULL, NULL);
            if (cfd >= 0) {
                int added = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd < 0) {
                        clients[i].fd = cfd;
                        clients[i].logged_in = 0;
                        clients[i].username[0] = '\0';
                        lb_init(&clients[i].lb);
                        log_event("CONN", "Client connected");
                        added = 1;
                        break;
                    }
                }
                if (!added) close(cfd);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int cfd = clients[i].fd;
            if (cfd < 0 || !FD_ISSET(cfd, &readfds)) continue;

            char tmp[BUF_SIZE];
            ssize_t n = recv(cfd, tmp, sizeof(tmp), 0);
            if (n <= 0) {
                if (clients[i].logged_in) leave_current_team(clients[i].username);
                close(cfd);
                clients[i].fd = -1;
                clients[i].logged_in = 0;
                clients[i].username[0] = '\0';
                log_event("CONN", "Client disconnected");
                continue;
            }

            if (lb_append(&clients[i].lb, tmp, (int)n) < 0) {
                if (clients[i].logged_in) leave_current_team(clients[i].username);
                close(cfd);
                clients[i].fd = -1;
                clients[i].logged_in = 0;
                continue;
            }

            char line[BUF_SIZE];
            while (lb_pop_line(&clients[i].lb, line, sizeof(line)) == 1) {
                handle_line(&clients[i], line);
            }
        }
    }

    close(listen_fd);
    return 0;
}
