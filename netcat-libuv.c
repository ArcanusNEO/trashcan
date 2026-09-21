#if 0
exe="$(mktemp)"
cc -ggdb3 -O2 -fwrapv -fms-extensions -Wall -Wextra -Wvla -Wno-parentheses -Wno-microsoft "$0" -o "$exe" $(pkg-config --cflags --libs libuv) && trap "exec rm -f -- $exe" EXIT && "$exe" "$@"
ret="$?"
rm -f -- "$exe"
exit "$ret"
static_assert (0, "unreachable");
#endif
#include "cmacs.h"
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <uv.h>

#define INPUT_BUFFER_SIZE (64 * 1024)
#define UDP_PENDING_LIMIT (4 * 1024 * 1024)

/* libuv resumes I/O through callbacks; a stackless coroutine would add a
   second continuation state without removing those callbacks. */

typedef struct app app_t;
typedef struct tcp_write_request tcp_write_request_t;
typedef struct udp_write_request udp_write_request_t;
typedef struct pending_datagram pending_datagram_t;

struct tcp_write_request
{
  uv_write_t request;
  uv_buf_t buffer;
  app_t *app;
};

struct udp_write_request
{
  uv_udp_send_t request;
  uv_buf_t buffer;
  app_t *app;
};

struct pending_datagram
{
  pending_datagram_t *next;
  char *data;
  size_t length;
};

struct app
{
  uv_loop_t *loop;
  int udp;
  int listening;
  int failed;
  int exit_code;

  uv_pipe_t stdin_pipe;
  uv_fs_t stdin_request;
  char stdin_file_buffer[INPUT_BUFFER_SIZE];
  int stdin_initialized;
  int stdin_file;
  int stdin_file_pending;
  int stdin_file_closing;
  int stdin_reading;
  int stdin_eof;

  uv_tcp_t listener;
  int listener_initialized;

  uv_tcp_t tcp;
  uv_connect_t connect_request;
  uv_shutdown_t shutdown_request;
  int tcp_initialized;
  int tcp_ready;
  int tcp_reading;
  int tcp_write_shutdown;
  int tcp_shutdown_complete;
  int tcp_peer_eof;
  unsigned int pending_tcp_writes;

  uv_udp_t udp_socket;
  int udp_initialized;
  int udp_reading;
  int udp_peer_known;
  struct sockaddr_storage udp_peer;
  unsigned int pending_udp_writes;
  int close_udp_when_drained;
  pending_datagram_t *pending_datagrams;
  pending_datagram_t *pending_datagrams_tail;
  size_t pending_datagram_bytes;
};

typedef struct options
{
  int udp;
  int listen;
  const char *host;
  const char *port;
} options_t;

static void close_all (app_t *app);
static void free_pending_datagrams (app_t *app);
static void app_fail (app_t *app, const char *operation, int status);
static void app_fail_text (app_t *app, const char *message);
static int start_file_stdin (app_t *app);

static void
usage (const char *program)
{
  fprintf (stderr,
           "usage: %s [-u] host port\n"
           "       %s [-u] -l [host] port\n"
           "\n"
           "TCP is the default. -u selects UDP; -l listens for one TCP\n"
           "connection or keeps a UDP socket open.\n",
           program, program);
}

static int
parse_options (int argc, char *argv[], options_t *options)
{
  int index = 1;

  memset (options, 0, sizeof *options);
  while (index < argc)
    {
      if (!strcmp (argv[index], "-u"))
        options->udp = 1;
      else if (!strcmp (argv[index], "-l"))
        options->listen = 1;
      else if (!strcmp (argv[index], "--"))
        {
          ++index;
          break;
        }
      else if (!strcmp (argv[index], "-h") || !strcmp (argv[index], "--help"))
        return 1;
      else if (argv[index][0] == '-')
        return -1;
      else
        break;
      ++index;
    }

  if (options->listen)
    {
      if (argc - index == 1)
        {
          options->port = argv[index];
          return 0;
        }
      if (argc - index == 2)
        {
          options->host = argv[index];
          options->port = argv[index + 1];
          return 0;
        }
    }
  else if (argc - index == 2)
    {
      options->host = argv[index];
      options->port = argv[index + 1];
      return 0;
    }

  return -1;
}

