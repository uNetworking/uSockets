/* Public interfaces for polls */

/* A fallthrough poll does not keep the loop running, it falls through */
struct us_poll *us_create_poll(struct us_loop *loop, int fallthrough, int ext_size);

/* Associate this poll with a socket descriptor and poll type */
void us_poll_init(struct us_poll *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type);

/* Start, change and stop polling for events */
void us_poll_start(struct us_poll *p, struct us_loop *loop, int events);
void us_poll_change(struct us_poll *p, struct us_loop *loop, int events);
void us_poll_stop(struct us_poll *p, struct us_loop *loop);

/* Returns the user data extension of this poll */
void *us_poll_ext(struct us_poll *p);

/* Get associated socket descriptor from a poll */
LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll *p);
