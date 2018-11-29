#include "tinyos.h"
#include "kernel_streams.h"
#include <math.h>

#define FCB_PIPES 2

static file_ops reader_file_ops = {
	.Open = NULL,
	.Read = rpipe_read,
	.Write = NULL,
	.Close = rpipe_close
} ;

static file_ops writer_file_ops = {
	.Open = NULL,
	.Read = NULL,
	.Write = wpipe_write,
	.Close = wpipe_close
} ;

int sys_Pipe(pipe_t* pipe)
{
	PIPECB* pipecb=(PIPECB*)xmalloc(sizeof(PIPECB));

	FCB* fcb[FCB_PIPES];
	Fid_t fid[FCB_PIPES];

	for(int i=0; i<FCB_PIPES; i++)
		fid[i]=NULL;

	switch(FCB_reserve(FCB_PIPES, fid, fcb)){
		case 1:
			pipe->read=fid[0];
			pipe->write=fid[1];

			pipecb->reader=fcb[0];
			pipecb->writer=fcb[1];

			pipecb->cv_reader=COND_INIT;
			pipecb->cv_writer=COND_INIT;

			pipecb->w=0;
			pipecb->r=0;
			pipecb->w2=0;
			pipecb->r2=0;

			pipecb->reader->streamobj=pipecb;
			pipecb->writer->streamobj=pipecb;

			pipecb->reader->streamfunc=&reader_file_ops;
			pipecb->writer->streamfunc=&writer_file_ops;

			return 0;
		default:
			return -1;
	}


}

int rpipe_read(void* pipe, const char *buf, unsigned int size){

	PIPECB* pipecb=(PIPECB*)pipe;

	uint read_bytes = pipecb->w - pipe->r;	//o arithmos twn bytes pou prepei na diavasw
	uint read_size = size;
	int step=0;
	while(read_bytes==0){	//an den exw grapsei kati ston buffer perimenw mexri na grapsei o writer
		Cond_Broadcast(&pipecb->cv_writer);
		kernel_wait(&pipecb->cv_reader);
		read_bytes = pipecb->w - pipe->r;
	}
	int max;
	while (read_bytes > 0 && read_size > 0) {	//i deyteri sinthiki einai gia an to read_bytes<read_size
		/* code */
		max = (read_size <= read_bytes)? read_size:read_bytes;	//epilegw ti megisti timi me tavani to read_size
		for (int  i = 0; i < max; i++) {
			/* code */
			if (pipecb->r2 >= BUF_SIZE)
				pipecb->r2 = 0;

			buf[i+step] = pipecb->BUFFER[pipecb->r2];	//gt yparxei i periptwsi o writer na vazei px 60-60 ta chars ston buffer
			pipecb->r++;							    //epomemws thelw na xerw se poio simeio tou buffer vriskomai gt panwgrafei
														//afou prwta gemisei
			pipecb->r2++;
		}
		Cond_Broadcast(&pipecb->cv_writer);
		read_size -= max - 1;
		step += max - 1;
		read_bytes = pipecb->w - pipe->r;	//gia na dw an egrapse xana o writer

	}

	return size-read_size;

}

int wpipe_write(void* pipe, const char *buf, unsigned int size){
	PIPECB* pipecb=(PIPECB*)pipe;

	uint read_bytes = BUF_SIZE-(pipecb->w - pipe->r);
	uint read_size =size;
	int step=0;
	while(read_bytes==0){
		Cond_Broadcast(&pipecb->cv_reader);
		kernel_wait(&pipecb->cv_writer);
		read_bytes =BUF_SIZE-(pipecb->w - pipe->r);
	}
	int max;
	while (read_bytes > 0 && read_size > 0) {
		/* code */
		max = (read_size <= read_bytes)? read_size:read_bytes;
		for (int  i = 0; i < max; i++) {
			/* code */
			if (pipecb->r2 >= BUF_SIZE)
				pipecb->r2 = 0;

			buf[i+step] = pipecb->BUFFER[pipecb->r2];
			pipecb->r++;
			pipecb->r2++;
		}
		Cond_Broadcast(&pipecb->cv_reader);
		read_size -= max - 1;
		step += max - 1;
		read_bytes = BUF_SIZE-(pipecb->w - pipe->r);

	}

	return size-read_size;


}