static int
resolve_address (const char *host, const char *port, int socket_type,
                 int passive, struct sockaddr_storage *address)
{
  struct addrinfo hints = { 0 };
  struct addrinfo *result = NULL;
  int status;

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = socket_type;
  hints.ai_flags = passive ? AI_PASSIVE : 0;
  if (host != NULL && !strcmp (host, "*"))
    host = NULL;

  status = getaddrinfo (host, port, &hints, &result);
  if (status != 0)
    return status;

  if (result->ai_addrlen > sizeof *address)
    {
      freeaddrinfo (result);
      return EAI_FAIL;
    }

  memset (address, 0, sizeof *address);
  memcpy (address, result->ai_addr, result->ai_addrlen);
  freeaddrinfo (result);
  return 0;
}

static void
close_stdin (app_t *app)
{
  if (!app->stdin_initialized)
    return;

  if (app->stdin_file)
    {
      app->stdin_file_closing = 1;
      if (app->stdin_file_pending)
        uv_cancel ((uv_req_t *)&app->stdin_request);
      return;
    }

  if (app->stdin_reading)
    {
      uv_read_stop ((uv_stream_t *)&app->stdin_pipe);
      app->stdin_reading = 0;
    }
  if (!uv_is_closing ((uv_handle_t *)&app->stdin_pipe))
    uv_close ((uv_handle_t *)&app->stdin_pipe, NULL);
}

static void
close_listener (app_t *app)
{
  if (app->listener_initialized
      && !uv_is_closing ((uv_handle_t *)&app->listener))
    uv_close ((uv_handle_t *)&app->listener, NULL);
}

static void
close_tcp (app_t *app)
{
  if (!app->tcp_initialized)
    return;

  if (app->tcp_reading)
    {
      uv_read_stop ((uv_stream_t *)&app->tcp);
      app->tcp_reading = 0;
    }
  if (!uv_is_closing ((uv_handle_t *)&app->tcp))
    uv_close ((uv_handle_t *)&app->tcp, NULL);
}

static void
close_udp (app_t *app)
{
  if (!app->udp_initialized)
    return;

  if (app->udp_reading)
    {
      uv_udp_recv_stop (&app->udp_socket);
      app->udp_reading = 0;
    }
  if (!uv_is_closing ((uv_handle_t *)&app->udp_socket))
    uv_close ((uv_handle_t *)&app->udp_socket, NULL);
}

static void
close_all (app_t *app)
{
  close_stdin (app);
  close_listener (app);
  close_tcp (app);
  close_udp (app);
}

static void
free_pending_datagrams (app_t *app)
{
  pending_datagram_t *datagram = app->pending_datagrams;

  while (datagram != NULL)
    {
      pending_datagram_t *next = datagram->next;
      free (datagram->data);
      free (datagram);
      datagram = next;
    }
  app->pending_datagrams = NULL;
  app->pending_datagrams_tail = NULL;
  app->pending_datagram_bytes = 0;
}

static void
app_fail (app_t *app, const char *operation, int status)
{
  if (!app->failed)
    {
      app->failed = 1;
      app->exit_code = 1;
      fprintf (stderr, "netcat: %s: %s\n", operation, uv_strerror (status));
    }
  close_all (app);
  free_pending_datagrams (app);
}

static void
app_fail_text (app_t *app, const char *message)
{
  if (!app->failed)
    {
      app->failed = 1;
      app->exit_code = 1;
      fprintf (stderr, "netcat: %s\n", message);
    }
  close_all (app);
  free_pending_datagrams (app);
}

static void
maybe_close_tcp (app_t *app)
{
  if (!app->tcp_peer_eof || !app->tcp_shutdown_complete
      || app->pending_tcp_writes != 0)
    return;
  app->tcp_ready = 0;
  close_tcp (app);
  close_stdin (app);
}

static void
alloc_buffer (uv_handle_t *handle, size_t suggested_size, uv_buf_t *buffer)
{
  unsigned int length;

  (void)handle;
  if (suggested_size > UINT_MAX)
    length = UINT_MAX;
  else
    length = (unsigned int)suggested_size;
  if (length == 0)
    length = 1;
  buffer->base = malloc (length);
  buffer->len = buffer->base == NULL ? 0 : length;
}

