


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>


#define GUID		"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


enum {
	WS_HANDSHARK = 0,
	WS_TRANMISSION = 1,
	WS_END = 2,
	WS_COUNT
};


struct ws_ophdr {

	unsigned char opcode:4,
				rsv3:1,
				rsv2:1,
				rsv1:1,
				fin:1;

	unsigned char pl_len:7,
	 			  mask:1;

};


#define BUFFER_LENGTH		1024
#define ACCEPT_KEY_LENGTH		64

#define MAX_EPOLL_EVENTS	1024
#define SERVER_PORT			8888
#define PORT_COUNT			500

typedef int NCALLBACK(int ,int, void*);

struct ntyevent {
	int fd;
	int events;
	void *arg;
	int (*callback)(int fd, int events, void *arg);
	
	int status;
	char buffer[BUFFER_LENGTH]; //request
	int length;
	//long last_active;

	char wbuffer[BUFFER_LENGTH]; //response
	int wlength;
	
	char sec_accept[ACCEPT_KEY_LENGTH];

	int wsstatus; //0, 1, 2, 3
};

struct eventblock {
	
	struct eventblock *next;
	struct ntyevent *events;
};

struct ntyreactor {
	int epfd;
	int blkcnt;

	struct eventblock *evblks;
};


int recv_cb(int fd, int events, void *arg);
int send_cb(int fd, int events, void *arg);
struct ntyevent *ntyreactor_idx(struct ntyreactor *reactor, int sockfd);



void nty_event_set(struct ntyevent *ev, int fd, NCALLBACK callback, void *arg) {

	ev->fd = fd;
	ev->callback = callback;
	ev->events = 0;
	ev->arg = arg;
	//ev->last_active = time(NULL);

	return ;
	
}


int nty_event_add(int epfd, int events, struct ntyevent *ev) {

	struct epoll_event ep_ev = {0, {0}};
	ep_ev.data.ptr = ev;
	ep_ev.events = ev->events = events;

	int op;
	if (ev->status == 1) {
		op = EPOLL_CTL_MOD;
	} else {
		op = EPOLL_CTL_ADD;
		ev->status = 1;
	}

	if (epoll_ctl(epfd, op, ev->fd, &ep_ev) < 0) {
		printf("event add failed [fd=%d], events[%d]\n", ev->fd, events);
		return -1;
	}

	return 0;
}

int nty_event_del(int epfd, struct ntyevent *ev) {

	struct epoll_event ep_ev = {0, {0}};

	if (ev->status != 1) {
		return -1;
	}

	ep_ev.data.ptr = ev;
	ev->status = 0;
	epoll_ctl(epfd, EPOLL_CTL_DEL, ev->fd, &ep_ev);

	return 0;
}

// 

int readline(char* allbuf,int idx,char* linebuf) {    
	int len = strlen(allbuf);    

	for (;idx < len; ++idx)    {        
		if(allbuf[idx]=='\r' && allbuf[idx+1]=='\n')            
			return idx+2;        
		else            
			*(linebuf++) = allbuf[idx];    
	}    

	return -1;
}


int base64_encode(char *in_str, int in_len, char *out_str) {    
	BIO *b64, *bio;    
	BUF_MEM *bptr = NULL;    
	size_t size = 0;    

	if (in_str == NULL || out_str == NULL)        
		return -1;    

	b64 = BIO_new(BIO_f_base64());    
	bio = BIO_new(BIO_s_mem());    
	bio = BIO_push(b64, bio);
	
	BIO_write(bio, in_str, in_len);    
	BIO_flush(bio);    

	BIO_get_mem_ptr(bio, &bptr);    
	memcpy(out_str, bptr->data, bptr->length);    
	out_str[bptr->length-1] = '\0';    
	size = bptr->length;    

	BIO_free_all(bio);    
	return size;
}


// request

int ws_handshark(struct ntyevent *ev) {

	
	int idx = 0;
	char sec_data[128] = {0};
	char sec_accept[128] = {0};

	do {
		char linebuf[1024] = {0};
		idx = readline(ev->buffer, idx, linebuf);

		if (strstr(linebuf, "Sec-WebSocket-Key")) {
		
			strcat(linebuf, GUID);
			SHA1(linebuf+19, strlen(linebuf+19), sec_data);
			base64_encode(sec_data, strlen(sec_data), sec_accept);		

			printf("idx: %d, line: %ld\n",idx, sizeof("Sec-WebSocket-Key: "));
			memcpy(ev->sec_accept, sec_accept, ACCEPT_KEY_LENGTH);

		}

	} while((ev->buffer[idx] != '\r' || ev->buffer[idx+1] != '\n') && idx != -1);

}

void umask(char *payload, int length, char *mask_key) {

	int i = 0;

	for (i = 0;i < length;i ++) {
		payload[i] ^= mask_key[i%4];
	}

}



