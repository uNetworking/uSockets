/*
 * Authored by Alex Hultman, 2018-2021.
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

#ifdef LIBUS_USE_ASIO

#include <boost/asio.hpp>
#include <iostream>
#include <mutex>
#include <memory>

int polls; // temporary solution keeping track of outstanding work

extern "C" {

#include "libusockets.h"
#include "internal/internal.h"
#include <stdlib.h>

// define a timer internally as something that inherits from callback_t
// us_timer_t is convertible to this one
struct boost_timer : us_internal_callback_t {
    boost::asio::deadline_timer timer;

    boost_timer(boost::asio::io_context *io) : timer(*io) {
        std::cout << "constructing timer" << std::endl;
    }
};

struct boost_block_poll_t : boost::asio::posix::descriptor {

    boost_block_poll_t(boost::asio::io_context *io, us_poll_t *p) : boost::asio::posix::descriptor(*io), p(p) {
        isValid.reset(this, [](boost_block_poll_t *t) {});
    }

    std::shared_ptr<boost_block_poll_t> isValid;

    unsigned char nr = 0;
    struct us_poll_t *p;
};

// poll
void us_poll_init(struct us_poll_t *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type) {
    struct boost_block_poll_t *boost_block = (struct boost_block_poll_t *) p->boost_block;
    boost_block->assign(fd);
    p->poll_type = poll_type;
    p->events = 0;
}

void us_poll_free(struct us_poll_t *p, struct us_loop_t *loop) {
    struct boost_block_poll_t *boost_block = (struct boost_block_poll_t *) p->boost_block;

    //boost_block->release();
    delete boost_block; // because of post mortem calls we need to have a weak_ptr to this block
    free(p);
}

void poll_for_error(struct boost_block_poll_t *boost_block) {
    polls++;
    boost_block->async_wait(boost::asio::posix::descriptor::wait_type::wait_error, [nr = boost_block->nr, weakBoostBlock = std::weak_ptr<boost_block_poll_t>(boost_block->isValid)](boost::system::error_code ec) {
        polls--;
        
        if (ec != boost::asio::error::operation_aborted) {

            // post mortem check
            struct boost_block_poll_t *boost_block;
            if (auto observe = weakBoostBlock.lock()) {
                boost_block = observe.get();
            } else {
                std::cout << "poll for error post mortem" << std::endl;
                return;
            }

            // get boost_block from weakptr
            if (nr != boost_block->nr) {
                return;
            }

            poll_for_error(boost_block); // ska man verkligen polla for error igen
            us_internal_dispatch_ready_poll(boost_block->p, 1, LIBUS_SOCKET_READABLE | LIBUS_SOCKET_WRITABLE);
        }
    });
}

void poll_for_read(struct boost_block_poll_t *boost_block) {
    polls++;
    boost_block->async_wait(boost::asio::posix::descriptor::wait_type::wait_read, [nr = boost_block->nr, weakBoostBlock = std::weak_ptr<boost_block_poll_t>(boost_block->isValid)](boost::system::error_code ec) {
        polls--;

        if (ec != boost::asio::error::operation_aborted) {

            // post mortem check
            struct boost_block_poll_t *boost_block;
            if (auto observe = weakBoostBlock.lock()) {
                boost_block = observe.get();
            } else {
                return;
            }

            // get boost_block from weakptr
            if (nr != boost_block->nr) {
                return;
            }

            poll_for_read(boost_block);
            us_internal_dispatch_ready_poll(boost_block->p, ec ? -1 : 0, LIBUS_SOCKET_READABLE);
        }
    });
}

void poll_for_write(struct boost_block_poll_t *boost_block) {
    polls++;
    boost_block->async_wait(boost::asio::posix::descriptor::wait_type::wait_write, [nr = boost_block->nr, weakBoostBlock = std::weak_ptr<boost_block_poll_t>(boost_block->isValid)](boost::system::error_code ec) {
        polls--;
        
        if (ec != boost::asio::error::operation_aborted) {

            // post mortem check
            struct boost_block_poll_t *boost_block;
            if (auto observe = weakBoostBlock.lock()) {
                boost_block = observe.get();
            } else {
                return;
            }

            // get boost_block from weakptr
            if (nr != boost_block->nr) {
                return;
            }
            poll_for_write(boost_block);
            us_internal_dispatch_ready_poll(boost_block->p, ec ? -1 : 0, LIBUS_SOCKET_WRITABLE);
        }
    });
}

void us_poll_start(struct us_poll_t *p, struct us_loop_t *loop, int events) {
    struct boost_block_poll_t *boost_block = (struct boost_block_poll_t *) p->boost_block;

    p->events = events;
    poll_for_error(boost_block);

    if (events & LIBUS_SOCKET_READABLE) {
        poll_for_read(boost_block);
    }

    if (events & LIBUS_SOCKET_WRITABLE) {
        poll_for_write(boost_block);
    }
}

void us_poll_change(struct us_poll_t *p, struct us_loop_t *loop, int events) {
    struct boost_block_poll_t *boost_block = (struct boost_block_poll_t *) p->boost_block;

    boost_block->nr++;
    boost_block->cancel();

    us_poll_start(p, loop, events);
}

void us_poll_stop(struct us_poll_t *p, struct us_loop_t *loop) {
    struct boost_block_poll_t *boost_block = (struct boost_block_poll_t *) p->boost_block;

    boost_block->nr++;
    boost_block->cancel();
}

int us_poll_events(struct us_poll_t *p) {
    return p->events;
}

unsigned int us_internal_accept_poll_event(struct us_poll_t *p) {
    return 0;
}

int us_internal_poll_type(struct us_poll_t *p) {
    return p->poll_type;
}

void us_internal_poll_set_type(struct us_poll_t *p, int poll_type) {
    p->poll_type = poll_type;
}

LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll_t *p) {
    struct boost_block_poll_t *boost_block = (struct boost_block_poll_t *) p->boost_block;

    return boost_block->native_handle();
}

// if we get an io_context ptr as hint, we use it
// otherwise we create a new one for only us
struct us_loop_t *us_create_loop(void *hint, void (*wakeup_cb)(struct us_loop_t *loop), void (*pre_cb)(struct us_loop_t *loop), void (*post_cb)(struct us_loop_t *loop), unsigned int ext_size) {
    std::cout << "creating loop" << std::endl;

    struct us_loop_t *loop = (struct us_loop_t *) malloc(sizeof(struct us_loop_t) + ext_size);

    loop->io = hint ? hint : new boost::asio::io_context();
    loop->is_default = hint != 0;

    // here we create two unreffed handles - timer and async
    us_internal_loop_data_init(loop, wakeup_cb, pre_cb, post_cb);

    // if we do not own this loop, we need to integrate and set up timer
    if (hint) {
        us_loop_integrate(loop);
    }

    return loop;
}

// based on if this was default loop or not
void us_loop_free(struct us_loop_t *loop) {

}

void us_loop_run(struct us_loop_t *loop) {
    //std::cout << "us_loop_run" << std::endl;

    us_loop_integrate(loop);

    //((boost::asio::io_context *) loop->io)->run();
    //return;

    // this kind of running produces identical syscalls as just run, so it is fine
    while (true) {
        us_internal_loop_pre(loop);
        size_t num = ((boost::asio::io_context *) loop->io)->run_one();
        if (!num) {
            break;
        }
        /*
        for (int i = 0; true; i++) {
            num = ((boost::asio::io_context *) loop->io)->poll_one();
            if (!num || i == 999) {
                break;
            }
        }*/
        us_internal_loop_post(loop);

        // here we check if our timer is the only poll in the event loop - how?

    }
}

