#ifndef __CONFIG_H__
#define __CONFIG_H__

//计时的最小时间
#define TIME_CYCLE 10
//程序运行时间
#define TASK_TIME  50

#define GET_STRUCT_MEMBER_ADDR(type,member) (unsigned long)(&((type *)0)->member)
#define GET_STRUCT_START_ADDR(type,member,member_addr) (void *)((char *)member_addr - GET_STRUCT_MEMBER_ADDR(type,member))

#endif