static int
write_stdout (app_t *app, const char *data, size_t length)
{
  while (length != 0)
    {
      ssize_t written = write (STDOUT_FILENO, data, length);
      if (written > 0)
        {
          data += written;
          length -= (size_t)written;
          continue;
        }
      if (written < 0 && errno == EINTR)
        continue;
      app_fail_text (app, "write stdout");
      return -1;
    }
  return 0;
}

static void
on_tcp_write (uv_write_t *request, int status)
{
  tcp_write_request_t *write_request
      = container_of (request, tcp_write_request_t, request);
  app_t *app = write_request->app;

  free (write_request->buffer.base);
  free (write_request);
  if (app->pending_tcp_writes != 0)
    --app->pending_tcp_writes;
  if (status < 0 && !app->failed)
    {
      app_fail (app, "TCP write", status);
      return;
    }
  if (!app->failed)
    maybe_close_tcp (app);
}

static int
send_tcp (app_t *app, const char *data, size_t length)
{
  tcp_write_request_t *write_request;
  int status;

  write_request = malloc (sizeof *write_request);
  if (write_request == NULL)
    {
      app_fail_text (app, "allocate TCP write request");
      return -1;
    }
  write_request->buffer.base = malloc (length);
  if (write_request->buffer.base == NULL)
    {
      free (write_request);
      app_fail_text (app, "allocate TCP write buffer");
      return -1;
    }
  memcpy (write_request->buffer.base, data, length);
  write_request->buffer.len = (unsigned int)length;
  write_request->app = app;
  ++app->pending_tcp_writes;
  status = uv_write (&write_request->request, (uv_stream_t *)&app->tcp,
                     &write_request->buffer, 1, on_tcp_write);
  if (status < 0)
    {
      --app->pending_tcp_writes;
      free (write_request->buffer.base);
      free (write_request);
      app_fail (app, "TCP write", status);
      return -1;
    }
  return 0;
}

static void
on_udp_write (uv_udp_send_t *request, int status)
{
  udp_write_request_t *write_request
      = container_of (request, udp_write_request_t, request);
  app_t *app = write_request->app;

  free (write_request->buffer.base);
  free (write_request);
  if (app->pending_udp_writes != 0)
    --app->pending_udp_writes;
  if (status < 0 && !app->failed)
    {
      app_fail (app, "UDP write", status);
      return;
    }
  if (!app->failed && app->close_udp_when_drained
      && app->pending_udp_writes == 0)
    close_udp (app);
}

static int
send_udp_owned (app_t *app, char *data, size_t length)
{
  udp_write_request_t *write_request;
  const struct sockaddr *address = NULL;
  int status;

  if (app->listening)
    address = (const struct sockaddr *)&app->udp_peer;
  write_request = malloc (sizeof *write_request);
  if (write_request == NULL)
    {
      free (data);
      app_fail_text (app, "allocate UDP write request");
      return -1;
    }
  write_request->buffer = uv_buf_init (data, (unsigned int)length);
  write_request->app = app;
  ++app->pending_udp_writes;
  status = uv_udp_send (&write_request->request, &app->udp_socket,
                        &write_request->buffer, 1, address, on_udp_write);
  if (status < 0)
    {
      --app->pending_udp_writes;
      free (write_request->buffer.base);
      free (write_request);
      app_fail (app, "UDP write", status);
      return -1;
    }
  return 0;
}

static int
queue_udp (app_t *app, const char *data, size_t length)
{
  pending_datagram_t *datagram;

  if (length > UDP_PENDING_LIMIT - app->pending_datagram_bytes)
    {
      app_fail_text (app, "UDP peer is not known and pending input is full");
      return -1;
    }
  datagram = malloc (sizeof *datagram);
  if (datagram == NULL)
    {
      app_fail_text (app, "allocate pending UDP datagram");
      return -1;
    }
  datagram->data = malloc (length);
  if (datagram->data == NULL)
    {
      free (datagram);
      app_fail_text (app, "allocate pending UDP data");
      return -1;
    }
  memcpy (datagram->data, data, length);
  datagram->length = length;
  datagram->next = NULL;
  if (app->pending_datagrams_tail == NULL)
    app->pending_datagrams = datagram;
  else
    app->pending_datagrams_tail->next = datagram;
  app->pending_datagrams_tail = datagram;
  app->pending_datagram_bytes += length;
  return 0;
}

