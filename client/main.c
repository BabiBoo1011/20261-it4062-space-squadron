#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "framing.h"
#include "protocol.h"

int main(int argc, char *argv[]) {
    const char *ip = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &srv.sin_addr) <= 0) {
        fprintf(stderr, "IP khong hop le: %s\n", ip);
        close(sockfd);
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    printf("Da ket noi toi %s:%d (go lenh hoac QUIT de thoat)\n", ip, port);

    LineBuffer lb;
    lb_init(&lb);

    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        FD_SET(sockfd, &fds);
        int max_fd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

        if (select(max_fd + 1, &fds, NULL, NULL, NULL) < 0) break;

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            char input[BUF_SIZE];
            if (!fgets(input, sizeof(input), stdin)) break;
            input[strcspn(input, "\r\n")] = '\0';
            if (strcmp(input, "QUIT") == 0) break;
            if (strlen(input) > 0 && send_line(sockfd, input) < 0) break;
        }

        if (FD_ISSET(sockfd, &fds)) {
            char tmp[BUF_SIZE];
            ssize_t n = recv(sockfd, tmp, sizeof(tmp), 0);
            if (n <= 0) {
                printf("Mat ket noi server.\n");
                break;
            }
            if (lb_append(&lb, tmp, (int)n) < 0) break;
            char line[BUF_SIZE];
            while (lb_pop_line(&lb, line, sizeof(line)) == 1) {
                printf("[Server] %s\n", line);
            }
        }
    }

    close(sockfd);
    return 0;
}
