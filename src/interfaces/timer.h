/*
 * Authored by Alex Hultman, 2018-2019.
 * Intellectual property of third-party.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Public interfaces for timers */

/* Create a new high precision, low performance timer. May fail and return null */
WIN32_EXPORT struct us_timer_t *us_create_timer(struct us_loop_t *loop, int fallthrough, unsigned int ext_size);

/* Returns user data extension for this timer */
WIN32_EXPORT void *us_timer_ext(struct us_timer_t *timer);

/* */
WIN32_EXPORT void us_timer_close(struct us_timer_t *timer);

/* Arm a timer with a delay from now and eventually a repeat delay.
 * Specify 0 as repeat delay to disable repeating. Specify both 0 to disarm. */
WIN32_EXPORT void us_timer_set(struct us_timer_t *timer, void (*cb)(struct us_timer_t *t), int ms, int repeat_ms);

/* Returns the loop for this timer */
WIN32_EXPORT struct us_loop_t *us_timer_loop(struct us_timer_t *t);