struct us_poll_t *us_create_poll(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    struct us_poll_t *p = (struct us_poll_t *) malloc(sizeof(struct us_poll_t) + ext_size);
    p->boost_block = new boost_block_poll_t( (boost::asio::io_context *)loop->io, p);

    return p;
}

/* If we update our block position we have to updarte the uv_poll data to point to us */
struct us_poll_t *us_poll_resize(struct us_poll_t *p, struct us_loop_t *loop, unsigned int ext_size) {
    p = (struct us_poll_t *) realloc(p, sizeof(struct us_poll_t) + ext_size);

    // captures must never capture p directly, only boost_block and derive p from there
    ((struct boost_block_poll_t *) p->boost_block)->p = p;

    return p;
}

// timer
struct us_timer_t *us_create_timer(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    //std::cout << "creating timer" << std::endl;

    struct boost_timer *cb = (struct boost_timer *) malloc(sizeof(struct boost_timer) + ext_size);

    cb->loop = loop;
    cb->cb_expects_the_loop = 0;
    cb->p.poll_type = POLL_TYPE_CALLBACK; // this is missing from libuv flow

    // inplace construct the timer on this callback_t
    new (cb) boost_timer((boost::asio::io_context *)loop->io);

    if (fallthrough) {
        //uv_unref((uv_handle_t *) uv_timer);
    }

    return (struct us_timer_t *) cb;
}

