#ifndef FRAMING_H
#define FRAMING_H

#include "protocol.h"

typedef struct {
    char buf[BUF_SIZE * 2];
    int len;
} LineBuffer;

void lb_init(LineBuffer *lb);
int lb_append(LineBuffer *lb, const char *data, int n);
int lb_pop_line(LineBuffer *lb, char *out, int max_len);
int send_line(int sockfd, const char *msg);

#endif
