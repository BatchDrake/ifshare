#include <fqueue.h>

INSTANCER(fqueue)
{
  fqueue_t *new = NULL;

  ALLOCATE_FAIL(new, fqueue_t);
  
  TRYZ_FAIL(pthread_mutex_init(&new->mutex, NULL));
  TRYZ_FAIL(pthread_cond_init(&new->cond, NULL));

  return new;

fail:
  DISPOSE(fqueue, new);
}

COLLECTOR(fqueue)
{
  pthread_cond_destroy(&self->cond);
  pthread_mutex_destroy(&self->mutex);
  free(self);
}

/* Transfer ownership */

METHOD(fqueue, bool, push_frame, frame_t *frame)
{
  bool ok = false;
  struct fqueue_frame *current = NULL;

  pthread_mutex_lock(&self->mutex);

  if (self->free != NULL) {
    current    = self->free;
    self->free = current->next;
  } else {
    ALLOCATE(current, struct fqueue_frame);
  }

  gettimeofday(&current->tv, NULL);

  current->next = NULL;

  if (self->last == NULL) {
    self->first   = self->last = current;
    current->prev = NULL;
  } else {
    current->prev    = self->last;
    self->last->next = current;
    self->last       = current;
  }

  /* Transfer frame. No incref */
  current->frame = frame;
  frame = NULL;

  pthread_cond_signal(&self->cond);  

  ok = true;

done:
  if (frame != NULL)
    frame_dec_ref(frame);
  
  pthread_mutex_unlock(&self->mutex);
  return ok;
}

METHOD(fqueue, frame_t *, pop_frame)
{
  frame_t *frame = NULL;
  struct fqueue_frame *first;
  struct fqueue_frame *last;
  struct fqueue_frame *current;

  pthread_mutex_lock(&self->mutex);

  while (self->first == NULL)
    pthread_cond_wait(&self->cond, &self->mutex);
  
  /* No inc/ref, ownership is transferred directly to the caller */
  first = self->first;
  last  = self->last;

  current = first;

  frame = current->frame;

  if (first == last)
    first = last = NULL;
  else
    first = first->next;

  self->first       = first;
  self->last        = last;
  self->first->prev = NULL;

  /* Cache this one here */
  current->prev = NULL;
  current->next = self->free;

  pthread_mutex_unlock(&self->mutex);
}
