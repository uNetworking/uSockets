use super::*;

struct epoll_event {
    events: u32,      /* Epoll events */
    data: u64        /* User data variable */
}

struct kevent {
	ident: usize,	/* identifier for this event */
	filter: i16,	/* filter for event */
	flags: u16, /* action flags for kqueue */
	fflags: u32,	/* filter flag value */
	data: i64,	/* filter data value */
	udata: usize	/* opaque user data identifier */
}


const LIBUS_SOCKET_READABLE: i32 = 1;
const LIBUS_SOCKET_WRITABLE: i32 = 2;

#[repr(C)]
pub struct us_loop_t {
    /* us_loop_t is always convertible to us_internal_loop_data_t */
    pub data: us_internal_loop_data_t,

    /* Number of non-fallthrough polls in the loop */
    num_polls: i32,

    /* Number of ready polls this iteration */
    num_ready_polls: i32,

    /* Current index in list of ready polls */
    current_ready_poll: i32,

    /* Loop's own file descriptor */
    fd: i32,

    /* The list of ready polls */
    #[cfg(LIBUS_USE_EPOLL)]
    ready_polls: [epoll_event; 1024],
    #[cfg(not(LIBUS_USE_EPOLL))]
    ready_polls: [kevent; 1024]
}

pub struct us_poll_t {
    data: u32
    // alignas(LIBUS_EXT_ALIGNMENT) struct {
    //     signed int fd : 28; // we could have this unsigned if we wanted to, -1 should never be used
    //     unsigned int poll_type : 4;
    // } state;
}

/* Loop */
#[no_mangle]
pub unsafe extern "C" fn us_loop_free(_loop: *mut us_loop_t) {
    us_internal_loop_data_free(_loop/*(*_loop)*//*.data*/);
    close((*_loop).fd);
    free(_loop as *mut us_socket_context_t);
}

unsafe fn k() {
    bsd_socket_flush(0);
}