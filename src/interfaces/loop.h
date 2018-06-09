/* Public interfaces for loops */

/* Returns a new event loop with user data extension */
struct us_loop *us_create_loop(void (*wakeup_cb)(struct us_loop *loop), void (*pre_cb)(struct us_loop *loop), void (*post_cb)(struct us_loop *loop), int ext_size);

/* Returns the loop user data extension */
void *us_loop_ext(struct us_loop *loop);

/* Blocks the calling thread and drives the event loop until no more non-fallthrough polls are scheduled */
void us_loop_run(struct us_loop *loop);

/* Signals the loop from any thread to wake up and execute its wakeup handler from the loop's own running thread.
 * This is the only fully thread-safe function and serves as the basis for thread safety */
void us_wakeup_loop(struct us_loop *loop);
