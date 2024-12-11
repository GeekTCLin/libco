## libco 源码分析
### [参考文章](https://www.cnblogs.com/ljx-null/p/15928889.html)
### 设计目标
libco 的设计目标是对现有同步代码做异步改造，但完全重写为异步代码工作量大，已上线项目风险大？？？

### 协程优势
以同步的方式实现异步执行逻辑，当执行异步操作时，将协程挂起，等异步操作返回时唤醒协程继续执行，避免编写大量回调代码，业务代码呈现为线性方式

而在考虑阻塞情况下，非io复用回调情况下，阻塞会导致当前线程挂起，使得线程切换，使用协程可使得该线程继续执行，而不会引起上下文切换

### 非对称和对称协程
#### 非对称协程
libco 基于非对称协程，存在调用方和被调用方的关系，控制流程为堆栈式，调用方可以拉起调用方并将控制权交给被调用方，被调用方挂起的时候，将控制权交还给调用方

#### 对称协程
对称协程可以不受限将控制权限交给其他协程，参考RESTfulCweb 源码实现，协程挂起和主协程进行置换，不同状态的协程（可运行/挂起）处于不同队列或链表中

### hook 机制
协程调度的目标是为了解决 CPU 利用率与 I/O 利用率不平衡的问题，反映在同步代码中也就是阻塞等待 I/O 会导致 CPU 资源浪费

**因此最佳协程切换时机就是执行阻塞系统调用操作之前**

所以libco的目的在于阻塞调用前，将其改为带协程切换的协程库接口

Libco 采取的方案是利用 Library Interpositioning 的原理，在执行阻塞系统调用前自动执行协程切换

```cpp
typedef ssize_t (*read_pfn_t)(int fildes, void *buf, size_t nbyte);
static read_pfn_t g_sys_read_func = (read_pfn_t)dlsym(RTLD_NEXT, "read");
```
通过 dlsym，将系统的阻塞 api 接口符号名改变，并提供自己的实现，在执行阻塞 I/O 前，让出执行权，等到网络事件发生或超时，再恢复该协程的执行；

### 协程结构
```cpp
struct stCoRoutine_t {
  stCoRoutineEnv_t *env;
                         
  pfn_co_routine_t pfn;  
  void *arg;             
  coctx_t ctx;           

  char cStart;
  char cEnd;
  char cIsMain;
  char cEnableSysHook;
  char cIsShareStack;

  void *pvEnv;

  stStackMem_t *stack_mem;

  char *stack_sp;
  unsigned int save_size;
  char *save_buffer;

  stCoSpec_t aSpec[1024];
};
```

* env：环境，与线程绑定，同一个线程下的所有协程共享此环境
* pfn 和 arg：协程代理的入口函数和参数
* ctx：保存栈指针和各寄存器
* pvEnv：保存的环境变量
* stack_mem：协程栈
* aSpec：协程局部存储数据
* clsMain：是否是主协程
* cEnableSysHook：是否启用hook（替换阻塞接口）
* stack_sp、stack_size、stack_buffer： 共享栈使用
* cStart、cEnd：协程状态
* save_buffer：用于共享栈切换保存栈数据

### 协程运行huanj
```cpp
struct stCoRoutineEnv_t {
  stCoRoutine_t *pCallStack[128];
  int iCallStackSize;
  stCoEpoll_t *pEpoll;

  stCoRoutine_t *pending_co;
  stCoRoutine_t *occupy_co;
};
```
iCallStackSize 是当前正在运行的协程的下标+1，其实就是pCallStack最后一个协程
pending_co 和 occupy_co 由共享栈使用


### 共享栈
```cpp
struct stShareStack_t {
  unsigned int alloc_idx;
  int stack_size;
  int count;
  stStackMem_t **stack_array; 
};

struct stStackMem_t {
	stCoRoutine_t *occupy_co; 
	int stack_size;
	char *stack_bp; 
	char *stack_buffer;
};
```
* stack_array 表示 count 个栈大小为 stack_size 的共享栈
* alloc_idx 由共享栈分配时使用，用于取模
* occupy_co 是当前正在使用该共享栈的进程

#### 保存共享栈
```cpp
// pending_co 待执行的协程
env->pending_co = pending_co;
// occupy_co 当前 pending_co 想用的共享栈 但正在使用的协程
stCoRoutine_t *occupy_co = pending_co->stack_mem->occupy_co;
pending_co->stack_mem->occupy_co = pending_co;
env->occupy_co = occupy_co;

if (occupy_co && occupy_co != pending_co) {
  // occupy_co 存在，需要保存 occupy_co 的栈内存
  save_stack_buffer(occupy_co);
}

// 保存共享栈
void save_stack_buffer(stCoRoutine_t *occupy_co) {
  /// copy out
  stStackMem_t *stack_mem = occupy_co->stack_mem;
  int len = stack_mem->stack_bp - occupy_co->stack_sp;

  if (occupy_co->save_buffer) {
    free(occupy_co->save_buffer);
    occupy_co->save_buffer = NULL;
  }

  // 将共享栈数据保存至 save_buffer 中
  occupy_co->save_buffer = (char *)malloc(len); // malloc buf;
  occupy_co->save_size = len;

  memcpy(occupy_co->save_buffer, occupy_co->stack_sp, len);
}
```

#### 从共享栈恢复
```cpp
stCoRoutineEnv_t *curr_env = co_get_curr_thread_env();
stCoRoutine_t *update_occupy_co = curr_env->occupy_co;
stCoRoutine_t *update_pending_co = curr_env->pending_co;

if (update_occupy_co && update_pending_co &&
  update_occupy_co != update_pending_co) {
	if (update_pending_co->save_buffer && 
		update_pending_co->save_size > 0) {
		memcpy(update_pending_co->stack_sp, 
			   update_pending_co->save_buffer,
			   update_pending_co->save_size);
	}
}
```