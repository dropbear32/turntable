/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "winsockfix.h"

#undef recv
#undef setsockopt

int recv_nt(SOCKET s, char *buf, int len, int flags) {
    // if (flags & MSG_DONTWAIT) {
    //     u_long one = 1;
    //     u_long zero = 0;
    //     ioctlsocket(s, FIONBIO, &one);
    //     int retval = recv(s, buf, len, flags);
    //     ioctlsocket(s, FIONBIO, &zero);
    //     return retval;
    // } else {
    return recv(s, buf, len, flags);
    // }
}

int setsockopt_nt(SOCKET s, int level, int optname, int *optval, int optlen) {
    if (optval == NULL)
        return -1;
    char optval_char = *optval;
    return setsockopt(s, level, optname, &optval_char, optlen);
}
