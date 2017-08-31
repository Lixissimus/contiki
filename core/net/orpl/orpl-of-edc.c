/*
 * API for RPL objective functions (OF)
 *
 * reset(dag)
 *
 *  Resets the objective function state for a specific DAG. This function is
 *  called when doing a global repair on the DAG.
 *
 * parent_link_metric(parent)
 *
 *  Returns the link metric of a parent
 *
 * parent_has_usable_link(parent)
 *
 *  Returns 1 iff we have a usable link to this parent
 *
 * parent_path_cost(parent)
 *
 *  Returns the path cost of a parent
 *
 * rank_via_parent(parent)
 *
 *  Returns our rank if we select a given parent as preferred parent
 *
 * parent_is_acceptable
 *
 *  Returns 1 if a parent is usable as preferred parent, 0 otherwise
 *
 * best_parent(parent1, parent2)
 *
 *  Compares two parents and returns the best one, according to the OF.
 *
 * best_dag(dag1, dag2)
 *
 *  Compares two DAGs and returns the best one, according to the OF.
 *
 * update_metric_container(dag)
 *
 *  Updates the metric container for outgoing DIOs in a certain DAG.
 *  If the objective function of the DAG does not use metric containers,
 *  the function should set the object type to RPL_DAG_MC_NONE.
 *
 * dao_ack_callback(parent, status)
 *
 * A callback on the result of the DAO ACK. Similar to the neighbor link
 * callback. A failed DAO_ACK (NACK) can be used for switching to another
 * parent via changed link metric or other mechanisms.
 */
// struct rpl_of {
//   // void (*reset)(struct rpl_dag *);
// #if RPL_WITH_DAO_ACK
//   void (*dao_ack_callback)(rpl_parent_t *, int status);
// #endif
//   uint16_t (*parent_link_metric)(rpl_parent_t *);
//   int (*parent_has_usable_link)(rpl_parent_t *);
//   uint16_t (*parent_path_cost)(rpl_parent_t *);
//   // rpl_rank_t (*rank_via_parent)(rpl_parent_t *);
//   // rpl_parent_t *(*best_parent)(rpl_parent_t *, rpl_parent_t *);
//   // rpl_dag_t *(*best_dag)(rpl_dag_t *, rpl_dag_t *);
//   // void (*update_metric_container)( rpl_instance_t *);
//   // rpl_ocp_t ocp;
// };
// typedef struct rpl_of rpl_of_t;

#include "net/rpl/rpl.h"

#include "orpl.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

#if ORPL_ENABLED

/* Current hop-by-hop EDC, which is the expected strobe duration before getting
 * an ack from a parent. We maintain this as a moving average. */
static uint16_t hbh_edc = EDC_DIVISOR;

/* The size of the forwarder set and neighbor set.
 * Is this used anywhere? */
int forwarder_set_size = 0;

/*---------------------------------------------------------------------------*/
/* Utility function for computing the forwarder set. Adds a parent and returns the
 * resulting EDC */
