/*
  fqueue.h: Frame queue API
  Copyright (C) 2025 Gonzalo José Carracedo Carballal
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, version 3.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

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
  struct fqueue_frame *this;
  struct fqueue_frame *next;

  this = self->free;
  while (this != NULL) {
    next = this->next;
    free(this);
    this = next;
  }

  this = self->first;
  while (this != NULL) {
    if (this->frame != NULL)
      frame_dec_ref(this->frame);
    next = this->next;
    free(this);
    this = next;
  }

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

  current->next = NULL;
  current->prev = self->last;

  if (self->last == NULL)
    self->first = current;
  else
    self->last->next = current;
  
  self->last = current;

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
  first   = self->first;
  last    = self->last;
  current = first;

  frame   = current->frame;

  if (first == last)
    last = NULL;
  
  first = first->next;
  if (first != NULL)
    first->prev = NULL;
  
  self->first       = first;
  self->last        = last;
  
  /* Cache this one here */
  current->prev = NULL;
  current->next = self->free;

  pthread_mutex_unlock(&self->mutex);

  return frame;
}