void *us_timer_ext(struct us_timer_t *timer) {
    return ((struct boost_timer *) timer) + 1;
}

void us_timer_close(struct us_timer_t *t) {

}

void us_timer_set(struct us_timer_t *t, void (*cb)(struct us_timer_t *t), int ms, int repeat_ms) {
    struct boost_timer *b_timer = (struct boost_timer *) t;

    b_timer->cb = (void(*)(struct us_internal_callback_t *)) cb;

    b_timer->timer.expires_from_now(boost::posix_time::milliseconds(ms)); // timer.expiry() + 
    b_timer->timer.async_wait([t, cb, ms, repeat_ms](const boost::system::error_code &ec) {
        if (ec != boost::asio::error::operation_aborted) {
            if (repeat_ms) {

                if (!polls) {
                    //std::cout << "we own no polls" << std::endl;
                    return;
                }

                us_timer_set(t, cb, ms, repeat_ms);
            }
            us_internal_dispatch_ready_poll((struct us_poll_t *)t, 0, 0);
        }
    });

}

struct us_loop_t *us_timer_loop(struct us_timer_t *t) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) t;

    return internal_cb->loop;
}

// async (internal only) probably map to io_context::post
struct boost_async : us_internal_callback_t {
    std::mutex m;
};

struct us_internal_async *us_internal_create_async(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    struct boost_async *cb = (struct boost_async *) malloc(sizeof(struct boost_async) + ext_size);

    // inplace construct
    new (cb) boost_async();

    // these properties are accessed from another thread when wakeup
    cb->m.lock();
    cb->loop = loop;
    cb->cb_expects_the_loop = 0;
    cb->p.poll_type = POLL_TYPE_CALLBACK; // this is missing from libuv flow
    cb->m.unlock();

    if (fallthrough) {
        //uv_unref((uv_handle_t *) uv_timer);
    }

    return (struct us_internal_async *) cb;
}

void us_internal_async_close(struct us_internal_async *a) {
    ((boost_async *) a)->~boost_async();
    free(a);
}

void us_internal_async_set(struct us_internal_async *a, void (*cb)(struct us_internal_async *)) {
    struct boost_async *internal_cb = (struct boost_async *) a;

    //internal_cb->m.lock();
    internal_cb->cb = (void(*)(struct us_internal_callback_t *)) cb;
    //internal_cb->m.unlock();
}

void us_internal_async_wakeup(struct us_internal_async *a) {
    //std::cout << "us_internal_async_wakeup not implemented" << std::endl;

    struct boost_async *cb = (struct boost_async *) a;

    // snarare loopens mutex som locks

    cb->m.lock();
    ((boost::asio::io_context *) cb->loop->io)->post([cb]() {

        // check if our weak_ptr is expired from the close/free above

        //std::cout << "calling async cb now" << std::endl;

        us_internal_dispatch_ready_poll((struct us_poll_t *) cb, 0, 0);

        cb->m.lock();
        //cb->cb(cb);// = (void(*)(struct us_internal_callback_t *)) cb;
        cb->m.unlock();
    });
    cb->m.unlock();
}

}

#endif
