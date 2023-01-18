/**
 @file  map.h
 @brief ENet integer keyed hashmap implementation
*/
#ifndef __ENET_MAP_H__
#define __ENET_MAP_H__

#include <stdlib.h>

typedef struct _ENetMapDataItem {
   enet_uint16 value;
   enet_uint16 key;
} ENetMapDataItem;

typedef struct _ENetMap
{
   ENetMapDataItem * hashArray;
   enet_uint16 size;
} ENetMap;

// Init and delete map structs
extern void enet_map_init(ENetMap *map, enet_uint16 size);
extern void enet_map_free(ENetMap *map);

// Get, Insert, and Delete items into a map
extern enet_uint16 enet_map_get(ENetMap *map, enet_uint16 key);
extern int enet_map_insert(ENetMap *map, enet_uint16 key, enet_uint16 value);
extern void enet_map_delete(ENetMap *map, enet_uint16 key);

#endif /* __ENET_MAP_H__ */
