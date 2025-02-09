//------------------------------------------------------------------------------
// Copyright (C) 2019, EDV-Beratung Vogt GmbH
// Copyright (C) 2019, Robert Johansson, Raditex AB
// All rights reserved.
//------------------------------------------------------------------------------

#include "unistd.h"
#include <fcntl.h>

#include <winsock.h>
#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "mbus.h"
#include "mbus-tcp.h"

#define __PRETTY_FUNCTION__ __FUNCTION__
#define PACKET_BUFF_SIZE 2048

DWORD timeout = 4000;

//------------------------------------------------------------------------------
/// Setup a TCP/IP handle.
//------------------------------------------------------------------------------
//mbus_tcp_handle*
//mbus_tcp_connect(char* host, int port)
int
mbus_tcp_connect(mbus_handle* handle)
{
	char error_str[128];
	char* host;
	struct hostent* host_addr;
	struct sockaddr_in s;
	mbus_tcp_data* tcp_data;
	uint16_t port;

	if (handle == NULL)
		return -1;

	tcp_data = (mbus_tcp_data*)handle->auxdata;
	if (tcp_data == NULL || tcp_data->host == NULL)
		return -1;

	host = tcp_data->host;
	port = tcp_data->port;

	//
	// create the TCP connection
	//
	if ((handle->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		snprintf(error_str, sizeof(error_str), "%s: failed to setup a socket.", __PRETTY_FUNCTION__);
		mbus_error_str_set(error_str);
		return -1;
	}

	s.sin_family = AF_INET;
	s.sin_port = htons(port);

	/* resolve hostname */
	if ((host_addr = gethostbyname(host)) == NULL)
	{
		snprintf(error_str, sizeof(error_str), "%s: unknown host: %s", __PRETTY_FUNCTION__, host);
		mbus_error_str_set(error_str);
		return -1;
	}

	memcpy((void*)(&s.sin_addr), (void*)(host_addr->h_addr), host_addr->h_length);

	if (connect(handle->fd, (struct sockaddr*) & s, sizeof(s)) < 0)
	{
		snprintf(error_str, sizeof(error_str), "%s: Failed to establish connection to %s:%d", __PRETTY_FUNCTION__, host, port);
		mbus_error_str_set(error_str);
		return -1;
	}

	// Set a timeout
	setsockopt(handle->fd, SOL_SOCKET, SO_SNDTIMEO, (char*)& timeout, sizeof(timeout));
	setsockopt(handle->fd, SOL_SOCKET, SO_RCVTIMEO, (char*)& timeout, sizeof(timeout));

	return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void
mbus_tcp_data_free(mbus_handle* handle)
{
	mbus_tcp_data* tcp_data;

	if (handle == NULL)
		return;

	tcp_data = (mbus_tcp_data*)handle->auxdata;

	if (tcp_data == NULL)
		return;

	free(tcp_data->host);
	free(tcp_data);
	handle->auxdata = NULL;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_tcp_disconnect(mbus_handle* handle)
{
	if (handle == NULL)
		return -1;

	if (handle->fd < 0)
		return -1;

	closesocket(handle->fd);
	handle->fd = -1;

	return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_tcp_send_frame(mbus_handle* handle, mbus_frame* frame)
{
	unsigned char buff[PACKET_BUFF_SIZE];
	int len, ret;
	char error_str[128];

	if (handle == NULL || frame == NULL)
		return -1;

	if ((len = mbus_frame_pack(frame, buff, sizeof(buff))) == -1)
	{
		snprintf(error_str, sizeof(error_str), "%s: mbus_frame_pack failed\n", __PRETTY_FUNCTION__);
		mbus_error_str_set(error_str);
		return -1;
	}

	if ((ret = send(handle->fd, buff, len, 0)) == len)
	{
		//
		// call the send event function, if the callback function is registered
		// 
		if (handle->send_event)
			handle->send_event(MBUS_HANDLE_TYPE_TCP, buff, len);
	}
	else
	{
		snprintf(error_str, sizeof(error_str), "%s: Failed to write frame to socket (ret = %d)\n", __PRETTY_FUNCTION__, ret);
		mbus_error_str_set(error_str);
		return -1;
	}

	return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_tcp_recv_frame(mbus_handle* handle, mbus_frame* frame)
{
	char buff[PACKET_BUFF_SIZE];
	int remaining;
	ssize_t len, nread;

	if (handle == NULL || frame == NULL)
	{
		fprintf(stderr, "%s: Invalid parameter.\n", __PRETTY_FUNCTION__);
		return MBUS_RECV_RESULT_ERROR;
	}

	memset((void*)buff, 0, sizeof(buff));

	//
	// read data until a packet is received
	//
	remaining = 1; // start by reading 1 byte
	len = 0;

	do {
	retry:
		if (len + remaining > PACKET_BUFF_SIZE)
		{
			// avoid out of bounds access
			return MBUS_RECV_RESULT_ERROR;
		}

		nread = recv(handle->fd, &buff[len], remaining, 0);
		switch (nread)
		{
		case SOCKET_ERROR:
			errno = WSAGetLastError();
			if (errno == WSAEINTR)
				goto retry;

			if (errno == WSAETIMEDOUT || errno == WSAEWOULDBLOCK)
			{
				mbus_error_str_set("M-BUS tcp transport layer response timeout has been reached.");
				return MBUS_RECV_RESULT_TIMEOUT;
			}

			mbus_error_str_set("M-Bus tcp transport layer failed to read data.");
			return MBUS_RECV_RESULT_ERROR;
		case 0:
			mbus_error_str_set("M-Bus tcp transport layer connection closed by remote host.");
			return MBUS_RECV_RESULT_RESET;
		default:
			if (len > (SSIZE_MAX - nread))
			{
				// avoid overflow
				return MBUS_RECV_RESULT_ERROR;
			}

			len += nread;
		}
	} while ((remaining = mbus_parse(frame, buff, len)) > 0);

	//
	// call the receive event function, if the callback function is registered
	// 
	if (handle->recv_event)
		handle->recv_event(MBUS_HANDLE_TYPE_TCP, buff, len);

	if (remaining < 0)
	{
		mbus_error_str_set("M-Bus layer failed to parse data.");
		return MBUS_RECV_RESULT_INVALID;
	}

	return MBUS_RECV_RESULT_OK;
}

//------------------------------------------------------------------------------
/// The the timeout in seconds that will be used as the amount of time the
/// a read operation will wait before giving up. Note: This configuration has
/// to be made before calling mbus_tcp_connect.
//------------------------------------------------------------------------------
int
mbus_tcp_set_timeout_set(double seconds)
{
	if (seconds < 0.0)
	{
		mbus_error_str_set("Invalid timeout (must be positive).");
		return -1;
	}

	timeout = (DWORD)(seconds * 1000);

	return 0;
}
