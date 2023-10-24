/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#ifndef TURNTABLE_ATOMIC_H
#define TURNTABLE_ATOMIC_H

#define atomic_write(dst, src) __atomic_store_n(dst, src, __ATOMIC_SEQ_CST)

#endif /* TURNTABLE_ATOMIC_H */
