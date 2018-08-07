/* Public interfaces for timers */

/* Create a new high precision, low performance timer. May fail and return null */
WIN32_EXPORT struct us_timer *us_create_timer(struct us_loop *loop, int fallthrough, int ext_size);

/* */
WIN32_EXPORT void us_timer_close(struct us_timer *timer);

/* Arm a timer with a delay from now and eventually a repeat delay.
 * Specify 0 as repeat delay to disable repeating. Specify both 0 to disarm. */
WIN32_EXPORT void us_timer_set(struct us_timer *timer, void (*cb)(struct us_timer *t), int ms, int repeat_ms);

/* Returns the loop for this timer */
WIN32_EXPORT struct us_loop *us_timer_loop(struct us_timer *t);