int ws_tranmission(struct ntyevent *ev) {

	struct ws_ophdr *hdr = (struct ws_ophdr *)ev->buffer;

	if (hdr->pl_len < 126) {

		unsigned char *payload = NULL;
		if (hdr->mask) {
			payload = ev->buffer + 6;

			umask(payload, hdr->pl_len, ev->buffer + 2);
		} else {
			payload = ev->buffer + 2;
		}

		printf("payload: %s\n", payload);

	} else if (hdr->pl_len == 126) {

	} else if (hdr->pl_len == 127)  {

	} else {
		//assert(0);
	}

}

int ws_request(struct ntyevent *ev) {

	if (ev->wsstatus == WS_HANDSHARK) {
		ws_handshark(ev);
		ev->wsstatus = WS_TRANMISSION;
	} else if (ev->wsstatus == WS_TRANMISSION) {
		ws_tranmission(ev);
	}
	
}


int ws_response(struct ntyevent *ev) {

	ev->wlength = sprintf(ev->wbuffer, "HTTP/1.1 101 Switching Protocols\r\n"
						"Upgrade: websocket\r\n"
						"Connection: Upgrade\r\n"
						"Sec-WebSocket-Accept: %s\r\n\r\n", ev->sec_accept);

	printf("response: %s\n", ev->wbuffer);
	return ev->wlength;
	
}

// connection

int recv_cb(int fd, int events, void *arg) {

	struct ntyreactor *reactor = (struct ntyreactor*)arg;
	struct ntyevent *ev = ntyreactor_idx(reactor, fd);

	if (ev == NULL) return -1;

	int len = recv(fd, ev->buffer, BUFFER_LENGTH, 0);
	nty_event_del(reactor->epfd, ev);

	//if (ev->buffer[])

	if (len > 0) {
		
		ev->length = len;
		ev->buffer[len] = '\0';

		ws_request(ev);

		//printf("recv [%d]:%s\n", fd, ev->buffer);

		nty_event_set(ev, fd, send_cb, reactor);
		nty_event_add(reactor->epfd, EPOLLOUT, ev);
		
		
	} else if (len == 0) {

		nty_event_del(reactor->epfd, ev);
		printf("recv_cb --> disconnect\n");
		close(ev->fd);
		 
	} else {

		if (errno == EAGAIN && errno == EWOULDBLOCK) { //
			
		} else if (errno == ECONNRESET){
			nty_event_del(reactor->epfd, ev);
			close(ev->fd);
		}
		printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
		
	}

	return len;
}


int send_cb(int fd, int events, void *arg) {

	struct ntyreactor *reactor = (struct ntyreactor*)arg;
	struct ntyevent *ev = ntyreactor_idx(reactor, fd);

	if (ev == NULL) return -1;

	ws_response(ev);
	

	int len = send(fd, ev->wbuffer, ev->wlength, 0);
	if (len > 0) {
		printf("send[fd=%d], [%d]%s\n", fd, len, ev->wbuffer);

		nty_event_del(reactor->epfd, ev);
		nty_event_set(ev, fd, recv_cb, reactor);
		nty_event_add(reactor->epfd, EPOLLIN, ev);
		
	} else {

		nty_event_del(reactor->epfd, ev);
		close(ev->fd);

		printf("send[fd=%d] error %s\n", fd, strerror(errno));

	}

	return len;
}

int accept_cb(int fd, int events, void *arg) {

	struct ntyreactor *reactor = (struct ntyreactor*)arg;
	if (reactor == NULL) return -1;

	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	int clientfd;

	if ((clientfd = accept(fd, (struct sockaddr*)&client_addr, &len)) == -1) {
		if (errno != EAGAIN && errno != EINTR) {
			
		}
		printf("accept: %s\n", strerror(errno));
		return -1;
	}

	
	int flag = 0;
	if ((flag = fcntl(clientfd, F_SETFL, O_NONBLOCK)) < 0) {
		printf("%s: fcntl nonblocking failed, %d\n", __func__, MAX_EPOLL_EVENTS);
		return -1;
	}

	struct ntyevent *event = ntyreactor_idx(reactor, clientfd);

	if (event == NULL) return -1;
		
	nty_event_set(event, clientfd, recv_cb, reactor);
	nty_event_add(reactor->epfd, EPOLLIN, event);

	

	printf("new connect [%s:%d], pos[%d]\n", 
		inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), clientfd);

	return 0;

}

int init_sock(short port) {

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(fd, F_SETFL, O_NONBLOCK);

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if (listen(fd, 20) < 0) {
		printf("listen failed : %s\n", strerror(errno));
		return -1;
	}

	printf("listen server port : %d\n", port);
	return fd;
}


