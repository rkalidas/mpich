/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include <stdio.h>
#include <string.h>

#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif

#include "coll_schedule_ring.h"
#include "coll_progress_util.h"
#include "coll_args_generic.h"

MPL_STATIC_INLINE_PREFIX int COLL_init()
{
    return 0;
}


MPL_STATIC_INLINE_PREFIX int COLL_comm_init(COLL_comm_t * comm, int *tag, int rank, int size)
{
    comm->curTag = tag;
    TSP_comm_init(&comm->tsp_comm, COLL_COMM_BASE(comm));
    return 0;
}

MPL_STATIC_INLINE_PREFIX int COLL_comm_cleanup(COLL_comm_t * comm)
{
    TSP_comm_cleanup(&comm->tsp_comm);
    return 0;
}

MPL_STATIC_INLINE_PREFIX int COLL_alltoall(const void     *sendbuf,
                      int                 sendcount,
                      COLL_dt_t          sendtype,
                      void               *recvbuf,
                      int                 recvcount,
                      COLL_dt_t          recvtype,
                      COLL_comm_t        *comm,
                      int *errflag)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_RING_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_RING_ALLTOALL);

    int mpi_errno = MPI_SUCCESS;

    /*generate the key for searching the schedule database */
    COLL_args_t coll_args = {.coll_op = ALLTOALL,
                            .args = {.alltoall = {.alltoall = {.sbuf = (void *) sendbuf,
                                                               .scount = sendcount,
                                                               .st_id = (int) sendtype,
                                                               .rbuf = recvbuf,
                                                               .rcount = recvcount,
                                                               .rt_id = (int) recvtype
                                                              }
                                                }
                                    }
                            };

    int is_new = 0;
    int tag = (*comm->curTag)++;

    /* Check with the transport if schedule already exisits
     * If it exists, reset and return the schedule else
     * return a new empty schedule*/
    int key_size = COLL_get_key_size (&coll_args);
    TSP_sched_t *s =
        TSP_get_schedule(&comm->tsp_comm, (void *) &coll_args, key_size, tag, &is_new);

    if (is_new) {       /*schedule is new */
        /*generate the schedule */
        mpi_errno =
            COLL_sched_alltoall_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, tag, comm, s, 1);
        /*save the schedule */
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, key_size, (void *) s);
    }

    /*execute the schedule */
    COLL_sched_wait(s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_RING_ALLTOALL);
    return mpi_errno;

}

#undef FUNCNAME
#define FUNCNAME COLL_bcast_get_ring_schedule
/* Internal function that returns broadcast schedule. This is used by COLL_bcast and COLL_ibcast
 * to get the broadcast schedule */
MPL_STATIC_INLINE_PREFIX int COLL_bcast_get_ring_schedule(void *buffer, int count, COLL_dt_t datatype,
                                                      int root, COLL_comm_t * comm, int segsize, TSP_sched_t **sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_BCAST_GET_RING_SCHEDULE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_BCAST_GET_RING_SCHEDULE);

    int mpi_errno = MPI_SUCCESS;

    /* generate key for searching the schedule database */
    COLL_args_t coll_args = {.coll_op = BCAST,
                             .args = {.bcast = {.bcast =
                                                 {.buf = buffer,
                                                  .count = count,
                                                  .dt_id = (int) datatype,
                                                  .root = root
                                                 },
                                                .segsize = segsize}
                                    }
                            };

    int is_new = 0;
    int tag = (*comm->curTag)++;

    /* Check with the transport if schedule already exisits
     * If it exists, reset and return the schedule else
     * return a new empty schedule */
    int key_size = COLL_get_key_size (&coll_args);
    *sched =
        TSP_get_schedule(&comm->tsp_comm, (void *) &coll_args, key_size, tag, &is_new);

    if (is_new) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Schedule does not exist\n"));
        /* generate the schedule */
        mpi_errno = COLL_sched_bcast_ring_pipelined(buffer, count, datatype, root, tag, comm, segsize, *sched, 1);
        /* store the schedule (it is optional for the transport to store the schedule */
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, key_size, (void *) *sched);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_BCAST_GET_RING_SCHEDULE);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME COLL_bcast
/* Blocking broadcast */
MPL_STATIC_INLINE_PREFIX int COLL_bcast(void *buffer, int count, COLL_dt_t datatype, int root,
                           COLL_comm_t * comm, int *errflag, int segsize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_RING_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_RING_BCAST);

    int mpi_errno = MPI_SUCCESS;

    /* get the schedule */
    TSP_sched_t *sched;
    mpi_errno = COLL_bcast_get_ring_schedule(buffer, count, datatype, root, comm, segsize, &sched);

    /* execute the schedule */
    COLL_sched_wait(sched);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_TREE_BCAST);

    return mpi_errno;
}