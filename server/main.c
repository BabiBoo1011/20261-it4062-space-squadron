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

typedef struct {
    int fd;
    LineBuffer lb;
} ClientSlot;

static ClientSlot clients[MAX_CLIENTS];

static void handle_line(int fd, const char *line) {
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
                close(cfd);
                clients[i].fd = -1;
                log_event("CONN", "Client disconnected");
                continue;
            }

            if (lb_append(&clients[i].lb, tmp, (int)n) < 0) {
                close(cfd);
                clients[i].fd = -1;
                continue;
            }

            char line[BUF_SIZE];
            while (lb_pop_line(&clients[i].lb, line, sizeof(line)) == 1) {
                handle_line(cfd, line);
            }
        }
    }

    close(listen_fd);
    return 0;
}
