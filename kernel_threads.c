
#include "kernel_cc.h"
#include "kernel_proc.h"
#include "kernel_sched.h"
#include "tinyos.h"
#include "util.h"
static Mutex kernel_mutex = MUTEX_INIT;

/**
 @brief Finds and returns a PTCB in the current Process based on a given Thread
 ID
 */
PTCB *searchPTCB(Tid_t tid) {
  rlnode *temp = CURPROC->ptcbs.next; //to temp deixnei sto prwto stoixeio
  while (temp != &(CURPROC->ptcbs)) { //sigkrisi temp me to head tis listas
    if ((Tid_t)(temp->ptcb->main_thread) == tid)
      return temp->ptcb;
    else
      temp = temp->next;
  }
  return NULL;
}

/**
 @brief Initializes the argl,args arguements and calls the Thread's Main Func
 Task
 */
void start_thread() {
  PTCB *ptcb = searchPTCB((Tid_t)CURTHREAD); //evresi tou current ptcb
  int exitval;
  Task call = ptcb->task;
  int argl = ptcb->argl;
  void *args = ptcb->args;
  exitval = call(argl, args);

  ThreadExit(exitval);
}

/**
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void *args) {
  // Makes sure that there is a task
  if (task != NULL) {
    // Setting up PCB & PTCB
    PCB *pcb = CURPROC;                         /*Creates a local copy of PCB*/
    PTCB *ptcb = (PTCB *)xmalloc(sizeof(PTCB)); /*Allocates space for ptcb*/

    // Setting up flags of PTCB
    ptcb->pcb = pcb;                           /*Sets owner PCB of PTCB*/
    ptcb->argl = argl;                         /*Initializes argl flag*/
    ptcb->args = (args == NULL) ? NULL : args; /*Initializes args flag*/
    ptcb->ref_count = 0;                       /*Initializes ref_count counter*/
    ptcb->task = task;                         /*Initializes main task*/
    ptcb->detached = 0;                        /*Initializes detached flag*/
    ptcb->cv_joined = COND_INIT;
    ptcb->exited = 0; /*Initializes exited flag*/

    // Initializes PTCB node and pushes it back to the ptcbs list in PCB
    rlnode_init(&ptcb->list_node, ptcb); //to rlnode deixnei sto ptcb, next kai prev deixnoun ston eauto tou
    rlist_push_back(&CURPROC->ptcbs, &ptcb->list_node);

    // Starts & Initializes thread, enwsi me pcb
    TCB *tcb = spawn_thread(CURPROC, start_thread);
    ptcb->main_thread = tcb;
    ptcb->tid = (Tid_t)tcb;
    wakeup(ptcb->main_thread);

    return (Tid_t)tcb;
  }
  return NOTHREAD;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf() { return (Tid_t)CURTHREAD; }

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int *exitval) {
  //
  TCB *tcb = (TCB *)tid;

  if (tcb == NULL)
    return -1; // Thread Join Failed

  PTCB *ptcb = searchPTCB(tid);
  /*Makes sure the PTCB exists,
   the thread we are joining isn't detached and
   that we are not joining the self thread*/
  if (ptcb == NULL || ptcb->detached == 1 || (Tid_t)CURTHREAD == tid) {
    return -1; // Thread Join Failed
  } else {

    ptcb->ref_count++;

    while (ptcb->exited != 1 && ptcb->detached != 1) { //mexri to ptcb na ginei extied i detached
      kernel_wait(&ptcb->cv_joined, SCHED_USER);      //to thread tha einai joined
    }
    ptcb->ref_count--;

    if (exitval != NULL)
      *exitval = ptcb->exitval;

    if (ptcb->ref_count == 0) {
      rlist_remove(&ptcb->list_node);
      free(ptcb);
    }
  }

  return 0;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid) {

  PTCB *ptcb = searchPTCB(tid);

  fprintf(stderr, "%ld detach\n", ptcb->tid);

  if (ptcb != NULL || ptcb->exited != 1) {
    if (ptcb->ref_count > 0) { //an yparxoun thread pou exoun kanei join
      ptcb->ref_count = 0;
      Cond_Broadcast(&ptcb->cv_joined); //xipname ola ta nimata
    }
    ptcb->detached = 1;

    return 0;
  }

  return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval) {
  PTCB *ptcb = searchPTCB((Tid_t)CURTHREAD);

  ptcb->exited = 1;
  ptcb->exitval = exitval;

  Cond_Broadcast(&ptcb->cv_joined); //xipname ola ta nimata pou einai joined

  kernel_unlock();
  sleep_releasing(EXITED, &kernel_mutex, SCHED_USER, 0); //termatizei to quantum tou thread kai o scheduler pairnei to epomeno nimata
  kernel_lock();
}
