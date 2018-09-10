#ifndef _uvl_H_
#define _uvl_H_

#include <uv.h>

typedef struct uvl {

} uvl_t;
typedef struct uvl_tcp {

} uvl_tcp_t;

typedef struct uvl_input {

} uvl_input_t;
typedef struct uvl_write {

} uvl_write_t;
typedef struct uvl_shutdown {

} uvl_shutdown_t;

typedef int (*uvl_output)(uvl_t *handle, const uv_buf_t bufs[], unsigned int nbufs);
typedef void (*uvl_input_cb)(uvl_input_t *req, int status);
typedef void (*uvl_close_cb)(uvl_t *handle);
typedef void (*uvl_connection_cb)(uvl_t *handle, int status);
typedef void (*uvl_read_cb)(uvl_tcp_t *handle, ssize_t nread, const uv_buf_t *buf);
typedef void (*uvl_write_cb)(uvl_write_t *req, int status);
typedef void (*uvl_shutdown_cb)(uvl_shutdown_t *req, int status);

int uvl_init(uv_loop_t *loop, uvl_t *handle);
int uvl_bind(uvl_t *handle, uvl_output output);
int uvl_input(uvl_input_t *req, uvl_t *handle, const uv_buf_t buf[], unsigned int nbufs, uvl_input_cb cb);
int uvl_listen(uvl_t *handle, uvl_connection_cb connection_cb);
int uvl_close(uvl_t *handle, uvl_close_cb close_cb);
int uvl_accept(uvl_t *handle, uvl_tcp_t *client);
int uvl_read_start(uvl_tcp_t *client, uv_alloc_cb alloc_cb, uvl_read_cb read_cb);
int uvl_write(uvl_write_t *req, uvl_tcp_t *client, const uv_buf_t bufs[], unsigned int nbufs, uvl_write_cb cb);
int uvl_shutdown(uvl_shutdown_t *req, uvl_tcp_t *client, uvl_shutdown_cb cb);

#endif // _uvl_H_
