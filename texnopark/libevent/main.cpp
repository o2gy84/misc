#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Функция обратного вызова для события: данные готовы для чтения в buf_ev */
static void echo_read_cb( struct bufferevent *buf_ev, void *arg )
{
  struct evbuffer *buf_input = bufferevent_get_input( buf_ev );
  struct evbuffer *buf_output = bufferevent_get_output( buf_ev );
  

    /* Данные просто копируются из буфера ввода в буфер вывода */
    evbuffer_add_buffer( buf_output, buf_input );
}

static void echo_event_cb( struct bufferevent *buf_ev, short events, void *arg )
{
  if( events & BEV_EVENT_ERROR )
    perror( "Ошибка объекта bufferevent" );
  if( events & (BEV_EVENT_EOF | BEV_EVENT_ERROR) )
    bufferevent_free( buf_ev );
}

static void accept_connection_cb( struct evconnlistener *listener,
                   evutil_socket_t fd, struct sockaddr *addr, int sock_len,
                   void *arg )
{
  /* При обработке запроса нового соединения необходимо создать для него
     объект bufferevent */
  struct event_base *base = evconnlistener_get_base( listener );
  struct bufferevent *buf_ev = bufferevent_socket_new( base, fd, BEV_OPT_CLOSE_ON_FREE );

  bufferevent_setcb( buf_ev, echo_read_cb, NULL, echo_event_cb, NULL );
  bufferevent_enable( buf_ev, (EV_READ | EV_WRITE) );
}

static void accept_error_cb( struct evconnlistener *listener, void *arg )
{
  struct event_base *base = evconnlistener_get_base( listener );
  int error = EVUTIL_SOCKET_ERROR();
  fprintf( stderr, "Ошибка %d (%s) в мониторе соединений. Завершение работы.\n",
           error, evutil_socket_error_to_string( error ) );
  event_base_loopexit( base, NULL );
}

int main( int argc, char **argv )
{
  struct event_base *base;
  struct evconnlistener *listener;
  struct sockaddr_in sin;
  int port = 9876;

  if( argc > 1 )  port = atoi( argv[1] );
  if( port < 0 || port > 65535 )
  {
    fprintf( stderr, "Задан некорректный номер порта.\n" );
    return -1;
  }

  base = event_base_new();
  if( !base )
  {
    fprintf( stderr, "Ошибка при создании объекта event_base.\n" );
    return -1;
  }

  memset( &sin, 0, sizeof(sin) );
  sin.sin_family = AF_INET;    /* работа с доменом IP-адресов */
  sin.sin_addr.s_addr = htonl( INADDR_ANY );  /* принимать запросы с любых адресов */
  sin.sin_port = htons( port );

  listener = evconnlistener_new_bind( base, accept_connection_cb, NULL,
                                     (LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE),
                                     -1, (struct sockaddr *)&sin, sizeof(sin) );
  if( !listener )
  {
    perror( "Ошибка при создании объекта evconnlistener" );
    return -1;
  }
  evconnlistener_set_error_cb( listener, accept_error_cb );

  event_base_dispatch( base );
  return 0;
}



