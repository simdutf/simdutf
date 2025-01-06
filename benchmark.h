/*-
 * Copyright (c) 2023 The FreeBSD Foundation
 *
 * This software was developed by Robert Clausecker <fuz@FreeBSD.org>
 * under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ''AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE
 */

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdbool.h>
#include <time.h>

/* benchframework.c */

struct B {
        long             n;             /* number of iterations */
        size_t           bytes;         /* number of bytes this benchmark processed */
        const char      *name;          /* name of this benchmark */

        void            (*func)(struct B *, void *);
        void            *payload;
        struct timespec  start;         /* start of test */
        struct timespec  duration;      /* time elapsed during test */
        struct timespec  prevduration;  /* time elapsed during last run of test */
        long             prevn;
        bool             timeron;       /* is the timer running? */
};

extern void		  starttimer(struct B *);
extern void		  stoptimer(struct B *);
extern void		  resettimer(struct B *);
extern long long	  elapsed(struct B *);
extern void		  preamble(void);
extern void		  runbenchmark(const char *, void (*)(struct B *, void *), void *);

#endif /* BENCHMARK_H */
