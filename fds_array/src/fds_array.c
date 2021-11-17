#include <poll.h>
#include <stdlib.h>
#include "../include/fds_array.h"

int CAPACITY = 10;

void fds_add(struct pollfd *fds_start, struct pollfd **pfdsa, int new_fd) {

    if (*pfdsa == fds_start) {
        CAPACITY *= 2;
        fds_start = (struct pollfd *) realloc(fds_start, CAPACITY * sizeof(struct pollfd));
        *pfdsa = fds_start + CAPACITY / 2;
    }

    (*pfdsa)->fd     = new_fd;
    (*pfdsa)->events = POLLIN;
    (*pfdsa)++;

}