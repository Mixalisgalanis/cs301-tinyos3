
#include "tinyos.h"
#define FCB_SOCKETS 1

Fid_t sys_Socket(port_t port)
{
	if(port<0 || port>MAX_PORT){
		return NOFILE;
	}

	FCB* fcb;
	Fid_t fid;

	if(FCB_reserve(1,&fid,&fcb)==1){
		SCB* scb = (SCB*)xmalloc(sizeof(SCB));

		scb->fcb=fcb;
		scb->fid=fid;
		scb->type=-1;
		scb->port=port;
		scb->fcb->streamobj=scb;
		scb->fcb->streamfunc=&socket_fops;
		return fid;


	}else{
		return NOFILE;
	}

}

int sys_Listen(Fid_t sock)
{
	return -1;
}


Fid_t sys_Accept(Fid_t lsock)
{
	return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}
