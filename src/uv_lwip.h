#ifndef _UVL_LWIP_H_
#define _UVL_LWIP_H_

#define UVL_NBUF_LEN 10

#include <uv.h>


#define UVL_FIELDS                                  \
    void *data;                                     \
    uv_loop_t *loop;                                \
    uv_close_cb close_cb;

typedef struct uvl uvl_t;
typedef struct uvl_tcp uvl_tcp_t;
typedef struct uvl_write uvl_write_t;
typedef struct uvl_shutdown uvl_shutdown_t;
typedef int (*uvl_output_fn)(uvl_t *handle, const uv_buf_t bufs[], unsigned int nbufs);
typedef void (*uvl_alloc_tcp_cb)(uvl_t *handle, size_t suggested_size, uv_buf_t* buf);
typedef void (*uvl_close_cb)(uvl_t *handle);
typedef void (*uvl_connection_cb)(uvl_t *handle, int status, uvl_tcp_t *tcp);
typedef void (*uvl_read_cb)(uvl_tcp_t *handle, ssize_t nread, const uv_buf_t *buf);
typedef void (*uvl_write_cb)(uvl_write_t *req, int status);
typedef void (*uvl_alloc_cb)(uvl_tcp_t *handle, size_t suggested_size, uv_buf_t* buf);
typedef void (*uvl_shutdown_cb)(uvl_shutdown_t *req, int status);

struct uvl {
    UVL_FIELDS

    uvl_output_fn output;
    uvl_connection_cb connection_cb;
    uv_alloc_cb alloc_cb;
    struct netif *the_netif;
    struct tcp_pcb *listener;
};
struct uvl_tcp {
    UVL_FIELDS
    uvl_t *handle;
};
struct uvl_write {
    UVL_FIELDS
    uvl_tcp_t *handle;
};
struct uvl_shutdown {
    UVL_FIELDS
    uvl_tcp_t *handle;
};

int uvl_init(uv_loop_t *loop, uvl_t *handle);
int uvl_bind(uvl_t *handle, uvl_output_fn output);
int uvl_input(uvl_t *handle, const uv_buf_t buf[], unsigned int nbufs);
int uvl_listen_start(uvl_t *handle, uvl_alloc_tcp_cb alloc_cb, uvl_connection_cb connection_cb);
int uvl_close(uvl_t *handle, uvl_close_cb close_cb);
int uvl_read_start(uvl_tcp_t *client, uvl_alloc_cb alloc_cb, uvl_read_cb read_cb);
int uvl_write(uvl_write_t *req, uvl_tcp_t *client, const uv_buf_t bufs[], unsigned int nbufs, uvl_write_cb cb);
int uvl_shutdown(uvl_shutdown_t *req, uvl_tcp_t *client, uvl_shutdown_cb cb);

#endif // _UVL_LWIP_H_
