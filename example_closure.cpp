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

#include "co_closure.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <pthread.h>
#include <unistd.h>
using namespace std;

/**
 * 这个就展示下函数闭包吧，然后每个线程跑下
 */

static void *thread_func( void * arg )
{
	stCoClosure_t *p = (stCoClosure_t*) arg;
	p->exec();
	return 0;
}
static void batch_exec( vector<stCoClosure_t*> &v )
{
	// 每个协程分配一个线程
	vector<pthread_t> ths;
	for( size_t i=0;i<v.size();i++ )
	{
		pthread_t tid;
		pthread_create( &tid,0,thread_func,v[i] );
		ths.push_back( tid );
	}
	for( size_t i=0;i<v.size();i++ )
	{
		pthread_join( ths[i],0 );
	}
}
int main( int argc,char *argv[] )
{
	vector< stCoClosure_t* > v;

	pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

	int total = 100;
	vector<int> v2;
	/**
	 * expend:
	 * 
	 * typedef decltype( total ) typeof_total; 
	 * typedef decltype( v2 ) typeof_v2; 
	 * typedef decltype( m ) typeof_m; 
	 * class type_ref{
	 * public: 
	 * 	typeof_total & total; 
	 * 	typeof_v2 & v2; 
	 * 	typeof_m & m;  
	 * 	int _member_cnt;
	 *  type_ref( typeof_total & totalr, typeof_v2 & v2r, typeof_m & mr,  ... ): total(totalr), v2(v2r), m(mr),_member_cnt(3) {
	 *  }
	 * } 
	 * ref( total,v2,m ) ;		// 这里定义了一个 type_ref 对象，成员 int total; vector<int> v2; pthread_mutex_t m;
	 */
	co_ref( ref,total,v2,m);
	for(int i=0;i<10;i++)
	{
		/**
		 * expend:
		 * 
		 * typedef decltype( ref ) typeof_ref; 
		 * typedef decltype( i ) typeof_i;
		 * 
		 * class f : public stCoClosure_t{
		 * public: 
		 * 	typeof_ref ref; 
		 * 	typeof_i i;  
		 * 	int _member_cnt;
		 * public:
		 * 	f( typeof_ref & refr, typeof_i & ir,  ... ): ref(refr), i(ir),  _member_cnt(2) {} 
		 * 	void exec()
		 *
		 */
		co_func( f,ref,i )
		{
			// exec() 函数体
			printf("ref.total %d i %d\n",ref.total,i );
			//lock
			pthread_mutex_lock(&ref.m);
			ref.v2.push_back( i );
			pthread_mutex_unlock(&ref.m);
			//unlock
		}
		// expend
		// }
		co_func_end;
		// 定义 协程 stCoClosure_t 加入 v
		v.push_back( new f( ref,i ) );
	}
	for(int i=0;i<2;i++)
	{
		co_func( f2,i )
		{
			printf("i: %d\n",i);
			for(int j=0;j<2;j++)
			{
				usleep( 1000 );
				printf("i %d j %d\n",i,j);
			}
		}
		co_func_end;
		v.push_back( new f2( i ) );
	}

	batch_exec( v );
	printf("done\n");

	return 0;
}


