/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef COLL_ARGS_GENERIC_H
#define COLL_ARGS_GENERIC_H

typedef struct {
} barrier_args_t; /* barrier had no arguments */

typedef struct {
    void *buf;
    int count;
    int dt_id;
    int root;
} bcast_args_t;

typedef struct {
    const void *sbuf;
    void *rbuf;
    int count;
    int dt_id;
    int op_id;
    int root;
} reduce_args_t;

typedef struct {
    const void *sbuf;
    void *rbuf;
    int count;
    int dt_id;
    int op_id;
} allreduce_args_t;

typedef struct {
    const void *sbuf;
    int scount;
    int st_id;
    void *rbuf;
    int rcount;
    int rt_id;
} alltoall_args_t;
#endif /* COLL_ARGS_GENERIC_H */