#ifndef CHC_CHANNEL
#define CHC_CHANNEL

typedef struct channel channel;

channel *make_channel(int len, int size);
void push_channel(channel *, void *data);
void *pop_channel(channel *);
void free_channel(channel *);

#endif
