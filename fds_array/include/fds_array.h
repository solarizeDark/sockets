#ifndef __FDS_ARRAY__
#define __FDS_ARRAY__

extern int CAPACITY;

void fds_add(struct pollfd *fds_start, struct pollfd **pfdsa, int new_fd);

#endif
