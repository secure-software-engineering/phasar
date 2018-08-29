/*
 * Copyright (c) 2014, Volkan Yazıcı <volkan.yazici@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * A simple usage of libpqueue. (Priorities are represented as "double" values.)
 */


#include <stdio.h>
#include <stdlib.h>

#include "pqueue.h"


typedef struct node_t
{
	pqueue_pri_t pri;
	int    val;
	size_t pos;
} node_t;


static int
cmp_pri(pqueue_pri_t next, pqueue_pri_t curr)
{
	return (next < curr);
}


static pqueue_pri_t
get_pri(void *a)
{
	return ((node_t *) a)->pri;
}


static void
set_pri(void *a, pqueue_pri_t pri)
{
	((node_t *) a)->pri = pri;
}


static size_t
get_pos(void *a)
{
	return ((node_t *) a)->pos;
}


static void
set_pos(void *a, size_t pos)
{
	((node_t *) a)->pos = pos;
}
	

int
main(void)
{
	pqueue_t *pq;
	node_t   *ns;
	node_t   *n;

	ns = malloc(10 * sizeof(node_t));
	pq = pqueue_init(10, cmp_pri, get_pri, set_pri, get_pos, set_pos);
	if (!(ns && pq)) return 1;

	ns[0].pri = 5; ns[0].val = -5; pqueue_insert(pq, &ns[0]);
	ns[1].pri = 4; ns[1].val = -4; pqueue_insert(pq, &ns[1]);
	ns[2].pri = 2; ns[2].val = -2; pqueue_insert(pq, &ns[2]);
	ns[3].pri = 6; ns[3].val = -6; pqueue_insert(pq, &ns[3]);
	ns[4].pri = 1; ns[4].val = -1; pqueue_insert(pq, &ns[4]);

	n = pqueue_peek(pq);
	printf("peek: %lld [%d]\n", n->pri, n->val);

	pqueue_change_priority(pq, 8, &ns[4]);
	pqueue_change_priority(pq, 7, &ns[2]);

   	while ((n = pqueue_pop(pq)))
		printf("pop: %lld [%d]\n", n->pri, n->val);

	pqueue_free(pq);
	free(ns);

	return 0;
}

/*
 * $ cc -Wall -g pqueue.c sample.c -o sample
 * $ ./sample
 * peek: 6 [-6]
 * pop: 8 [-1]
 * pop: 7 [-2]
 * pop: 6 [-6]
 * pop: 5 [-5]
 * pop: 4 [-4]
 */
