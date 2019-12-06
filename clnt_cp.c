#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include <termios.h>
#include <curses.h>
#include <fcntl.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define BLANK "                                             "
#define LEFT 75
#define RIGHT 77

void *send_msg(void *arg);
void *recv_msg(void *arg);
void *send_game(void *arg);
void *recv_game(void *arg);

void set_nodelay_mode();
void error_handling(char *msg);
void set_cr_noecho_mode();
void set_cr_echo_mode();
void draw();
void redraw();
void start(int);
void game(int);
void end(int);

char flag=0;
char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
int chat_row = 30;
WINDOW* win;
int main(int argc, char *argv[])
{
	int chat_sock, game_sock;
	struct sockaddr_in serv_chat_addr, serv_game_addr;
	pthread_t snd_thread, rcv_thread;
	pthread_t snd_game_thread, rcv_game_thread;
	void *thread_return;
	if(argc!=4)
	{
		printf("Usage : %s <IP> <port> <name>\n",argv[0]);
		exit(1);
	}
	sprintf(name,"[%s]",argv[3]);

	chat_sock=socket(PF_INET,SOCK_STREAM,0);
	memset(&serv_chat_addr,0,sizeof(serv_chat_addr));
	serv_chat_addr.sin_family=AF_INET;
	serv_chat_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_chat_addr.sin_port=htons(atoi(argv[2]));

        game_sock=socket(PF_INET,SOCK_STREAM,0);
        memset(&serv_game_addr,0,sizeof(serv_game_addr));
        serv_game_addr.sin_family=AF_INET;
        serv_game_addr.sin_addr.s_addr=inet_addr(argv[1]);
        serv_game_addr.sin_port=htons(atoi(argv[2])+100);

	printf("%x %x\n", serv_chat_addr.sin_port, serv_game_addr.sin_port);


	if(connect(chat_sock,(struct sockaddr*)&serv_chat_addr,sizeof(serv_chat_addr))==-1)
		error_handling("chat connect() error");

	if(connect(game_sock,(struct sockaddr*)&serv_game_addr, sizeof(serv_game_addr)) == -1)
		error_handling("game connect() error");

	initscr();
	clear();
	win =newwin(43,60,0,0);
	wborder(win, '*','*','*','*','*','*','*','*');
	cbreak();
	draw();
	wrefresh(win);
	set_cr_noecho_mode();
	pthread_create(&snd_thread,NULL,send_msg,(void*)&chat_sock);
	pthread_create(&rcv_thread,NULL,recv_msg,(void*)&chat_sock);
	pthread_create(&snd_game_thread,NULL,send_game,(void*)&game_sock);
	pthread_create(&rcv_game_thread, NULL, recv_game,(void*)&game_sock);
	noecho();
	pthread_join(snd_game_thread, &thread_return);
	pthread_join(rcv_game_thread, &thread_return);
	pthread_join(snd_thread,&thread_return);
	pthread_join(rcv_thread,&thread_return);
	close(chat_sock);
	close(game_sock);
	set_cr_echo_mode();
	return 0;
}

void* send_game(void *arg){
	int sock=*((int*)arg);
	write(sock, name, strlen(name));
	while(1){
		if(flag == 'R' || flag == 'r'){
			write(sock, "r", 1);
			break;
		}
	}
	while(1){
		if(flag==32){
			write(sock, "h\0", 2);
			flag =1;
		}
		else if(flag =='e'||flag=='E'){
			write(sock,"e",1);
			mvwprintw(win,28,1,"Wait other player....");
			wrefresh(win);
			flag =1;
		}
	}
}

void* recv_game(void *arg){
	int sock=*((int*)arg);
		start(sock);
		game(sock);
		end(sock);
}

void *send_msg(void *arg)
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	while(1)
	{
		flag = wgetch(win);
		if(flag== 10){
			echo();
			move(41, 1);
			mvwprintw(win,41,1,name);
			wrefresh(win);
			wgetstr(win,msg);
			mvwprintw(win,41,1,BLANK);
			wrefresh(win);
			if(!strcmp(msg,"q\0")||!strcmp(msg,"Q\0"))
			{
				set_cr_echo_mode();
				close(sock);
				exit(0);
			}
			sprintf(name_msg,"%s %s",name,msg);
			write(sock,name_msg,strlen(name_msg));
		}
		fflush(stdin);
		noecho();
	}
	return NULL;
}
void *recv_msg(void *arg)
{
	int sock=*((int*)arg);
	int i=0;
	int j, start=0;
	char name_msg[100][NAME_SIZE+BUF_SIZE];
	int str_len;
	int system_row=0;
	int count=0;
	while(1)
	{
		str_len= read(sock, name_msg[i], NAME_SIZE+BUF_SIZE-1);
		name_msg[i][str_len]=0;
		if(str_len == -1)
			return(void*)-1;
		chat_row = 30;
		for(j=start; j<start+10; j++){
			mvwprintw(win,chat_row,1,BLANK);
			mvwprintw(win,chat_row,1,name_msg[j]);
			chat_row++;
		}
		wrefresh(win);
		i++;
		if(i>9)
			start++;
	}
	return NULL;
}
void error_handling(char *msg)
{
	fputs(msg,stderr);
	fputc('\n',stderr);
	exit(1);
}

