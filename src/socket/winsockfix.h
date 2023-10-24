/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#ifndef TURNTABLE_WINSOCKFIX_H
#define TURNTABLE_WINSOCKFIX_H

#include <WS2tcpip.h>
#include <WinSock2.h>

#define MSG_DONTWAIT              0x00
#define close(s)                  closesocket(s);
#define recv(a, b, c, d)          recv_nt(a, b, c, d)
#define setsockopt(a, b, c, d, e) setsockopt_nt(a, b, c, d, e)

int recv_nt(SOCKET s, char *buf, int len, int flags);
int setsockopt_nt(SOCKET s, int level, int optname, int *optval, int optlen);

#endif /* TURNTABLE_WINSOCKFIX_H */
