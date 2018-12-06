#include "kernel_cc.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "tinyos.h"

file_ops reader_file_ops = {
    .Open = NULL, .Read = rpipe_read, .Write = NULL, .Close = rpipe_close};

file_ops writer_file_ops = {
    .Open = NULL, .Read = NULL, .Write = wpipe_write, .Close = wpipe_close};

int sys_Pipe(pipe_t *pipe) {
  PIPECB *pipecb = (PIPECB *)xmalloc(sizeof(PIPECB));

  FCB *fcb[2];
  Fid_t fid[2];
  fcb[0] = NULL;
  fcb[1] = NULL;

  int check = FCB_reserve(2, fid, fcb);

  if (check == 1) {

    pipe->read = fid[0];
    pipe->write = fid[1];

    pipecb->rd = fid[0];
    pipecb->wt = fid[1];
    pipecb->reader = fcb[0];
    pipecb->writer = fcb[1];

    pipecb->cv_reader = COND_INIT;
    pipecb->cv_writer = COND_INIT;

    pipecb->reader->streamobj = pipecb;
    pipecb->writer->streamobj = pipecb;

    pipecb->r2 = 0;
    pipecb->w2 = 0;

    pipecb->r = 0;
    pipecb->w = 0;

    pipecb->reader->streamfunc = &reader_file_ops;
    pipecb->writer->streamfunc = &writer_file_ops;
    return 0;
  }
  return -1;
}

int rpipe_read(void *pipe, char *buf, unsigned int size) {
  PIPECB *pipecb = (PIPECB *)pipe;
  int read_size = size;
  int count = 0;
  int read_bytes = pipecb->w - pipecb->r;
  int step = 0;

  while (read_bytes == 0 && pipecb->writer != NULL) {

    Cond_Broadcast(&pipecb->cv_writer);
    kernel_wait(&pipecb->cv_reader, SCHED_PIPE);
    read_bytes = pipecb->w - pipecb->r;
  }

  int max = 0;
  while (read_size > 0 && read_bytes > 0) {

    max = (read_size <= read_bytes) ? read_size : read_bytes;

    for (int i = 0; i < max; i++) {
      /* code */
      if (pipecb->r2 >= BUF_SIZE)
        pipecb->r2 = 0;
      buf[i + step] = pipecb->buffer[pipecb->r2];
      pipecb->r++;
      pipecb->r2++;
      count++;
    }
    Cond_Broadcast(&pipecb->cv_writer);
    read_size -= (count);
    step += count;
    read_bytes = pipecb->w - pipecb->r;
    count = 0;
  }
  return size - read_size;
}

int wpipe_write(void *pipe, const char *buf, unsigned int size) {
  PIPECB *pipecb = (PIPECB *)pipe;
  int max = 0;
  int write_size = size;
  int count = 0;
  int write_bytes = BUF_SIZE - (pipecb->w - pipecb->r);
  int step = 0;

  if (pipecb->reader == NULL || pipecb->writer == NULL) {
    return -1;
  }

  while (write_bytes == 0) {
    Cond_Broadcast(&pipecb->cv_reader);
    kernel_wait(&pipecb->cv_writer, SCHED_PIPE);
    write_bytes = BUF_SIZE - (pipecb->w - pipecb->r);
  }
  while (write_size > 0 && write_bytes > 0) {

    max = (write_size <= write_bytes) ? write_size : write_bytes;

    for (int i = 0; i < max; i++) {

      if (pipecb->w2 < BUF_SIZE) {
        pipecb->buffer[pipecb->w2] = buf[i + step];
      } else {
        pipecb->w2 = 0;
      }
      pipecb->w++;
      pipecb->w2++;
      count++;
    }
    Cond_Broadcast(&pipecb->cv_reader);
    write_size -= count;
    step += count;
    write_bytes = BUF_SIZE - (pipecb->w - pipecb->r);
    count = 0;
  }
  return size - write_size;
}

int rpipe_close(void *pipe) {

  PIPECB *pipecb = (PIPECB *)pipe;

  if (pipecb->reader->refcount == 0) {

    pipecb->reader = NULL;
    pipecb->rd = -1;

    if (pipecb->writer == NULL) {
      free(pipecb);
      return 0;
    } else {
      Cond_Broadcast(&pipecb->cv_writer);
      return 0;
    }
  }
  return -1;
}

int wpipe_close(void *pipe) {

  PIPECB *pipecb = (PIPECB *)pipe;

  if (pipecb->writer->refcount == 0) {

    pipecb->writer = NULL;
    pipecb->wt = -1;

    if (pipecb->reader == NULL) {
      free(pipecb);
      return 0;
    } else {
      Cond_Broadcast(&pipecb->cv_reader);
      return 0;
    }
  }
  return -1;
}