static int
send_udp (app_t *app, const char *data, size_t length)
{
  char *copy;

  if (app->listening && !app->udp_peer_known)
    return queue_udp (app, data, length);
  copy = malloc (length);
  if (copy == NULL)
    {
      app_fail_text (app, "allocate UDP write buffer");
      return -1;
    }
  memcpy (copy, data, length);
  return send_udp_owned (app, copy, length);
}

static void
flush_pending_udp (app_t *app)
{
  while (!app->failed && app->udp_peer_known && app->pending_datagrams != NULL)
    {
      pending_datagram_t *datagram = app->pending_datagrams;
      app->pending_datagrams = datagram->next;
      if (app->pending_datagrams == NULL)
        app->pending_datagrams_tail = NULL;
      app->pending_datagram_bytes -= datagram->length;
      if (send_udp_owned (app, datagram->data, datagram->length) < 0)
        {
          free (datagram);
          return;
        }
      free (datagram);
    }
}

static void
on_tcp_shutdown (uv_shutdown_t *request, int status)
{
  app_t *app = container_of (request, app_t, shutdown_request);

  if (status < 0 && status != UV_ECANCELED && !app->failed)
    {
      app_fail (app, "TCP shutdown", status);
      return;
    }
  if (!app->failed)
    {
      app->tcp_shutdown_complete = 1;
      maybe_close_tcp (app);
    }
}

static void
input_eof (app_t *app)
{
  int status;

  app->stdin_eof = 1;
  if (app->udp)
    {
      if (!app->listening)
        {
          if (app->pending_udp_writes == 0)
            close_udp (app);
          else
            app->close_udp_when_drained = 1;
        }
      return;
    }
  if (!app->tcp_ready || app->tcp_write_shutdown
      || uv_is_closing ((uv_handle_t *)&app->tcp))
    return;
  status = uv_shutdown (&app->shutdown_request, (uv_stream_t *)&app->tcp,
                        on_tcp_shutdown);
  if (status < 0)
    app_fail (app, "TCP shutdown", status);
  else
    app->tcp_write_shutdown = 1;
}

static void
on_stdin_read (uv_stream_t *stream, ssize_t nread, const uv_buf_t *buffer)
{
  app_t *app = stream->data;

  if (nread > 0 && !app->failed)
    {
      if (app->udp)
        send_udp (app, buffer->base, (size_t)nread);
      else if (app->tcp_ready)
        send_tcp (app, buffer->base, (size_t)nread);
    }
  if (nread < 0)
    {
      if (nread != UV_EOF && !app->failed)
        app_fail (app, "stdin read", (int)nread);
      if (!app->failed)
        {
          close_stdin (app);
          input_eof (app);
        }
    }
  free (buffer->base);
}

static void
on_stdin_file_read (uv_fs_t *request)
{
  app_t *app = request->data;
  ssize_t nread = request->result;

  uv_fs_req_cleanup (request);
  app->stdin_file_pending = 0;
  if (app->stdin_file_closing || app->failed)
    return;
  if (nread > 0)
    {
      if (app->udp)
        send_udp (app, app->stdin_file_buffer, (size_t)nread);
      else if (app->tcp_ready)
        send_tcp (app, app->stdin_file_buffer, (size_t)nread);
      if (!app->failed)
        start_file_stdin (app);
    }
  else if (nread == 0)
    input_eof (app);
  else
    app_fail (app, "stdin read", (int)nread);
}

static void
on_tcp_read (uv_stream_t *stream, ssize_t nread, const uv_buf_t *buffer)
{
  app_t *app = stream->data;

  if (nread > 0 && !app->failed)
    write_stdout (app, buffer->base, (size_t)nread);
  if (nread < 0)
    {
      if (nread != UV_EOF && !app->failed)
        app_fail (app, "TCP read", (int)nread);
      if (!app->failed)
        {
          if (nread == UV_EOF)
            {
              app->tcp_peer_eof = 1;
              if (app->tcp_reading)
                {
                  uv_read_stop ((uv_stream_t *)&app->tcp);
                  app->tcp_reading = 0;
                }
              maybe_close_tcp (app);
            }
        }
    }
  free (buffer->base);
}

