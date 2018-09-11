#include <uv.h>
#include <uv_lwip.h>
#include <assert.h>
#include <stdlib.h>

char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nX-Organization: Nintendo\r\n\r\nok";
uv_loop_t loop;
uvl_t uvl;

void write_cb()
{

}

void read_cb(uvl_tcp_t *handle, ssize_t nread, const uv_buf_t *buf)
{
    uvl_write_t *req = malloc(sizeof(uvl_write_t));
    uv_buf_t b;

    b.base = resp;
    b.len = strlen(resp);

    printf(buf->base);

    uvl_write(req, handle, &b, 1, write_cb);
}

void alloc_cb(uvl_tcp_t *handle, size_t suggested_size, uv_buf_t* buf)
{
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void on_connect(uvl_t *handle, int status)
{
    assert(status == 0);

    uvl_tcp_t *client = malloc(sizeof(uvl_tcp_t));

    assert(uvl_tcp_init(handle->loop, client) == 0);
    assert(uvl_accept(handle, client) == 0);

    uvl_read_start(client, alloc_cb, read_cb);
}



int main()
{
    assert(uv_loop_init(&loop) == 0);

    assert(uvl_init(&loop, &uvl) == 0);

    assert(uvl_listen(&uvl, on_connect) == 0);

    return uv_run(&loop, UV_RUN_DEFAULT);
}
