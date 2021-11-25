#ifndef __COMPLEX_H__
#define __COMPLEX_H__

bool one_chunk_message(char *buf, char *huge, int rc);
bool multi_chunk_message(char *buf, char *huge, int rc, bool state, int current_i);
bool get_message_complex(int *fd);

#endif
