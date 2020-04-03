/** 
 @file  types.h
 @brief type definitions for ENet
*/
#ifndef __ENET_TYPES_H__
#define __ENET_TYPES_H__

typedef unsigned char enet_uint8;       /**< unsigned 8-bit type  */
typedef unsigned short enet_uint16;     /**< unsigned 16-bit type */
typedef unsigned int enet_uint32;      /**< unsigned 32-bit type */

#ifdef _MSC_VER
typedef signed __int64 enet_int64;
#else
#include <stdint.h>
typedef int64_t enet_int64;
#endif

#endif /* __ENET_TYPES_H__ */