int ntyreactor_alloc(struct ntyreactor *reactor) {

	if (reactor == NULL) return -1;
	if (reactor->evblks == NULL) return -1;
	
	struct eventblock *blk = reactor->evblks;

	while (blk->next != NULL) {
		blk = blk->next;
	}

	struct ntyevent* evs = (struct ntyevent*)malloc((MAX_EPOLL_EVENTS) * sizeof(struct ntyevent));
	if (evs == NULL) {
		printf("ntyreactor_alloc ntyevent failed\n");
		return -2;
	}
	memset(evs, 0, (MAX_EPOLL_EVENTS) * sizeof(struct ntyevent));

	struct eventblock *block = malloc(sizeof(struct eventblock));
	if (block == NULL) {
		printf("ntyreactor_alloc eventblock failed\n");
		return -3;
	}
	block->events = evs;
	block->next = NULL;

	blk->next = block;
	reactor->blkcnt ++;

	return 0;
}

struct ntyevent *ntyreactor_idx(struct ntyreactor *reactor, int sockfd) {

	if (reactor == NULL) return NULL;
	if (reactor->evblks == NULL) return NULL;

	int blkidx = sockfd / MAX_EPOLL_EVENTS;
	while (blkidx >= reactor->blkcnt) {
		ntyreactor_alloc(reactor);
	}

	int i = 0;
	struct eventblock *blk = reactor->evblks;
	while (i++ != blkidx && blk != NULL) {
		blk = blk->next;
	}

	return &blk->events[sockfd % MAX_EPOLL_EVENTS];
}


int ntyreactor_init(struct ntyreactor *reactor) {

	if (reactor == NULL) return -1;
	memset(reactor, 0, sizeof(struct ntyreactor));

	reactor->epfd = epoll_create(1);
	if (reactor->epfd <= 0) {
		printf("create epfd in %s err %s\n", __func__, strerror(errno));
		return -2;
	}

	struct ntyevent* evs = (struct ntyevent*)malloc((MAX_EPOLL_EVENTS) * sizeof(struct ntyevent));
	if (evs == NULL) {
		printf("create epfd in %s err %s\n", __func__, strerror(errno));
		close(reactor->epfd);
		return -3;
	}
	memset(evs, 0, (MAX_EPOLL_EVENTS) * sizeof(struct ntyevent));

	struct eventblock *block = malloc(sizeof(struct eventblock));
	if (block == NULL) {
		free(evs);
		close(reactor->epfd);
		return -3;
	}
	block->events = evs;
	block->next = NULL;

	reactor->evblks = block;
	reactor->blkcnt = 1;

	return 0;
}

int ntyreactor_destory(struct ntyreactor *reactor) {

	close(reactor->epfd);

	struct eventblock *blk = reactor->evblks;
	struct eventblock *blk_next;
	while (blk != NULL) {
		blk_next = blk->next;

		free(blk->events);
		free(blk);
		
		blk = blk_next;
	}

	return 0;
}



int ntyreactor_addlistener(struct ntyreactor *reactor, int sockfd, NCALLBACK *acceptor) {

	if (reactor == NULL) return -1;
	if (reactor->evblks == NULL) return -1;

	struct ntyevent *event = ntyreactor_idx(reactor, sockfd);
	if (event == NULL) return -1;

	nty_event_set(event, sockfd, acceptor, reactor);
	nty_event_add(reactor->epfd, EPOLLIN, event);

	return 0;
}



int ntyreactor_run(struct ntyreactor *reactor) {
	if (reactor == NULL) return -1;
	if (reactor->epfd < 0) return -1;
	if (reactor->evblks == NULL) return -1;
	
	struct epoll_event events[MAX_EPOLL_EVENTS+1];
	
	int checkpos = 0, i;

	while (1) {

		int nready = epoll_wait(reactor->epfd, events, MAX_EPOLL_EVENTS, 1000);
		if (nready < 0) {
			printf("epoll_wait error, exit\n");
			continue;
		}

		for (i = 0;i < nready;i ++) {

			struct ntyevent *ev = (struct ntyevent*)events[i].data.ptr;

			if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {
				ev->callback(ev->fd, events[i].events, ev->arg);
			}
			if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {
				ev->callback(ev->fd, events[i].events, ev->arg);
			}
			
		}

	}
}


int main(int argc, char *argv[]) {

	
	struct ntyreactor *reactor = (struct ntyreactor*)malloc(sizeof(struct ntyreactor));
	ntyreactor_init(reactor);

	unsigned short port = SERVER_PORT;
	if (argc == 2) {
		port = atoi(argv[1]);
	}

	int i = 0;
	int sockfds[PORT_COUNT] = {0};
	
	//construct a listener
	for (i = 0;i < PORT_COUNT;i ++) {
		sockfds[i] = init_sock(port+i);
		ntyreactor_addlistener(reactor, sockfds[i], accept_cb);
	}


	ntyreactor_run(reactor);

	ntyreactor_destory(reactor);
	
	for (i = 0;i < PORT_COUNT;i ++) {
		close(sockfds[i]);
	}
	free(reactor);
	

	return 0;
}



