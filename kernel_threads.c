
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"

/**
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
	PTCB *ptcb = xmalloc(sizeof(ptcb));
	ptcb->task=task;
	ptcb->argl=argl;
	ptcb->args=args;
	ptcb-> exitval=0;
	ptcb->exited=0;
	ptcb->detached=0;
	ptcb->cv_joined=COND_INIT;	
	ptcb->ref_count=0;

	rlnode_init(&ptcb->node, ptcb);

	rlist_push_back(&CURPROC->ptcbs, &ptcb->list_node);
	ptcb->main_thread = spawn_thread(CURPROC, start_thread);
	wakeup(ptcb->tcb);
	return (Tid_t) ptcb;

}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) CURTHREAD;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
	PTCB* curptcb = curproc->ptcbs;
	while(curptcb != (PTCB*) tid)
			curptcb = curptcb->  //exei minei miso
	return -1;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
	return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{

}
