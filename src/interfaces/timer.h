/* Public interfaces for timers */

/* Create a new high precision, low performance timer. May fail and return null */
struct us_timer *us_create_timer(struct us_loop *loop, int fallthrough, int ext_size);

/* Arm a timer with a delay from now and eventually a repeat delay.
 * Specify 0 as repeat delay to disable repeating. Specify both 0 to disarm. */
void us_timer_set(struct us_timer *timer, void (*cb)(struct us_timer *t), int ms, int repeat_ms);

/* Returns the loop for this timer */
struct us_loop *us_timer_loop(struct us_timer *t);