void set_nodelay_mode(){
	int termflags;
	termflags = fcntl(0,F_GETFL);
	termflags |= O_NDELAY;
	fcntl(0,F_SETFL, termflags);
}

void set_cr_noecho_mode(void){
        struct termios ttystate;

        tcgetattr(0, &ttystate);
        ttystate.c_lflag &= ~ICANON;
        ttystate.c_lflag &= ~ECHO;
        ttystate.c_cc[VMIN] = 1;
        tcsetattr(0, TCSANOW, &ttystate);
}

void set_cr_echo_mode(void){
	struct termios ttystate;

        tcgetattr(0, &ttystate);
        ttystate.c_lflag |= ICANON;
        ttystate.c_lflag |= ECHO;
        ttystate.c_cc[VMIN] = 1;
        tcsetattr(0, TCSANOW, &ttystate);
}

void draw(){
	mvwprintw(win,4,15,"Welcome to BLACKJACK!!!");
	mvwprintw(win,27,10,"If you want ready, Press <r>");
	mvwprintw(win,29,1,"------------------------chat room------------------------");
	move(41,0);
	wrefresh(win);
}

void start(int sock){
	char msg[BUF_SIZE];
	int row=10;
	int str_len;
	while((str_len = read(sock, msg, sizeof(msg))) != 0){
		if(!strncmp(msg,"end\0",4)){
			memset(msg,0,sizeof(msg));
			break;
		}
		msg[str_len] =0;
		move(row, 10);
		addstr(BLANK);
		mvwprintw(win,row,1,BLANK);
		mvwprintw(win,row,10,msg);
		row +=4;
		if(row > 22){
			row = 10;
			wrefresh(win);
		}
	}
}

void game(int sock){
	int str_len;
	int pos =5;
	int card = 2;
	int test=0;
	char gmsg[BUF_SIZE];
	
	char buf[100];

	redraw();

	while((str_len = read(sock, gmsg, sizeof(gmsg))) != 0){
		if(!strncmp(gmsg,"end\0",4))
			break;
		gmsg[str_len] = 0;
		mvwprintw(win,pos,25,gmsg);
		pos++;
	}
	memset(gmsg,0,sizeof(gmsg));
	pos = 11;
	while(1){
		str_len = read(sock, gmsg, sizeof(gmsg));
		if(!strncmp(gmsg,"end\0",4)){
			memset(gmsg,0,sizeof(gmsg));
			break;
		}
		else if(!strncmp(gmsg,"next\0",5)){
			pos = 11;
			card += 15;
			if(card >47){
				card = 2;
			}
			memset(gmsg,0,sizeof(gmsg));
			continue;
		}
		gmsg[str_len] = 0;
		move(pos, card);
		mvwprintw(win,pos,card,"          ");
		mvwprintw(win,pos,card,gmsg);
		wrefresh(win);
		pos++;
		memset(gmsg,0,sizeof(gmsg));
	}
}

void end(int sock){
	int str_len;
	str_len = read(sock,msg, sizeof(msg));
	msg[str_len]=0;
	mvwprintw(win,15,3,msg);
        str_len = read(sock,msg, sizeof(msg));
        msg[str_len]=0;
	mvwprintw(win,15,18,msg);
        str_len = read(sock,msg, sizeof(msg));
        msg[str_len]=0;
	mvwprintw(win,15,33,msg);
        str_len = read(sock,msg, sizeof(msg));
        msg[str_len]=0;
	mvwprintw(win,15,48,msg);
	str_len = read(sock, msg, sizeof(msg));
	msg[str_len] =0 ;
	mvwprintw(win, 20,1, "HIDDEN : ");
	mvwprintw(win, 20, 10, msg);
	wrefresh(win);
}

void redraw(){
	int i=0;
	for(i=1; i<29; i++){
		mvwprintw(win,i,1, BLANK);
	}
	move(1,10);
	mvwprintw(win,1,1,"------------------------GAME START------------------------");
	move(3,20);
	mvwprintw(win,3,27,"Dealer");
	move(10,3);
	addstr("P1");
	mvwprintw(win,10,5,"P1");
	mvwprintw(win,10,20,"P2");
	mvwprintw(win,10,35,"P3");
	mvwprintw(win,10,50,"P4");
	mvwprintw(win,25,1,"<space> DRAW CARD");
	mvwprintw(win,26,1,"<E> WAIT OTHER PLAYER");
		wrefresh(win);
}