static size_t
sockaddr_size (const struct sockaddr *address)
{
  if (address->sa_family == AF_INET)
    return sizeof (struct sockaddr_in);
  if (address->sa_family == AF_INET6)
    return sizeof (struct sockaddr_in6);
  return 0;
}

static void
on_udp_read (uv_udp_t *socket, ssize_t nread, const uv_buf_t *buffer,
             const struct sockaddr *address, unsigned flags)
{
  app_t *app = socket->data;

  if (nread > 0 && !app->failed)
    {
      if (app->listening && address != NULL)
        {
          size_t length = sockaddr_size (address);
          if (length != 0)
            {
              memcpy (&app->udp_peer, address, length);
              app->udp_peer_known = 1;
              flush_pending_udp (app);
            }
        }
      if (!app->failed)
        write_stdout (app, buffer->base, (size_t)nread);
    }
  if (nread < 0 && !app->failed)
    app_fail (app, "UDP read", (int)nread);

  if (!(flags & UV_UDP_MMSG_CHUNK) || (flags & UV_UDP_MMSG_FREE))
    free (buffer->base);
}

static int
start_stdin (app_t *app)
{
  struct stat statbuf;
  int status;

  if (app->stdin_initialized)
    return 0;
  if (fstat (STDIN_FILENO, &statbuf) == 0 && !isatty (STDIN_FILENO)
      && (S_ISREG (statbuf.st_mode) || S_ISCHR (statbuf.st_mode)
          || S_ISBLK (statbuf.st_mode)))
    {
      app->stdin_initialized = 1;
      app->stdin_file = 1;
      return start_file_stdin (app);
    }
  status = uv_pipe_init (app->loop, &app->stdin_pipe, 0);
  if (status < 0)
    {
      app_fail (app, "initialize stdin", status);
      return -1;
    }
  app->stdin_initialized = 1;
  app->stdin_pipe.data = app;
  status = uv_pipe_open (&app->stdin_pipe, STDIN_FILENO);
  if (status < 0)
    {
      app_fail (app, "open stdin", status);
      return -1;
    }
  status = uv_read_start ((uv_stream_t *)&app->stdin_pipe, alloc_buffer,
                          on_stdin_read);
  if (status < 0)
    {
      app_fail (app, "read stdin", status);
      return -1;
    }
  app->stdin_reading = 1;
  return 0;
}

static int
start_file_stdin (app_t *app)
{
  uv_buf_t buffer
      = uv_buf_init (app->stdin_file_buffer, sizeof app->stdin_file_buffer);
  int status;

  app->stdin_request.data = app;
  app->stdin_file_pending = 1;
  status = uv_fs_read (app->loop, &app->stdin_request, STDIN_FILENO, &buffer,
                       1, -1, on_stdin_file_read);
  if (status < 0)
    {
      app->stdin_file_pending = 0;
      uv_fs_req_cleanup (&app->stdin_request);
      app_fail (app, "read stdin", status);
      return -1;
    }
  return 0;
}

static int
start_tcp_read (app_t *app)
{
  int status
      = uv_read_start ((uv_stream_t *)&app->tcp, alloc_buffer, on_tcp_read);
  if (status < 0)
    {
      app_fail (app, "read TCP", status);
      return -1;
    }
  app->tcp_reading = 1;
  return 0;
}

static void
on_tcp_connect (uv_connect_t *request, int status)
{
  app_t *app = request->data;

  if (app->failed)
    return;
  if (status < 0)
    {
      app_fail (app, "TCP connect", status);
      return;
    }
  app->tcp_ready = 1;
  if (start_tcp_read (app) < 0)
    return;
  if (start_stdin (app) < 0)
    return;
  if (app->stdin_eof)
    input_eof (app);
}

static void
on_tcp_connection (uv_stream_t *server, int status)
{
  app_t *app = server->data;

  if (app->failed)
    return;
  if (status < 0)
    {
      app_fail (app, "TCP accept", status);
      return;
    }

  status = uv_tcp_init (app->loop, &app->tcp);
  if (status < 0)
    {
      app_fail (app, "initialize TCP connection", status);
      return;
    }
  app->tcp_initialized = 1;
  app->tcp.data = app;
  status = uv_accept (server, (uv_stream_t *)&app->tcp);
  if (status < 0)
    {
      app_fail (app, "accept TCP connection", status);
      return;
    }
  app->tcp_ready = 1;
  close_listener (app);
  if (start_tcp_read (app) < 0)
    return;
  if (!app->stdin_eof && start_stdin (app) < 0)
    return;
  if (app->stdin_eof)
    input_eof (app);
}

