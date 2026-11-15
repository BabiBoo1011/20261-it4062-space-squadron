#include "framing.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void lb_init(LineBuffer *lb) {
    lb->len = 0;
    lb->buf[0] = '\0';
}

int lb_append(LineBuffer *lb, const char *data, int n) {
    if (n <= 0) return 0;
    if (lb->len + n >= (int)sizeof(lb->buf)) return -1;
    memcpy(lb->buf + lb->len, data, n);
    lb->len += n;
    lb->buf[lb->len] = '\0';
    return 0;
}

int lb_pop_line(LineBuffer *lb, char *out, int max_len) {
    char *nl = memchr(lb->buf, '\n', lb->len);
    if (!nl) return 0;

    int line_len = (int)(nl - lb->buf);
    int copy_len = (line_len < max_len - 1) ? line_len : (max_len - 1);
    memcpy(out, lb->buf, copy_len);
    out[copy_len] = '\0';

    /* Bo \r neu client gui CRLF */
    if (copy_len > 0 && out[copy_len - 1] == '\r') {
        out[copy_len - 1] = '\0';
    }

    int remain = lb->len - (line_len + 1);
    if (remain > 0) {
        memmove(lb->buf, nl + 1, remain);
    }
    lb->len = remain;
    lb->buf[lb->len] = '\0';
    return 1;
}

int send_line(int sockfd, const char *msg) {
    char out[BUF_SIZE + 2];
    int n = snprintf(out, sizeof(out), "%s\n", msg);
    if (n < 0 || n >= (int)sizeof(out)) return -1;

    int sent = 0;
    while (sent < n) {
        ssize_t w = send(sockfd, out + sent, n - sent, 0);
        if (w <= 0) return -1;
        sent += (int)w;
    }
    return 0;
}
