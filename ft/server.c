#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct cmsg {
  int cm_type;
  int cm_cblk;
  char cm_body[256];
};

struct smsg {
  int sm_type;
  int sm_nblk;
  char sm_body[1024];
};

int main(int argc, char ** argv)
{
  int sockfd, len, n, fd;
  struct sockaddr_in servaddr, cliaddr;

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Socket Error");
    exit(-1);
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(atoi(argv[1]));

  printf("Starting server at port %d\n", atoi(argv[1]));

  if((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0) {
    perror("Bind Error");
    exit(-1);
  }


  struct cmsg * recv_mesg = (struct cmsg*)malloc(sizeof(struct cmsg));
  struct smsg * send_mesg = (struct smsg*)malloc(sizeof(struct smsg));
  struct stat st;
  int nblk, lblk_sz;
  int lablk;

  while(1) {
    len = sizeof(cliaddr);
    n = recvfrom(sockfd, recv_mesg, sizeof(struct cmsg), 0, (struct sockaddr*)&cliaddr, (socklen_t*)&len);
    
    if(recv_mesg->cm_type == 0) {

      // client is requesting for a file
      printf("Client request for %s\n", recv_mesg->cm_body);

      if((fd = open(recv_mesg->cm_body, O_RDONLY)) < 0) {
        fprintf(stderr, "File %s couldn't be opened - bad filename or file not found\n", recv_mesg->cm_body);
        send_mesg->sm_type = -1;
      } else {
        if(fstat(fd, &st) < 0) {
          fprintf(stderr, "Information for file %s couldn't be retrieved\n", recv_mesg->cm_body);
          send_mesg->sm_type = -1;
        } else {
          // calculate number of blocks file will be transferred in
          nblk = (int)ceil((long double)st.st_size/1024);
          // calculate size of last block
          lblk_sz = (int)(((int)st.st_size) - 1024*(nblk-1));
          // transmit number of blocks and size of last block to client
          send_mesg->sm_type = 0;
          send_mesg->sm_nblk = nblk; 
          sprintf(send_mesg->sm_body, "%d", lblk_sz);
        }
      }
      // send number of blocks client should expect
      sendto(sockfd, send_mesg, sizeof(struct smsg), 0, (struct sockaddr*)&cliaddr, len);
      printf("Sending %s in %d blocks\n", recv_mesg->cm_body, nblk);

    } else if(recv_mesg->cm_type == 1) {

      if((lablk = recv_mesg->cm_cblk) == 0) {
        printf("Starting file transfer...\n");
      }

      if(recv_mesg->cm_cblk == nblk) {

        // client acknowledged receipt of last block
        printf("File sent succesfully.\n");
        close(fd);
        
        nblk = 0;
        lblk_sz = 0;

      } else {
        if((lseek(fd, 0, SEEK_CUR)/1024) != recv_mesg->cm_cblk) {
      
          // block to be read by server and block client is requesting do not match
          // this might be because the client wants to resume an interrupted download
          // jump to the requested block
          lseek(fd, (recv_mesg->cm_cblk*1024), SEEK_SET); 
          
        } 

        send_mesg->sm_type = 1;
        send_mesg->sm_nblk = recv_mesg->cm_cblk;
        read(fd, send_mesg->sm_body, 1024);
        sendto(sockfd, send_mesg, sizeof(struct smsg), 0, (struct sockaddr*)&cliaddr, len);

      }
/*
      if((lseek(fd, 0, SEEK_CUR)/1024) == recv_mesg->cm_cblk) {
        // client acknowledges receipt of previous block and is asking for next block
        send_mesg->sm_type = 1;
        read(fd, send_mesg->sm_body, 1024);
        sendto(sockfd, send_mesg, sizeof(struct smsg), 0, (struct sockaddr*)&cliaddr, len);
      } else {
        if(recv_mesg->cm_cblk == nblk) {
          // client acknowledges receipt of last block
          printf("File sent succesfully!\n");
          close(fd);
          nblk = 0;
          lblk_sz = 0;
        } else {
          // block to be read by server and block client is requesting do not match
          // this might be because client wants to resume an interrupted download
          // jump to the requested block and start serving from that point
          send_mesg->sm_type = 1;
          lseek(fd, (recv_mesg->cm_cblk*1024), SEEK_SET);
          read(fd, send_mesg->sm_body, 1024);
          sendto(sockfd, send_mesg, sizeof(struct smsg), 0, (struct sockaddr*)&cliaddr, len);
        }
      }
 */
    } else if(recv_mesg->cm_type == 2) {
      // unused message type
    } else if(recv_mesg->cm_type == -1) {

      printf("Client encountered fatal error, cancelling transfer\n");
      close(fd);
      nblk = 0;
      lblk_sz = 0;
    } else {
      printf("Bad request format\n");
    }
  }


  return 0;
}
