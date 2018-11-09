
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "util.h"
#include "kernel_cc.h"
static Mutex kernel_mutex=MUTEX_INIT;

PTCB* searchPTCB(Tid_t tid){
	rlnode* temp = CURPROC->ptcbs.next;
	while(temp!=&(CURPROC->ptcbs)) {
		if((Tid_t)(temp->ptcb->main_thread) == tid)
			return temp->ptcb;
		else
			temp = temp->next;
	}
	return NULL;
}
/**
  @brief Create a new thread in the current process.
  */

	PTCB* search_ptcb(){
	  return CURPROC->ptcbs.prev->ptcb;
	}

	void start_thread(){
	  int exitval;

	  PTCB* ptcb=search_ptcb();
	  Task call = ptcb->task;
	  int argl = ptcb->argl;
	  void* args = ptcb->args;

	  exitval = call(argl,args);
	  sys_ThreadExit(exitval);
	}

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

	rlnode_init(&ptcb->list_node, ptcb);

	rlist_push_back(&CURPROC->ptcbs, &ptcb->list_node);
	ptcb->main_thread = spawn_thread(CURPROC, start_thread);
	wakeup(ptcb->main_thread);
	fprintf(stderr, "Thread Creation attemption\n");
	return (Tid_t) ptcb->main_thread;

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
	PTCB* ptcb=searchPTCB(tid);

	if(ptcb->detached == 1 || ptcb==NULL || CURTHREAD==ptcb->main_thread ){
		return -1 ;
	}else {
		ptcb->ref_count ++;
		while( ptcb->exited!=1 && ptcb->detached!=1){
			kernel_wait(&ptcb->cv_joined,SCHED_USER);
		}
		ptcb->ref_count--;
		if(exitval!=NULL){
			*exitval=ptcb->exitval;
		}
		if(ptcb->ref_count==0){
			rlist_remove(&ptcb->list_node);
			free(ptcb);
		}
		return 0;
	}
	return -1;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
	PTCB* ptcb=searchPTCB(tid);
	if(ptcb != NULL && ptcb->detached == 0 && ptcb->exited ==0){
		if(ptcb->ref_count > 0){
			ptcb->ref_count = 0;
			Cond_Broadcast(&ptcb->cv_joined);
		}
		ptcb->detached = 1;
		return 0;
	}
	return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{
	PTCB* ptcb= searchPTCB((Tid_t)CURTHREAD);

	CURTHREAD->state = EXITED;
	ptcb->exited = 1;
	ptcb->exitval = exitval;
	Cond_Broadcast(&ptcb->cv_joined);
	kernel_lock();
	sleep_releasing(EXITED,&kernel_mutex,SCHED_USER,0);
	kernel_unlock();
}

/*TCB* searchThread1(Tid_t tid){
		rlnode* temp = CURPROC->ptcbs.next;
		while(temp!=CURPROC->ptcbs) {
			if((Tid_t)(temp->ptcb->main_thread) == tid)
				return temp->ptcb->main_thread;
			else
				temp = temp->next;
		}
		return NULL;
	}
*/
