//#[cfg(any(LIBUS_USE_LIBUV, LIBUS_USE_ASIO)))]
//pub mod epoll_kqueue;
//#[cfg(not(any(LIBUS_USE_LIBUV, LIBUS_USE_ASIO)))]

#[cfg(not(any(LIBUS_USE_LIBUV, LIBUS_USE_ASIO, LIBUS_USE_GCD)))]
pub mod epoll_kqueue;
#[cfg(not(any(LIBUS_USE_LIBUV, LIBUS_USE_ASIO, LIBUS_USE_GCD)))]
use epoll_kqueue::*;

#[cfg(LIBUS_USE_LIBUV)]
pub struct us_loop_t {

}

extern "C" {
    fn us_socket_is_shut_down(ssl: i32, s: u64) -> i32;
    fn bsd_socket_flush(fd: i32);
    fn bsd_shutdown_socket_read(fd: i32);
    fn us_poll_fd(p: u64) -> i32;
    fn us_internal_create_async(loop_data: *mut us_internal_loop_data_t, fallthrough: i32, ext_size: i32) -> *mut us_socket_context_t;
    fn us_internal_async_set(_async: *mut us_socket_context_t, cb: *mut us_socket_context_t);
    fn us_create_timer(loop_data: *mut us_internal_loop_data_t, fallthrough: i32, ext_size: i32) -> *mut us_socket_context_t;
    
    fn us_internal_ssl_socket_context_on_data(context: *mut us_socket_context_t, on_data: *mut us_socket_context_t);
    fn us_internal_ssl_socket_context_on_open(context: *mut us_socket_context_t, on_open: *mut us_socket_context_t);
    fn us_internal_ssl_socket_context_on_close(context: *mut us_socket_context_t, on_close: *mut us_socket_context_t);
    fn us_internal_ssl_socket_context_on_connect_error(context: *mut us_socket_context_t, on_connect_error: *mut us_socket_context_t);
    fn us_internal_ssl_socket_context_on_end(context: *mut us_socket_context_t, on_end: *mut us_socket_context_t);
    fn us_internal_ssl_socket_context_on_writable(context: *mut us_socket_context_t, on_writable: *mut us_socket_context_t);
    fn us_internal_ssl_socket_context_on_socket_timeout(context: *mut us_socket_context_t, on_socket_timeout: *mut us_socket_context_t);
    fn us_internal_ssl_socket_context_on_socket_long_timeout(context: *mut us_socket_context_t, on_socket_long_timeout: *mut us_socket_context_t);

    fn recv(fd: i32, buf: u64, length: i32, flags: i32) -> i32;
    fn closesocket(fd: i32); // only on windows
    fn close(fd: i32); // not on windows?
    fn malloc(size: usize) -> *mut us_socket_context_t;
    fn free(m: *mut us_socket_context_t);


    // for kqueue temporary
    fn us_timer_close(timer: *mut us_socket_context_t);
    fn us_internal_async_close(timer: *mut us_socket_context_t);
}

#[repr(C)]
pub struct us_socket_context_options_t {
    key_file_name: *const char,
    cert_file_name: *const char,
    passphrase: *const char,
    dh_params_file_name: *const char,
    ca_file_name: *const char,
    ssl_ciphers: *const char,
    ssl_prefer_low_memory_usage: i32 /* Todo: rename to prefer_low_memory_usage and apply for TCP as well */
}

#[repr(C)]
pub struct us_socket_context_t {
    _loop: *mut us_internal_loop_data_t,
    global_tick: u32,
    timestamp: u8,
    long_timestamp: u8,

    head_sockets: u64,
    head_listen_sockets: u64,
    iterator: u64,
    prev: *mut us_socket_context_t,
    next: *mut us_socket_context_t,

    /*struct us_socket_t *(*on_open)(struct us_socket_t *, int is_client, char *ip, int ip_length);
    struct us_socket_t *(*on_data)(struct us_socket_t *, char *data, int length);
    struct us_socket_t *(*on_writable)(struct us_socket_t *);
    struct us_socket_t *(*on_close)(struct us_socket_t *, int code, void *reason);
    //void (*on_timeout)(struct us_socket_context *);
    struct us_socket_t *(*on_socket_timeout)(struct us_socket_t *);
    struct us_socket_t *(*on_socket_long_timeout)(struct us_socket_t *);
    struct us_socket_t *(*on_end)(struct us_socket_t *);
    struct us_socket_t *(*on_connect_error)(struct us_socket_t *, int code);
    int (*is_low_prio)(struct us_socket_t *);*/