static int
add_to_forwarder_set(rpl_parent_t *curr_p, rpl_rank_t curr_p_rank, uint16_t ackcount,
    uint32_t *curr_ackcount_sum, uint32_t *curr_ackcount_edc_sum)
{
  uint16_t tentative_edc;
  uint32_t total_tx_count;

  if(ackcount > orpl_broadcast_count) {
    ackcount = orpl_broadcast_count;
  }

  total_tx_count = orpl_broadcast_count;
  if(total_tx_count == 0) {
    /* No broadcast sent yet: assume a reception rate of 50% */
    ackcount = 1;
    total_tx_count = 2;
  }

  *curr_ackcount_sum += ackcount;
  *curr_ackcount_edc_sum += ackcount * curr_p_rank;

  /* The two main components of EDC: A, the cost of forwarding to any
   * parent, B the weighted mean EDC of the forwarder set */
  uint32_t A = hbh_edc * total_tx_count / *curr_ackcount_sum;
  uint32_t B = *curr_ackcount_edc_sum / *curr_ackcount_sum;

  /* Finally add W to EDC (cost of forwarding) */
  tentative_edc = A + B + ORPL_EDC_W;

  return tentative_edc;
}
/*---------------------------------------------------------------------------*/
rpl_rank_t
orpl_calculate_edc()
{
  rpl_rank_t edc = 0xffff;
  // rpl_rank_t prev_edc = orpl_current_edc();
  /* Counts the total number of ACKs received from nodes in the current set */
  uint32_t curr_ackcount_sum = 0;
  /* Counts the total number of EDC*ACKs received from nodes in the current set */
  uint32_t curr_ackcount_edc_sum = 0;
  /* Variables used for looping over parents and building the forwarder set */
  rpl_parent_t *p, *curr_p;
  int index = 0, curr_index = 0;
  uint16_t curr_p_rank = 0xffff;
  uint16_t curr_p_ackcount = 0xffff;

  int prev_index = -1;
  uint16_t prev_min_rank = 0;

  /* not used right now */
  // if(orpl_is_edc_frozen()) {
  //   return prev_edc;
  // }

  if(orpl_is_root()) {
    return 0;
  }

  forwarder_set_size = 0;

  /* Loop over the parents ordered by increasing rank, try to insert
   * them in the routing set until EDC does not improve. This is as
   * described in the IPSN'12 paper on ORW (upon which ORPL is built) */
  do {
    curr_p = NULL;

    /* This nested for loop finds the next parent for the do loop,
     * such as we get the parents by increasing rank */
    for(p = nbr_table_head(rpl_parents), index = 0;
        p != NULL;
        p = nbr_table_next(rpl_parents, p), index++) {
      uint16_t rank = p->rank;
      uint16_t ackcount = p->bc_ackcount;

      if(rank != 0xffff
          && !(orpl_broadcast_count > 0 && ackcount == 0)
          && (curr_p == NULL || rank < curr_p_rank)
          && (rank > prev_min_rank || (rank == prev_min_rank && index > prev_index))
      ) {
        curr_index = index;
        curr_p = p;
        curr_p_rank = rank;
        curr_p_ackcount = ackcount;
      }
    }

    /* Here, curr_p contains the current parent in our ordered lookup */
    if(curr_p) {
      rpl_rank_t tentative_edc;

      tentative_edc = add_to_forwarder_set(curr_p, curr_p_rank, curr_p_ackcount,
                &curr_ackcount_sum, &curr_ackcount_edc_sum);

      if(tentative_edc < edc) {
        /* The parent is now part of the forwarder set */
        edc = tentative_edc;
        forwarder_set_size++;
      } else {
        /* The parent is not part of the forwarder set. This means next parents won't be
         * part of the set either. */
        /* Todo: then we should break here, right? */
      }
      prev_index = curr_index;
      prev_min_rank = curr_p_rank;
    }
  } while(curr_p != NULL);

  return edc;
}
/*---------------------------------------------------------------------------*/
static void reset(rpl_dag_t *dag)
{
  PRINTF("ORPL: reset EDC\n");
  hbh_edc = EDC_DIVISOR;
  forwarder_set_size = 0;
}
/*---------------------------------------------------------------------------*/
static rpl_rank_t
rank_via_parent(rpl_parent_t *parent)
{
  rpl_rank_t edc = orpl_calculate_edc();
  orpl_update_edc(edc);
  return edc;
}
/*---------------------------------------------------------------------------*/
static rpl_dag_t *
best_dag(rpl_dag_t *d1, rpl_dag_t *d2)
{
  if(d1->grounded != d2->grounded) {
    return d1->grounded ? d1 : d2;
  }

  if(d1->preference != d2->preference) {
    return d1->preference > d2->preference ? d1 : d2;
  }

  return d1->rank < d2->rank ? d1 : d2;
}
/*---------------------------------------------------------------------------*/
static rpl_parent_t *
best_parent(rpl_parent_t *p1, rpl_parent_t *p2)
{
  /* With EDC, we don't need to compare parents */
  return p1;
}
/*---------------------------------------------------------------------------*/
static void
update_metric_container(rpl_instance_t *instance)
{
  /* We don't use any metric container (we only
   * use the rank field of DIO messages) */
}
/*---------------------------------------------------------------------------*/
rpl_of_t rpl_of_edc = {
  reset,
  NULL,
  NULL,
  NULL,
  rank_via_parent,
  best_parent,
  best_dag,
  update_metric_container,
  RPL_OCP_EDC
};

#endif /* ORPL_ENABLED */
