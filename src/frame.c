#include <frame.h>


static pthread_mutex_t g_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static frame_t        *g_pool       = NULL;

INSTANCER(frame, size_t size)
{
  frame_t *new = NULL;
  bool complete = false;

  pthread_mutex_lock(&g_pool_mutex);

  if (g_pool != NULL) {
    new      = g_pool;
    g_pool   = new->next;
  }

  pthread_mutex_unlock(&g_pool_mutex);

  if (new == NULL) {
    ALLOCATE_FAIL(new, frame_t);
    TRYZ_FAIL(pthread_mutex_init(&new->mutex, NULL));
  }

  complete = true;

  TRY_FAIL(frame_resize(new, size));

  new->refcnt = 0;
  frame_inc_ref(new);

  return new;

fail:
  if (new != NULL) {
    if (complete)
      DISPOSE(frame, new);
    else
      free(new);
  }
  
  return NULL;
}

COLLECTOR(frame)
{
  pthread_mutex_lock(&g_pool_mutex);

  self->next = g_pool;
  g_pool = self;

  pthread_mutex_unlock(&g_pool_mutex);
}

METHOD(frame, bool, resize, size_t size)
{
  bool ok = false;

  if (size > self->alloc) {
    size_t new_alloc = self->alloc;
    uint8_t *tmp;

    if (new_alloc == 0)
      new_alloc = 16;

    while (new_alloc < size)
      new_alloc <<= 1;

    TRY(tmp = realloc(self->data, new_alloc));
    self->data  = tmp;
    self->alloc = new_alloc;
  }

  self->size = size;

  ok = true;

done:
  return ok;
}

GETTER(frame, size_t, size)
{
  return self->size;
}

GETTER(frame, size_t, allocation)
{
  return self->alloc;
}

METHOD(frame, void, inc_ref)
{
  pthread_mutex_lock(&self->mutex);
  ++self->refcnt;
  pthread_mutex_unlock(&self->mutex);
}

METHOD(frame, bool, dec_ref)
{
  bool dispose;

  pthread_mutex_lock(&self->mutex);
  dispose = --self->refcnt == 0;
  pthread_mutex_unlock(&self->mutex);

  if (dispose)
    DISPOSE(frame, self);

  return !dispose;
}
