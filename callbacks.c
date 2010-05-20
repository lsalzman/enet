/** 
 @file callbacks.c
 @brief ENet callback functions
*/
#define ENET_BUILDING_LIB 1
#include "enet/enet.h"

static ENetCallbacks callbacks = { malloc, free, rand, abort };

int
enet_initialize_with_callbacks (ENetVersion version, const ENetCallbacks * inits)
{
   if (inits -> malloc != NULL || inits -> free != NULL)
   {
      if (inits -> malloc == NULL || inits -> free == NULL)
        return -1;

      callbacks.malloc = inits -> malloc;
      callbacks.free = inits -> free;
   }
      
   if (inits -> rand != NULL)
     callbacks.rand = inits -> rand;

   if (version >= ENET_VERSION_CREATE (1, 2, 2))
   {
      if (inits -> no_memory != NULL)
        callbacks.no_memory = inits -> no_memory;
   }

   return enet_initialize ();
}
           
void *
enet_malloc (size_t size)
{
   void * memory = callbacks.malloc (size);

   if (memory == NULL)
     callbacks.no_memory ();

   return memory;
}

void
enet_free (void * memory)
{
   callbacks.free (memory);
}

int
enet_rand (void)
{
   return callbacks.rand ();
}

