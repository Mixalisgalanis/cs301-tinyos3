#include "tinyos.h"
#include "kernel_cc.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "util.h"

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
    }

    SCB *scb = (SCB *) xmalloc(sizeof(SCB));
    scb->fcb = fcb;
    scb->fid = fid;
    scb->type = UNBOUND;
    scb->port = port;
    scb->fcb->streamobj = scb;
    scb->fcb->streamfunc = &unbound_file_ops;
    return fid;
}

int sys_Listen(Fid_t sock) {
    FCB *fcb = get_fcb(sock);
    //Check if file_ID is legal
    if (fcb == NULL || fcb->streamobj == NULL)
        return -1;
    //Get corresponding SCB
    SCB *scb = fcb->streamobj;

    if (scb->port == NOPORT || scb->type != UNBOUND || port_map[scb->port] != NULL)
        return -1;

    scb->type = LISTENER;

    //Initializing Listener_control_block
    scb->listener_cb = (LICB *) xmalloc(sizeof(LICB));
    rlnode_init(&scb->listener_cb->queue, NULL);
    scb->listener_cb->req_available = COND_INIT;

    fcb->streamfunc = &listener_file_ops;

    port_map[scb->port] = scb;
    return 0;
}


Fid_t sys_Accept(Fid_t sock) {
    //Getting Listener SCB
    FCB *fcb = get_fcb(sock);
    if (fcb == NULL || fcb->streamobj == NULL)    //Check if file_ID is legal
        return NOFILE;
    SCB *listener_scb = fcb->streamobj;    //Get corresponding SCB
    if (listener_scb->type != LISTENER)
        return NOFILE;

    if (check(sock) == NULL) return NOFILE;

    if (rlist_len(&listener_scb->listener_cb->queue) == 0)
        kernel_wait(&listener_scb->listener_cb->req_available, SCHED_USER);

    Fid_t socket_fid = Socket(NOPORT);

    if (socket_fid == NOFILE || get_fcb(socket_fid) == NULL || get_fcb(socket_fid)->streamobj == NULL)
        return NOFILE;

    FCB *connection_fcb = get_fcb(socket_fid);
    SCB *end_side = connection_fcb->streamobj; //server

    rlnode *nd = rlist_pop_front(&listener_scb->listener_cb->queue);

    REQ *req = nd->req;

    SCB *start_side = req->scb; //client


    PIPECB *writer = (PIPECB *) xmalloc(sizeof(PIPECB));
    PIPECB *reader = (PIPECB *) xmalloc(sizeof(PIPECB));

    start_side->fcb->streamfunc = &peer_file_ops;
    end_side->fcb->streamfunc = &peer_file_ops;

    start_side->peer_cb->sender = writer;
    start_side->peer_cb->receiver = reader;
    start_side->type = PEER;
    end_side->peer_cb->sender = reader;
    end_side->peer_cb->receiver = writer;
    end_side->type = PEER;

    writer->w = 0;
    writer->r = 0;
    writer->w2 = 0;
    writer->r2 = 0;

    reader->w = 0;
    reader->r = 0;
    reader->w2 = 0;
    reader->r2 = 0;

    Cond_Broadcast(&req->request_cv);
    return socket_fid;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout) {

    FCB *fcb = get_fcb(sock);

    if (fcb == NULL || fcb->streamobj == NULL)
        return -1;

    SCB *scb = fcb->streamobj;

    if (scb->type != UNBOUND)
        return -1;

    SCB *lscb = port_map[port];

    if (lscb == NULL || lscb->type != LISTENER)
        return -1;

    REQ *req = (REQ *) xmalloc(sizeof(REQ));

    req->scb = scb;
    req->request_cv = COND_INIT;
    rlnode_init(&req->req_node, req);
    rlist_push_back(&lscb->listener_cb->queue, &req->req_node);
    Cond_Broadcast(&lscb->listener_cb->req_available);
    kernel_timedwait(&req->request_cv, SCHED_USER, timeout);

    return 0;
}


int sys_ShutDown(Fid_t sock, shutdown_mode mode) {

    FCB *fcb = get_fcb(sock);

    if (fcb == NULL || fcb->streamobj == NULL)
        return -1;

    SCB *scb = fcb->streamobj;

    if (scb->type != PEER)
        return -1;

    switch (mode) {
        case SHUTDOWN_READ:
            rpipe_close(&scb->peer_cb->receiver);
            break;
        case SHUTDOWN_WRITE:
            wpipe_close(&scb->peer_cb->sender);
            break;
        case SHUTDOWN_BOTH:
            rpipe_close(&scb->peer_cb->receiver);
            wpipe_close(&scb->peer_cb->sender);
            break;

    }
    return 0;
}

/*
File Ops Functions
*/
int peer_read(void *scb_arg, char *buf, unsigned int size) {
    SCB *scb = ((SCB *) scb_arg);
    return (scb->peer_cb->receiver == NULL) ? -1 : rpipe_read(&scb->peer_cb->receiver, buf, size);
}

int peer_write(void *scb_arg, const char *buf, unsigned int size) {
    SCB *scb = ((SCB *) scb_arg);
    return (scb->peer_cb->sender == NULL) ? -1 : wpipe_write(&scb->peer_cb->sender, buf, size);
}

int unbound_close(void *scb_arg) {
    free((SCB *) scb_arg);
    return 0;
}

int listener_close(void *scb_arg) {
    SCB *scb = ((SCB *) scb_arg);
    port_map[scb->port] = NULL;
    Cond_Signal(&scb->listener_cb->req_available);
    free(scb);
    return 0;
}

int peer_close(void *scb_arg) {
    SCB *scb = ((SCB *) scb_arg);
    if (scb->counter != 0)
        return -1;
    if (scb->peer_cb->sender != NULL)
        wpipe_close(scb->peer_cb->sender);
    if (scb->peer_cb->receiver != NULL)
        rpipe_close(scb->peer_cb->receiver);
    free(scb);
    return 0;
}