#include "tinyos.h"

#define FCB_PIPES 2
#define FCB_SOCKETS 1

file_ops reader_file_ops = {
	.Open = NULL,
	.Read = rpipe_read;
	.Write = NULL,
	.Close = rpipe_close
} RFO;

file_ops writer_file_ops = {
	.Open = NULL,
	.Read = NULL,
	.Write = wpipe_write,
	.Close = wpipe_close
} WFO;

int sys_Pipe(pipe_t* pipe)
{
	PIPECB* pipecb=(PIPECB*)xmalloc(sizeof(PIPECB));

	FCB* fcb[FCB_PIPES];
	Fid_t fid[FCB_PIPES];

	for(int i=0; i<FCB_PIPES; i++)
		fid[i]=NULL;

	switch(FCB_reserve(FCB_PIPES, fid, fcb)){
		case 1:



			return 0;
		default:
			return -1;
	}
}
