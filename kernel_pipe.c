#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_cc.h"
#include "kernel_dev.h"


#define FCB_PIPES 2

 file_ops reader_file_ops = {
	.Open = NULL,
	.Read = rpipe_read,
	.Write = NULL,
	.Close = rpipe_close
};

 file_ops writer_file_ops = {
	.Open = NULL,
	.Read = NULL,
	.Write = wpipe_write,
	.Close = wpipe_close
};

int sys_Pipe(pipe_t* pipe)
{
	PIPECB* pipecb=(PIPECB*)xmalloc(sizeof(PIPECB));

	FCB* fcb[FCB_PIPES];
	Fid_t fid[FCB_PIPES];

	for(int i=0; i<FCB_PIPES; i++)
		fcb[i]=NULL;

	if(FCB_reserve(FCB_PIPES, fid, fcb)==1){

			pipe->read=fid[0];
			pipe->write=fid[1];

			pipecb->rd=fid[0];
			pipecb->wt=fid[1];


			pipecb->reader=fcb[0];
			pipecb->writer=fcb[1];

			pipecb->cv_reader=COND_INIT;
			pipecb->cv_writer=COND_INIT;

			pipecb->pipe_mutex=MUTEX_INIT;

			pipecb->w=0;
			pipecb->r=0;
			pipecb->w2=0;
			pipecb->r2=0;

			pipecb->reader->streamobj=pipecb;
			pipecb->writer->streamobj=pipecb;

			pipecb->reader->streamfunc=&reader_file_ops;
			pipecb->writer->streamfunc=&writer_file_ops;

			return 0;
	}
return -1;

}

int rpipe_read(void* pipe, char *buf, unsigned int size){

	PIPECB* pipecb=(PIPECB*)pipe;

	int read_bytes = pipecb->w - pipecb->r;	//o arithmos twn bytes pou prepei na diavasw
	int read_size = size;
	int step=0;
	while(read_bytes==0){	//an den exw grapsei kati ston buffer perimenw mexri na grapsei o writer
		Cond_Broadcast(&pipecb->cv_writer);
		kernel_wait(&pipecb->cv_reader,SCHED_PIPE);
		read_bytes = pipecb->w - pipecb->r;
	}

	int max=0;
	while (read_bytes > 0 && read_size > 0) {	//i deyteri sinthiki einai gia an to read_bytes<read_size
		/* code */
		max = (read_size <= read_bytes) ? read_size : read_bytes;	//epilegw ti megisti timi me tavani to read_size
		for (int  i = 0; i < max; i++) {
			/* code */
			if (pipecb->r2 >= BUF_SIZE){
				pipecb->r2 = 0;
				//buf[i+step]= pipecb->BUFFER[pipecb->r2];
			}
			else{
			buf[i+step]= pipecb->BUFFER[pipecb->r2];	//gt yparxei i periptwsi o writer na vazei px 60-60 ta chars ston buffer
										    //epomemws thelw na xerw se poio simeio tou buffer vriskomai gt panwgrafei
		}
			pipecb->r++;									//afou prwta gemisei
			pipecb->r2++;
		}
		Cond_Broadcast(&pipecb->cv_writer);
		read_size =read_size -(max - 1);
		step = step + (max - 1);
		read_bytes = pipecb->w - pipecb->r;	//gia na dw an egrapse xana o writer

	}

	return size-read_size;

}

int wpipe_write(void* pipe, const char *buf, unsigned int size){
	PIPECB* pipecb=(PIPECB*)pipe;

	int write_bytes = BUF_SIZE-(pipecb->w - pipecb->r);
	int write_size = size;
	int step=0;
	while(write_bytes==0){
		Cond_Broadcast(&pipecb->cv_reader);
		kernel_wait(&pipecb->cv_writer,SCHED_PIPE);
		write_bytes =BUF_SIZE-(pipecb->w - pipecb->r);
	}
	int max=0;
	while (write_bytes > 0 && write_size > 0) {
		/* code */
		max = (write_size <= write_bytes) ? write_size : write_bytes;
		for (int  i = 0; i < max; i++) {
			/* code */
			if (pipecb->w2 >= BUF_SIZE){
				pipecb->w2 = 0;
			//pipecb->BUFFER[pipecb->w2]=buf[i+step];
}else{
	pipecb->BUFFER[pipecb->w2]=buf[i+step];
}
			pipecb->w++;
			pipecb->w2++;
		}
		Cond_Broadcast(&pipecb->cv_reader);
		write_size = write_size - (max - 1);
		step = step + (max - 1);
		write_bytes = BUF_SIZE-(pipecb->w - pipecb->r);

	}

	return size-write_size;
}

int  rpipe_close(void* pipe){

	PIPECB* pipecb=(PIPECB*)pipe;

	if(pipecb->reader->refcount==0){

		pipecb->reader=NULL;
		pipecb->rd=-1;

		if(pipecb->writer==NULL){
			free(pipecb);
			return 0;
		}else{
			Cond_Broadcast(&pipecb->cv_writer);
			return 0;
		}
	}
	return -1;
}

int wpipe_close(void* pipe){

	PIPECB* pipecb=(PIPECB*)pipe;

	if(pipecb->writer->refcount==0){

		pipecb->writer=NULL;
		pipecb->wt=-1;

		if(pipecb->reader==NULL){
			free(pipecb);
			return 0;
		}else{
			Cond_Broadcast(&pipecb->cv_reader);
			return 0;
		}
	}
	return -1 ;
}
