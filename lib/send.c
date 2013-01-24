/* Copyright 2011 NORDUnet A/S. All rights reserved.
   See LICENSE for licensing information.  */

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <radsec/radsec.h>
#include <radsec/radsec-impl.h>
#include "debug.h"
#include "message.h"
#include "event.h"
#include "peer.h"
#include "conn.h"
#include "tcp.h"
#include "udp.h"

static int
_conn_open (struct rs_connection *conn, struct rs_message *msg)
{
  if (event_init_eventbase (conn))
    return -1;

  if (!conn->active_peer)
    peer_pick_peer (conn);
  if (!conn->active_peer)
    return rs_err_conn_push_fl (conn, RSE_NOPEER, __FILE__, __LINE__, NULL);

  if (event_init_socket (conn, conn->active_peer))
    return -1;

  if (conn->realm->type == RS_CONN_TYPE_TCP
      || conn->realm->type == RS_CONN_TYPE_TLS)
    {
      if (tcp_init_connect_timer (conn))
	return -1;
      if (event_init_bufferevent (conn, conn->active_peer))
	return -1;
    }
  else
    {
      if (udp_init (conn, msg))
	return -1;
      if (udp_init_retransmit_timer (conn))
	return -1;
    }

  if (!conn->is_connected)
    if (!conn->is_connecting)
      event_do_connect (conn);

  return RSE_OK;
}

static int
_conn_is_open_p (struct rs_connection *conn)
{
  return conn->active_peer && conn->is_connected;
}

/* User callback used when we're dispatching for user.  */
static void
_wcb (void *user_data)
{
  struct rs_message *msg = (struct rs_message *) user_data;
  assert (msg);
  msg->flags |= RS_MESSAGE_SENT;
  if (msg->conn->bev)
    bufferevent_disable (msg->conn->bev, EV_WRITE|EV_READ);
  else
    event_del (msg->conn->wev);
}

int
rs_message_send (struct rs_message *msg, void *user_data)
{
  struct rs_connection *conn = NULL;
  int err = 0;

  assert (msg);
  assert (msg->conn);
  conn = msg->conn;

  if (_conn_is_open_p (conn))
    message_do_send (msg);
  else
    if (_conn_open (conn, msg))
      return -1;

  assert (conn->evb);
  assert (conn->active_peer);
  assert (conn->fd >= 0);

  conn->user_data = user_data;

  if (conn->bev)		/* TCP */
    {
      bufferevent_setcb (conn->bev, NULL, tcp_write_cb, tcp_event_cb, msg);
      bufferevent_enable (conn->bev, EV_WRITE);
    }
  else				/* UDP */
    {
      event_assign (conn->wev, conn->evb, event_get_fd (conn->wev),
		    EV_WRITE, event_get_callback (conn->wev), msg);
      err = event_add (conn->wev, NULL);
      if (err < 0)
	return rs_err_conn_push_fl (conn, RSE_EVENT, __FILE__, __LINE__,
				    "event_add: %s",
				    evutil_gai_strerror (err));
    }

  /* Do dispatch, unless the user wants to do it herself.  */
  if (!conn_user_dispatch_p (conn))
    {
      conn->callbacks.sent_cb = _wcb;
      conn->user_data = msg;
      rs_debug (("%s: entering event loop\n", __func__));
      err = event_base_dispatch (conn->evb);
      if (err < 0)
	return rs_err_conn_push_fl (conn, RSE_EVENT, __FILE__, __LINE__,
				    "event_base_dispatch: %s",
				    evutil_gai_strerror (err));
      rs_debug (("%s: event loop done\n", __func__));
      conn->callbacks.sent_cb = NULL;
      conn->user_data = NULL;

      if ((msg->flags & RS_MESSAGE_SENT) == 0)
	{
	  assert (rs_err_conn_peek_code (conn));
	  return rs_err_conn_peek_code (conn);
	}
    }

  return RSE_OK;
}
