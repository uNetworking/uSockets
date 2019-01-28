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

/* Public interfaces for loops */

/* Returns a new event loop with user data extension */
WIN32_EXPORT struct us_loop *us_create_loop(int default_hint, void (*wakeup_cb)(struct us_loop *loop), void (*pre_cb)(struct us_loop *loop), void (*post_cb)(struct us_loop *loop), unsigned int ext_size);

/* */
WIN32_EXPORT void us_loop_free(struct us_loop *loop);

/* Returns the loop user data extension */
WIN32_EXPORT void *us_loop_ext(struct us_loop *loop);

/* Blocks the calling thread and drives the event loop until no more non-fallthrough polls are scheduled */
WIN32_EXPORT void us_loop_run(struct us_loop *loop);

/* Signals the loop from any thread to wake up and execute its wakeup handler from the loop's own running thread.
 * This is the only fully thread-safe function and serves as the basis for thread safety */
WIN32_EXPORT void us_wakeup_loop(struct us_loop *loop);

/* Hook up timers in existing loop */
WIN32_EXPORT void us_loop_integrate(struct us_loop *loop);

/* Returns the loop iteration number */
WIN32_EXPORT long long us_loop_iteration_number(struct us_loop *loop);
