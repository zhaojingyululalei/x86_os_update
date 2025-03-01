#ifndef __TYPES_H
#define __TYPES_H

/*c99 支持 _Bool*/
#ifndef __cplusplus
#define  bool _Bool
#define true 1
#define false 0
#endif

#define UINT32_MAX 0xFFFFFFFF

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define NULL ((void*)0)



#endif