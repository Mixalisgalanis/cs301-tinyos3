#include "tinyos.h"
#include "kernel_cc.h"
#include "kernel_dev.h"
#include "kernel_streams.h"

#define FCB_SOCKETS 1

file_ops unbound_file_ops = {
        .Open = NULL,
        .Read = NULL,
        .Write = NULL,
        .Close = unbound_close
};

file_ops listener_file_ops = {
        .Open = NULL,
        .Read = NULL,
        .Write = NULL,
        .Close = listener_close
};

file_ops peer_file_ops = {
        .Open = NULL,
        .Read = peer_read,
        .Write = peer_write,
        .Close = peer_close
};


Fid_t sys_Socket(port_t port) {
    FCB *fcb;
    Fid_t fid;

    if (port < 0 || port > MAX_PORT || FCB_reserve(FCB_SOCKETS, &fid, &fcb) == 0) {
        return NOFILE;
    } else {
        SCB *scb = (SCB *) xmalloc(sizeof(SCB));

        scb->fcb = fcb;
        scb->fid = fid;
        scb->type = UNBOUND;
        scb->port = port;
        scb->fcb->streamobj = scb;
        scb->fcb->streamfunc = &unbound_file_ops;
        return fid;
    }
}

int sys_Listen(Fid_t sock) {

    FCB *fcb = get_fcb(sock);

    //Check if file_ID is legal
    if (fcb == NULL || fcb->streamobj == NULL) {
        return -1;
    }

    SCB *scb = fcb->streamobj;

    if (scb->port == NOPORT || scb->type != UNBOUND || port_map[scb->port] != NULL) {
        return -1;
    }

    scb->type = LISTENER;

    //Initializing Listener_control_block
    scb->listener_cb = (LICB *) xmalloc(sizeof(LICB));
    rlnode_init(&scb->listener_cb->queue, NULL);
    scb->listener_cb->req_available = COND_INIT;

    fcb->streamfunc = &listener_file_ops;


    //TODO Check whether there is a unique listening socket on each port (number of listening sockets <= 1)
    /*if (rlist_len(scb->listener_cb->queue) > 1){
        return -1;
    }*/

    port_map[scb->port] = scb;
    return 0;
}


Fid_t sys_Accept(Fid_t sock) {
    return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout) {
    return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how) {
    return -1;
}

/*
File Ops Functions
*/
int unbound_close(void *scb_arg) {
    free((SCB *) scb_arg);
    return 0;
}

int listener_close(void *scb_arg) {
    SCB* scb = ((SCB *) scb_arg);
    port_map[scb->port] = NULL;
    Cond_Signal(&scb->listener_cb->req_available);
    free(scb);
    return 0;
}

int peer_read(void *scb_arg, char *buf, unsigned int size) {
    SCB *scb = ((SCB *) scb_arg);

    if(scb->peer_cb->receiver==NULL){
        return -1;
    }else{
        return(rpipe_read(&scb->peer_cb->receiver, buf, size));
    }
}

int peer_write(void *scb_arg, const char *buf, unsigned int size) {
    SCB *scb = ((SCB *) scb_arg);

    if(scb->peer_cb->sender==NULL){
        return -1;
    }else{
        return(wpipe_write(&scb->peer_cb->sender, buf, size));
    }

}

int peer_close(void *scb_arg) {
    SCB *scb = ((SCB *) scb_arg);
    return 0;
}