static int
start_tcp_client (app_t *app, const struct sockaddr_storage *address)
{
  int status;

  status = uv_tcp_init (app->loop, &app->tcp);
  if (status < 0)
    {
      app_fail (app, "initialize TCP", status);
      return -1;
    }
  app->tcp_initialized = 1;
  app->tcp.data = app;
  app->connect_request.data = app;
  status = uv_tcp_connect (&app->connect_request, &app->tcp,
                           (const struct sockaddr *)address, on_tcp_connect);
  if (status < 0)
    {
      app_fail (app, "TCP connect", status);
      return -1;
    }
  return 0;
}

static int
start_tcp_listener (app_t *app, const struct sockaddr_storage *address)
{
  int status;

  status = uv_tcp_init (app->loop, &app->listener);
  if (status < 0)
    {
      app_fail (app, "initialize TCP listener", status);
      return -1;
    }
  app->listener_initialized = 1;
  app->listener.data = app;
  status = uv_tcp_bind (&app->listener, (const struct sockaddr *)address, 0);
  if (status < 0)
    {
      app_fail (app, "bind TCP listener", status);
      return -1;
    }
  status = uv_listen ((uv_stream_t *)&app->listener, 1, on_tcp_connection);
  if (status < 0)
    {
      app_fail (app, "listen TCP", status);
      return -1;
    }
  return 0;
}

static void
on_udp_connection_ready (app_t *app)
{
  int status;

  status = uv_udp_recv_start (&app->udp_socket, alloc_buffer, on_udp_read);
  if (status < 0)
    {
      app_fail (app, "read UDP", status);
      return;
    }
  app->udp_reading = 1;
  start_stdin (app);
}

static int
start_udp (app_t *app, const struct sockaddr_storage *address)
{
  int status;

  status = uv_udp_init (app->loop, &app->udp_socket);
  if (status < 0)
    {
      app_fail (app, "initialize UDP", status);
      return -1;
    }
  app->udp_initialized = 1;
  app->udp_socket.data = app;
  if (app->listening)
    status = uv_udp_bind (&app->udp_socket, (const struct sockaddr *)address,
                          UV_UDP_REUSEADDR);
  else
    status
        = uv_udp_connect (&app->udp_socket, (const struct sockaddr *)address);
  if (status < 0)
    {
      app_fail (app, app->listening ? "bind UDP listener" : "connect UDP",
                status);
      return -1;
    }
  on_udp_connection_ready (app);
  return app->failed ? -1 : 0;
}

int
main (int argc, char *argv[])
{
  options_t options;
  struct sockaddr_storage address;
  app_t app = { 0 };
  uv_loop_t loop;
  int status;

  if (signal (SIGPIPE, SIG_IGN) == SIG_ERR)
    return 1;
  status = parse_options (argc, argv, &options);
  if (status == 1)
    {
      usage (argv[0]);
      return 0;
    }
  if (status < 0)
    {
      usage (argv[0]);
      return 2;
    }

  status = resolve_address (options.host, options.port,
                            options.udp ? SOCK_DGRAM : SOCK_STREAM,
                            options.listen, &address);
  if (status != 0)
    {
      fprintf (stderr, "netcat: resolve %s: %s\n",
               options.host == NULL ? "*" : options.host,
               gai_strerror (status));
      return 1;
    }

  status = uv_loop_init (&loop);
  if (status < 0)
    {
      fprintf (stderr, "netcat: initialize event loop: %s\n",
               uv_strerror (status));
      return 1;
    }
  app.loop = &loop;
  app.udp = options.udp;
  app.listening = options.listen;
  if (app.udp)
    start_udp (&app, &address);
  else if (app.listening)
    start_tcp_listener (&app, &address);
  else
    start_tcp_client (&app, &address);

  uv_run (&loop, UV_RUN_DEFAULT);
  free_pending_datagrams (&app);
  status = uv_loop_close (&loop);
  if (status < 0 && !app.failed)
    {
      fprintf (stderr, "netcat: close event loop: %s\n", uv_strerror (status));
      app.exit_code = 1;
    }
  return app.exit_code;
}
