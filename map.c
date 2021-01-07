/**
 @file  map.c
 @brief ENet integer keyed hashmap implementation
*/
#define ENET_BUILDING_LIB 1
#include "enet/enet.h"

enet_uint16 hashCode(ENetMap *map, enet_uint16 key) {
   return key % map -> size;
}

void
enet_map_init(ENetMap *map, enet_uint16 size) {
  map -> size = size;
  map -> hashArray = (ENetMapDataItem *) enet_malloc( size * (sizeof(ENetMapDataItem)));
  for (enet_uint16 i = 0; i < size; i++) {
    map -> hashArray[i].key = -1;
    map -> hashArray[i].value = -1;
  }
}

void
enet_map_free(ENetMap *map) {
  enet_free(map -> hashArray);
}


enet_uint16
enet_map_get(ENetMap *map, enet_uint16 key) {
   enet_uint16 hashIndex = hashCode(map, key);

  // Worst case scenario O(n) lookup. But the vast majority of lookups will
  //   hit on the first key in practice.
  for (enet_uint16 i = 0; i < map -> size; i++) {
    if (map -> hashArray[hashIndex].key == key) {
      return map -> hashArray[hashIndex].value;
    }
    ++hashIndex;
    hashIndex %= map -> size;
  }
  return -1;
}

int
enet_map_insert(ENetMap *map, enet_uint16 key, enet_uint16 value) {
  int hashIndex = hashCode(map, key);

  // Start at hashIndex, and loop in array until we find an empty cell
  for (enet_uint16 i = 0; i < map -> size; i++) {
    if (map -> hashArray[hashIndex].key == (enet_uint16)(-1)) {
      map -> hashArray[hashIndex].key = key;
      map -> hashArray[hashIndex].value = value;
      return 1;
    }

    ++hashIndex;
    hashIndex %= map -> size;
  }
  return 0;
}

void
enet_map_delete(ENetMap *map, enet_uint16 key) {
  enet_uint16 hashIndex = hashCode(map, key);

  // Worst case scenario O(n) lookup. But the vast majority of lookups will
  //   hit on the first key in practice.
  for (enet_uint16 i = 0; i < map -> size; i++) {
   if (map -> hashArray[hashIndex].key != key) {
     map -> hashArray[hashIndex].key = -1;
     map -> hashArray[hashIndex].value = -1;
     return;
   }
   ++hashIndex;
   hashIndex %= map -> size;
  }
}
