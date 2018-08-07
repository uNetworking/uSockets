/* Public interfaces for loops */

/* Returns a new event loop with user data extension */
WIN32_EXPORT struct us_loop *us_create_loop(int default_hint, void (*wakeup_cb)(struct us_loop *loop), void (*pre_cb)(struct us_loop *loop), void (*post_cb)(struct us_loop *loop), int ext_size);

/* */
WIN32_EXPORT void us_loop_free(struct us_loop *loop);

/* Returns the loop user data extension */
WIN32_EXPORT void *us_loop_ext(struct us_loop *loop);

/* Blocks the calling thread and drives the event loop until no more non-fallthrough polls are scheduled */
WIN32_EXPORT void us_loop_run(struct us_loop *loop);

/* Signals the loop from any thread to wake up and execute its wakeup handler from the loop's own running thread.
 * This is the only fully thread-safe function and serves as the basis for thread safety */
WIN32_EXPORT void us_wakeup_loop(struct us_loop *loop);
