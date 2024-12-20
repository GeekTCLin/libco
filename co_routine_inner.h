/*
* Tencent is pleased to support the open source community by making Libco available.

* Copyright (C) 2014 THL A29 Limited, a Tencent company. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at
*
*	http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, 
* software distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License.
*/


#ifndef __CO_ROUTINE_INNER_H__

#include "co_routine.h"
#include "coctx.h"
struct stCoRoutineEnv_t;
struct stCoSpec_t
{
	void *value;
};

// 栈内存块
struct stStackMem_t
{
	stCoRoutine_t* occupy_co;	// 使用该空间的协程
	int stack_size;				// 栈字节长度
	char* stack_bp; 			//stack_buffer + stack_size 栈底 高地址 - 低地址
	char* stack_buffer;			// 栈空间

};

// 共享栈
struct stShareStack_t
{
	unsigned int alloc_idx;		// 访问下标
	int stack_size;				// 每个共享栈容量
	int count;					// stack_array 数量
	stStackMem_t** stack_array;
};


// 协程
struct stCoRoutine_t
{
	stCoRoutineEnv_t *env;	// 所属线程环境env
	pfn_co_routine_t pfn;	// 协程代理的方法体
	void *arg;				// pfn 方法 参数
	coctx_t ctx;			// 保存寄存器

	char cStart;			// 是否运行 = 1 为运行
	char cEnd;				// 是否结束 = 1 为已结束
	char cIsMain;			// 是否为主协程
	char cEnableSysHook;	// 是否启用hook
	char cIsShareStack;		// 是否使用共享栈

	void *pvEnv;

	//char sRunStack[ 1024 * 128 ];
	stStackMem_t* stack_mem;	// 协程栈


	//save satck buffer while confilct on same stack_buffer;
	// 这三个与 共享栈有空
	char* stack_sp; 
	unsigned int save_size;
	char* save_buffer;

	stCoSpec_t aSpec[1024];

};



//1.env
void 				co_init_curr_thread_env();
stCoRoutineEnv_t *	co_get_curr_thread_env();

//2.coroutine
void    co_free( stCoRoutine_t * co );
void    co_yield_env(  stCoRoutineEnv_t *env );

//3.func



//-----------------------------------------------------------------------------------------------

struct stTimeout_t;
struct stTimeoutItem_t ;

stTimeout_t *AllocTimeout( int iSize );
void 	FreeTimeout( stTimeout_t *apTimeout );
int  	AddTimeout( stTimeout_t *apTimeout,stTimeoutItem_t *apItem ,uint64_t allNow );

struct stCoEpoll_t;
stCoEpoll_t * AllocEpoll();
void 		FreeEpoll( stCoEpoll_t *ctx );

stCoRoutine_t *		GetCurrThreadCo();
void 				SetEpoll( stCoRoutineEnv_t *env,stCoEpoll_t *ev );

typedef void (*pfnCoRoutineFunc_t)();

#endif

#define __CO_ROUTINE_INNER_H__
