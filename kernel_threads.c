
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "util.h"
#include "kernel_cc.h"
static Mutex kernel_mutex= MUTEX_INIT;


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

void start_thread()
{
	PTCB* ptcb=searchPTCB((Tid_t)CURTHREAD);
	int exitval;
	Task call = ptcb->task;
	int argl=ptcb->argl;
	void* args =ptcb->args;
	exitval=call(argl,args);

	ThreadExit(exitval);
}



/**
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
		if(task!= NULL){
			PCB* pcb = CURPROC;
			PTCB* ptcb = (PTCB*) xmalloc(sizeof(PTCB));
			ptcb->pcb=pcb;
			ptcb->argl= argl;
			if(args==NULL){
		    ptcb->args=NULL;
		  }else{
		    ptcb->args =args;
		  }
			ptcb-> ref_count=0;
			ptcb -> task = task;

			ptcb -> detached =0;
			ptcb -> cv_joined = COND_INIT;
			ptcb->exited=0;
			assert(ptcb!= NULL);
			rlnode_init(&ptcb->list_node,ptcb);
			rlist_push_back(&CURPROC->ptcbs,&ptcb->list_node);



			TCB* tcb=spawn_thread(CURPROC,start_thread);
				ptcb->main_thread=tcb;
				ptcb -> tid=(Tid_t)tcb;
				assert(ptcb!=NULL);
			wakeup(ptcb->main_thread);
			//	fprintf(stderr, "active_ptcb\n" );
			return (Tid_t)tcb;
		}
	return NOTHREAD;
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
			TCB* tcb=(TCB*)tid;
			if(tcb==NULL)
				return -1;

			PTCB* ptcb= searchPTCB(tid);
			//assert(cur_id!=-1);
			if(ptcb==NULL ||ptcb->detached == 1 || (Tid_t)CURTHREAD==tid ){
				return -1 ;
			}else{

			ptcb->ref_count++;

			while(ptcb->exited!=1 && ptcb->detached != 1){
				kernel_wait(&ptcb->cv_joined,SCHED_USER);
			}
		ptcb->ref_count--;

			if(exitval!=NULL)
				*exitval=ptcb->exitval;

		if(ptcb->ref_count==0){
				rlist_remove(&ptcb->list_node);
				free(ptcb);
			}
			return 0;
}

	return 0;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{

	PTCB* ptcb=searchPTCB(tid);

		fprintf(stderr, "%ld detach\n",ptcb->tid );

	if(ptcb!=NULL || ptcb->exited==1){
			if(ptcb->ref_count>0){
				ptcb->ref_count=0;
					Cond_Broadcast(&ptcb->cv_joined);
			}
		ptcb->detached=1;

		return 0;
	}

	return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{
 			PTCB* ptcb=searchPTCB((Tid_t)CURTHREAD);
			assert(ptcb!=NULL);

		ptcb->exited=1;
	ptcb->exitval=exitval;

	Cond_Broadcast(&ptcb->cv_joined);

	kernel_unlock();
	sleep_releasing(EXITED,&kernel_mutex,SCHED_USER,0);
	kernel_lock();

}
