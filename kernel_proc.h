#ifndef __KERNEL_PROC_H
#define __KERNEL_PROC_H
#define max_of_ptcb 1024
/**
  @file kernel_proc.h
  @brief The process table and process management.

  @defgroup proc Processes
  @ingroup kernel
  @brief The process table and process management.

  This file defines the PCB structure and basic helpers for
  process access.

  @{
*/

#include "kernel_sched.h"
#include "tinyos.h"

/**
  @brief PID state

  A PID can be either free (no process is using it), ALIVE (some running process
  is using it), or ZOMBIE (a zombie process is using it).
  */
typedef enum pid_state_e {
  FREE,  /**< The PID is free and available */
  ALIVE, /**< The PID is given to a process */
  ZOMBIE /**< The PID is held by a zombie */
} pid_state;

/**
  @brief Process Control Block.

  This structure holds all information pertaining to a process.
 */
typedef struct process_control_block {
  pid_state pstate; /**< The pid state for this PCB */

  rlnode ptcbs; /*List with ptcbs*/

  PCB *parent; /**< Parent's pcb. */
  int exitval; /**< The exit value */

  TCB *main_thread; /**< The main thread */
  Task main_task;   /**< The main thread's function */
  int argl;         /**< The main thread's argument length */
  void *args;       /**< The main thread's argument string */

  rlnode children_list; /**< List of children */
  rlnode exited_list;   /**< List of exited children */

  rlnode children_node; /**< Intrusive node for @c children_list */
  rlnode exited_node;   /**< Intrusive node for @c exited_list */
  CondVar child_exit;   /**< Condition variable for @c WaitChild */

  FCB *FIDT[MAX_FILEID]; /**< The fileid table of the process */

} PCB;

typedef enum Detach_state { DETACH, UNDETACH } detach_state;

typedef enum Exit_state { EXITED_STATE, NOTEXITED } Exit_state;

typedef struct p_thread_control_block { //orisame to block tou ptcb
  // Basic Structure
  PCB *pcb;         /*PCB Owner*/
  TCB *main_thread; /*Main Thread*/
  Tid_t tid;        /*ID of Thread*/
  Task task;        /*Main Function of Thread*/
  rlnode list_node; /* PTCB node to be inserted in ptcbs list*/

  // Arguements
  int argl;   /* argument lenght*/
  void *args; /* argument string*/

  // Status flags
  int detached;      /*Won't let other threads join if this flag is enabled*/
  int exited;        /*Thread is finished if this flag is enabled*/
  int exitval;       /*Exit value*/
  int ref_count;     /*Show how many threads have joined this thread*/
  CondVar cv_joined; /*Condition variable for WaitChild*/

} PTCB;

typedef struct proc_info_control_block {
//ti prepei na periexei?

//mia structure proc info
//ena deikti poy na leei mexri poy exw diabasei apto PT

  procinfo pi;
  PCB* current_pcb;


} PICB;




/**
  @brief Initialize the process table.

  This function is called during kernel initialization, to initialize
  any data structures related to process creation.
*/
void initialize_processes();

/**
  @brief Get the PCB for a PID.

  This function will return a pointer to the PCB of
  the process with a given PID. If the PID does not
  correspond to a process, the function returns @c NULL.

  @param pid the pid of the process
  @returns A pointer to the PCB of the process, or NULL.
*/
PCB *get_pcb(Pid_t pid);

/**
  @brief Get the PID of a PCB.

  This function will return the PID of the process
  whose PCB is pointed at by @c pcb. If the pcb does not
  correspond to a process, the function returns @c NOPROC.

  @param pcb the pcb of the process
  @returns the PID of the process, or NOPROC.
*/
Pid_t get_pid(PCB *pcb);

/** @} */

#endif