    on_open: *mut us_socket_context_t,
    on_data: *mut us_socket_context_t,
    on_writable: *mut us_socket_context_t,
    on_close: *mut us_socket_context_t,
    on_socket_timeout: *mut us_socket_context_t,
    on_socket_long_timeout: *mut us_socket_context_t,
    on_end: *mut us_socket_context_t,
    on_connect_error: *mut us_socket_context_t,
    is_low_prio: unsafe extern "C" fn (_s: u64) -> i32
}

#[no_mangle]
pub unsafe extern "C" fn us_internal_loop_data_free(_loop: *mut us_loop_t) {
    #[cfg(not(LIBUS_NO_SSL))]
    us_internal_free_loop_ssl_data(_loop);

    let loop_data = _loop as *mut us_internal_loop_data_t;

    free((*loop_data).recv_buf);

    us_timer_close((*loop_data).sweep_timer);
    us_internal_async_close((*loop_data).wakeup_async);
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_on_data(ssl: i32, context: *mut us_socket_context_t, on_data: *mut us_socket_context_t) {

    #[cfg(not(LIBUS_NO_SSL))]
    if ssl != 0 {
        us_internal_ssl_socket_context_on_data(/*(struct us_internal_ssl_socket_context_t *)*/ context, /*(struct us_internal_ssl_socket_t * (*)(struct us_internal_ssl_socket_t *, char *, int))*/ on_data);
        return;
    }

    (*context).on_data = on_data;
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_on_open(ssl: i32, context: *mut us_socket_context_t, on_open: *mut us_socket_context_t) {

    #[cfg(not(LIBUS_NO_SSL))]
    if ssl != 0 {
        us_internal_ssl_socket_context_on_open(/*(struct us_internal_ssl_socket_context_t *)*/ context, /*(struct us_internal_ssl_socket_t * (*)(struct us_internal_ssl_socket_t *, char *, int))*/ on_open);
        return;
    }

    (*context).on_open = on_open;
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_on_close(ssl: i32, context: *mut us_socket_context_t, on_close: *mut us_socket_context_t) {

    #[cfg(not(LIBUS_NO_SSL))]
    if ssl != 0 {
        us_internal_ssl_socket_context_on_close(/*(struct us_internal_ssl_socket_context_t *)*/ context, /*(struct us_internal_ssl_socket_t * (*)(struct us_internal_ssl_socket_t *, char *, int))*/ on_close);
        return;
    }

    (*context).on_close = on_close;
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_on_end(ssl: i32, context: *mut us_socket_context_t, on_end: *mut us_socket_context_t) {

    #[cfg(not(LIBUS_NO_SSL))]
    if ssl != 0 {
        us_internal_ssl_socket_context_on_end(/*(struct us_internal_ssl_socket_context_t *)*/ context, /*(struct us_internal_ssl_socket_t * (*)(struct us_internal_ssl_socket_t *, char *, int))*/ on_end);
        return;
    }

    (*context).on_end = on_end;
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_on_connect_error(ssl: i32, context: *mut us_socket_context_t, on_connect_error: *mut us_socket_context_t) {

    #[cfg(not(LIBUS_NO_SSL))]
    if ssl != 0 {
        us_internal_ssl_socket_context_on_connect_error(/*(struct us_internal_ssl_socket_context_t *)*/ context, /*(struct us_internal_ssl_socket_t * (*)(struct us_internal_ssl_socket_t *, char *, int))*/ on_connect_error);
        return;
    }

    (*context).on_connect_error = on_connect_error;
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_on_timeout(ssl: i32, context: *mut us_socket_context_t, on_timeout: *mut us_socket_context_t) {

    #[cfg(not(LIBUS_NO_SSL))]
    if ssl != 0 {
        us_internal_ssl_socket_context_on_timeout(/*(struct us_internal_ssl_socket_context_t *)*/ context, /*(struct us_internal_ssl_socket_t * (*)(struct us_internal_ssl_socket_t *, char *, int))*/ on_timeout);
        return;
    }

    (*context).on_socket_timeout = on_timeout;
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_on_long_timeout(ssl: i32, context: *mut us_socket_context_t, on_long_timeout: *mut us_socket_context_t) {

    #[cfg(not(LIBUS_NO_SSL))]
    if ssl != 0 {
        us_internal_ssl_socket_context_on_long_timeout(/*(struct us_internal_ssl_socket_context_t *)*/ context, /*(struct us_internal_ssl_socket_t * (*)(struct us_internal_ssl_socket_t *, char *, int))*/ on_long_timeout);
        return;
    }

    (*context).on_socket_long_timeout = on_long_timeout;
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_on_writable(ssl: i32, context: *mut us_socket_context_t, on_writable: *mut us_socket_context_t) {

    #[cfg(not(LIBUS_NO_SSL))]
    if ssl != 0 {
        us_internal_ssl_socket_context_on_writable(/*(struct us_internal_ssl_socket_context_t *)*/ context, /*(struct us_internal_ssl_socket_t * (*)(struct us_internal_ssl_socket_t *, char *, int))*/ on_writable);
        return;
    }

    (*context).on_writable = on_writable;
}

#[repr(C)]
pub struct us_internal_loop_data_t {
    sweep_timer: *mut us_socket_context_t,
    wakeup_async: *mut us_socket_context_t,
    last_write_failed: i32,
    head: *mut us_socket_context_t,
    iterator: *mut us_socket_context_t,


    recv_buf: *mut us_socket_context_t, // wrong
    ssl_data: *mut us_socket_context_t, // wrong
    pre_cb: *mut us_socket_context_t, // wrong
    post_cb: *mut us_socket_context_t, // wrong
    closed_head: *mut us_socket_context_t, // wrong
    low_prio_head: *mut us_socket_context_t, // wrong
    low_prio_budget: i32,
    /* We do not care if this flips or not, it doesn't matter */
    iteration_nr: u64 // long long?
}

const LIBUS_RECV_BUFFER_LENGTH: usize = 524288;
const LIBUS_RECV_BUFFER_PADDING: usize = 32;

/* The loop has 2 fallthrough polls */
#[no_mangle]
pub unsafe extern "C" fn us_internal_loop_data_init(loop_data: *mut us_internal_loop_data_t, wakeup_cb: *mut us_socket_context_t, pre_cb: *mut us_socket_context_t, post_cb: *mut us_socket_context_t) {

    (*loop_data).sweep_timer = us_create_timer(loop_data, 1, 0);
    (*loop_data).recv_buf = malloc(LIBUS_RECV_BUFFER_LENGTH + LIBUS_RECV_BUFFER_PADDING * 2) as *mut us_socket_context_t;
    (*loop_data).ssl_data = std::ptr::null_mut();
    (*loop_data).head = std::ptr::null_mut();
    (*loop_data).iterator = std::ptr::null_mut();
    (*loop_data).closed_head = std::ptr::null_mut();
    (*loop_data).low_prio_head = std::ptr::null_mut();
    (*loop_data).low_prio_budget = 0;

    (*loop_data).pre_cb = pre_cb;
    (*loop_data).post_cb = post_cb;
    (*loop_data).iteration_nr = 0;

    (*loop_data).wakeup_async = us_internal_create_async(loop_data, 1, 0);
    us_internal_async_set((*loop_data).wakeup_async, /*(void (*)(struct us_internal_async *))*/ wakeup_cb);
}

#[no_mangle]
pub unsafe extern "C" fn us_loop_iteration_number(loop_data: *mut us_internal_loop_data_t) -> u64 {
   (*loop_data).iteration_nr
}

// us_loop_t is castable to us_internal_loop_data
#[no_mangle]
pub unsafe extern "C" fn us_internal_loop_link(loop_data: *mut us_internal_loop_data_t, context: *mut us_socket_context_t) {
    /* Insert this context as the head of loop */
    (*context).next = (*loop_data).head;
    (*context).prev = std::ptr::null_mut();
    if !(*loop_data).head.is_null() {
        (*(*loop_data).head).prev = context;
    }
    (*loop_data).head = context;
}

/* Unlink is called before free */
#[no_mangle]
pub unsafe extern "C" fn us_internal_loop_unlink(loop_data: *mut us_internal_loop_data_t, context: *mut us_socket_context_t) {
    if (*loop_data).head == context {
        (*loop_data).head = (*context).next;
        if !(*loop_data).head.is_null() {
            (*(*loop_data).head).prev = std::ptr::null_mut();
        }
    } else {
        (*(*context).prev).next = (*context).next;
        if !(*context).next.is_null() {
            (*(*context).next).prev = (*context).prev;
        }
    }
}

/*
struct us_internal_loop_data_t {
    sweep_timer: const *us_timer_t,
    wakeup_async: const *us_internal_async,
    last_write_failed: i32,*/

    /*struct us_socket_context_t *head;
    struct us_socket_context_t *iterator;
    char *recv_buf;
    void *ssl_data;
    void (*pre_cb)(struct us_loop_t *);
    void (*post_cb)(struct us_loop_t *);
    struct us_socket_t *closed_head;
    struct us_socket_t *low_prio_head;
    int low_prio_budget;
    /* We do not care if this flips or not, it doesn't matter */
    long long iteration_nr;*/
//}

/*
struct us_socket_context_t {
    alignas(LIBUS_EXT_ALIGNMENT) struct us_loop_t *loop;
    uint32_t global_tick;
    unsigned char timestamp;
    unsigned char long_timestamp;
    struct us_socket_t *head_sockets;
    struct us_listen_socket_t *head_listen_sockets;
    struct us_socket_t *iterator;
    struct us_socket_context_t *prev, *next;

    struct us_socket_t *(*on_open)(struct us_socket_t *, int is_client, char *ip, int ip_length);
    struct us_socket_t *(*on_data)(struct us_socket_t *, char *data, int length);
    struct us_socket_t *(*on_writable)(struct us_socket_t *);
    struct us_socket_t *(*on_close)(struct us_socket_t *, int code, void *reason);
    //void (*on_timeout)(struct us_socket_context *);
    struct us_socket_t *(*on_socket_timeout)(struct us_socket_t *);
    struct us_socket_t *(*on_socket_long_timeout)(struct us_socket_t *);
    struct us_socket_t *(*on_end)(struct us_socket_t *);
    struct us_socket_t *(*on_connect_error)(struct us_socket_t *, int code);
    int (*is_low_prio)(struct us_socket_t *);
};
*/

#[no_mangle]
pub extern "C" fn us_socket_flush(_ssl: i32, s: u64) {
    unsafe {
        if us_socket_is_shut_down(0, s) == 0 {
            bsd_socket_flush(us_poll_fd(s));
        }
    }
}

#[no_mangle]
pub extern "C" fn us_socket_shutdown_read(_ssl: i32, s: u64) {
    unsafe {
        // This syscall is idempotent so no extra check is needed
        bsd_shutdown_socket_read(us_poll_fd(s));
    }
}


#[no_mangle]
pub extern "C" fn default_is_low_prio_handler(_s: u64) -> i32 {
    0
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_timestamp(_ssl: i32, context: *const us_socket_context_t) -> u16 {
    (*context).timestamp.into()
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_loop(_ssl: i32, context: *const us_socket_context_t) -> *mut us_internal_loop_data_t {
    (*context)._loop
}

#[no_mangle]
pub extern "C" fn bsd_recv(fd: i32, buf: u64, length: i32, flags: i32) -> i32 {
    unsafe {
        recv(fd, buf, length, flags)
    }
}

#[no_mangle]
pub extern "C" fn bsd_close_socket(fd: i32) {
    if cfg!(windows) {
        unsafe {
            closesocket(fd);
        }
    } else {
        unsafe {
            close(fd);
        }
    }
}

/* Options is currently only applicable for SSL - this will change with time (prefer_low_memory is one example) */
#[no_mangle]
pub unsafe extern "C" fn us_create_socket_context(ssl: i32, _loop: *mut us_internal_loop_data_t, context_ext_size: i32, options: us_socket_context_options_t) -> *mut us_socket_context_t {
    #[cfg(not(LIBUS_NO_SSL))]
    if ssl {
        /* This function will call us, again, with SSL = false and a bigger ext_size */
        return /*(struct us_socket_context_t *)*/ us_internal_create_ssl_socket_context(_loop, context_ext_size, options);
    }

    /* This path is taken once either way - always BEFORE whatever SSL may do LATER.
        * context_ext_size will however be modified larger in case of SSL, to hold SSL extensions */

    let context: *mut us_socket_context_t = malloc(std::mem::size_of::<us_socket_context_t>() + context_ext_size as usize);
    (*context)._loop = _loop;
    (*context).head_sockets = 0;
    (*context).head_listen_sockets = 0;
    (*context).iterator = 0;
    (*context).next = std::ptr::null_mut();
    (*context).is_low_prio = default_is_low_prio_handler;

    /* Begin at 0 */
    (*context).timestamp = 0;
    (*context).long_timestamp = 0;
    (*context).global_tick = 0;

    us_internal_loop_link(_loop, context);

    /* If we are called from within SSL code, SSL code will make further changes to us */
    return context;
}

#[no_mangle]
pub unsafe extern "C" fn us_socket_context_free(ssl: i32, context: *mut us_socket_context_t) {
    #[cfg(not(LIBUS_NO_SSL))]
    if (ssl) {
        /* This function will call us again with SSL=false */
        us_internal_ssl_socket_context_free(/*(struct us_internal_ssl_socket_context_t *)*/ context);
        return;
    }

    /* This path is taken once either way - always AFTER whatever SSL may do BEFORE.
        * This is the opposite order compared to when creating the context - SSL code is cleaning up before non-SSL */

    us_internal_loop_unlink((*context)._loop, context);
    free(context);
}