#include "lfds611_queue_internal.h"




/****************************************************************************/
struct lfds611_freelist_state *lfds611_queue_freelist_new( lfds611_atom_t number_elements, lfds611_atom_t number_queues )
{
  struct lfds611_freelist_state *fs;
  if( !lfds611_freelist_new( &fs, number_elements+number_queues, lfds611_queue_internal_freelist_init_function, NULL ))
    return NULL;
  return fs;
}


int lfds611_queue_new( struct lfds611_queue_state **qs, lfds611_atom_t number_elements )
{
  // FIXME cleanup on fail
  return lfds611_queue_new_with_freelist(qs, number_elements, lfds611_queue_freelist_new(number_elements, 1));
}

int lfds611_queue_new_with_freelist( struct lfds611_queue_state **qs, lfds611_atom_t number_elements, struct lfds611_freelist_state *fs )
{
  int
    rv = 0;

  struct lfds611_queue_element
    *qe[LFDS611_QUEUE_PAC_SIZE];

  assert( qs != NULL );

  if( fs == NULL )
  {
    rv = 1;
    *qs = NULL;
  }
  else
  {
    // TRD : number_elements can be any value in its range

    *qs = (struct lfds611_queue_state *) lfds611_liblfds_aligned_malloc( sizeof(struct lfds611_queue_state), LFDS611_ALIGN_DOUBLE_POINTER );

    if( *qs != NULL )
    {
        // TRD : the size of the lfds611_freelist is the size of the lfds611_queue (+1 for the leading dummy element, which is hidden from the caller)
        (*qs)->fs = fs;
        lfds611_queue_internal_new_element_from_freelist( *qs, qe, NULL );
        (*qs)->enqueue[LFDS611_QUEUE_POINTER] = (*qs)->dequeue[LFDS611_QUEUE_POINTER] = qe[LFDS611_QUEUE_POINTER];
        (*qs)->enqueue[LFDS611_QUEUE_COUNTER] = (*qs)->dequeue[LFDS611_QUEUE_COUNTER] = 0;
        (*qs)->aba_counter = 0;
        rv = 1;
    }
  }

  LFDS611_BARRIER_STORE;

  return( rv );
}





/****************************************************************************/
#pragma warning( disable : 4100 )

void lfds611_queue_use( struct lfds611_queue_state *qs )
{
  assert( qs != NULL );

  LFDS611_BARRIER_LOAD;

  return;
}

#pragma warning( default : 4100 )





/****************************************************************************/
#pragma warning( disable : 4100 )

int lfds611_queue_internal_freelist_init_function( void **user_data, void *user_state )
{
  int
    rv = 0;

  assert( user_data != NULL );
  assert( user_state == NULL );

  *user_data = lfds611_liblfds_aligned_malloc( sizeof(struct lfds611_queue_element), LFDS611_ALIGN_DOUBLE_POINTER );

  if( *user_data != NULL )
    rv = 1;

  return( rv );
}

#pragma warning( default : 4100 )





/****************************************************************************/
void lfds611_queue_internal_new_element_from_freelist( struct lfds611_queue_state *qs, struct lfds611_queue_element *qe[LFDS611_QUEUE_PAC_SIZE], void *user_data )
{
  struct lfds611_freelist_element
    *fe;

  assert( qs != NULL );
  assert( qe != NULL );
  // TRD : user_data can be any value in its range

  qe[LFDS611_QUEUE_POINTER] = NULL;

  lfds611_freelist_pop( qs->fs, &fe );

  if( fe != NULL )
    lfds611_queue_internal_init_element( qs, qe, fe, user_data );

  return;
}





/****************************************************************************/
void lfds611_queue_internal_guaranteed_new_element_from_freelist( struct lfds611_queue_state *qs, struct lfds611_queue_element *qe[LFDS611_QUEUE_PAC_SIZE], void *user_data )
{
  struct lfds611_freelist_element
    *fe;

  assert( qs != NULL );
  assert( qe != NULL );
  // TRD : user_data can be any value in its range

  qe[LFDS611_QUEUE_POINTER] = NULL;

  lfds611_freelist_guaranteed_pop( qs->fs, &fe );

  if( fe != NULL )
    lfds611_queue_internal_init_element( qs, qe, fe, user_data );

  return;
}





/****************************************************************************/
void lfds611_queue_internal_init_element( struct lfds611_queue_state *qs, struct lfds611_queue_element *qe[LFDS611_QUEUE_PAC_SIZE], struct lfds611_freelist_element *fe, void *user_data )
{
  assert( qs != NULL );
  assert( qe != NULL );
  assert( fe != NULL );
  // TRD : user_data can be any value in its range

  lfds611_freelist_get_user_data_from_element( fe, (void **) &qe[LFDS611_QUEUE_POINTER] );
  qe[LFDS611_QUEUE_COUNTER] = (struct lfds611_queue_element *) lfds611_abstraction_increment( (lfds611_atom_t *) &qs->aba_counter );

  qe[LFDS611_QUEUE_POINTER]->next[LFDS611_QUEUE_POINTER] = NULL;
  qe[LFDS611_QUEUE_POINTER]->next[LFDS611_QUEUE_COUNTER] = (struct lfds611_queue_element *) lfds611_abstraction_increment( (lfds611_atom_t *) &qs->aba_counter );

  qe[LFDS611_QUEUE_POINTER]->fe = fe;
  qe[LFDS611_QUEUE_POINTER]->user_data = user_data;

  return;
}

