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
#define BLANK "                                                    "
#define LEFT 75
#define RIGHT 77

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
int main(int argc, char *argv[])
{
	int chat_sock, game_sock;
	struct sockaddr_in serv_game_addr;
	pthread_t snd_game_thread, rcv_game_thread;
	void *thread_return;
	if(argc!=4)
	{
		printf("Usage : %s <IP> <port> <name>\n",argv[0]);
		exit(1);
	}
	sprintf(name,"[%s]",argv[3]);

        game_sock=socket(PF_INET,SOCK_STREAM,0);
        memset(&serv_game_addr,0,sizeof(serv_game_addr));
        serv_game_addr.sin_family=AF_INET;
        serv_game_addr.sin_addr.s_addr=inet_addr(argv[1]);
        serv_game_addr.sin_port=htons(atoi(argv[2])+100);


	if(connect(game_sock,(struct sockaddr*)&serv_game_addr, sizeof(serv_game_addr)) == -1)
		error_handling("game connect() error");

	initscr();
	clear();
	set_cr_noecho_mode();

	pthread_create(&snd_game_thread,NULL,send_game,(void*)&game_sock);
	pthread_create(&rcv_game_thread, NULL, recv_game,(void*)&game_sock);
	noecho();
	pthread_join(snd_game_thread, &thread_return);
	pthread_join(rcv_game_thread, &thread_return);
	close(chat_sock);
	close(game_sock);
	set_cr_echo_mode();
	return 0;
}

void* send_game(void *arg){
	int sock=*((int*)arg);
	write(sock, name, strlen(name));
	char flag;
	while(1){
		flag = getch();
		if(flag == 'R' || flag == 'r'){
			write(sock, "r", 1);
			break;
		}
	}
	while(1){
		flag = getch();
		if(flag==32){
			write(sock, "h\0", 2);
			flag =1;
		}
		else if(flag =='e'||flag=='E'){
			write(sock,"e",1);
			move(28,0);
			addstr("Wait other player....");
			refresh();
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
	move(1,0);
	addstr("Welcome to BlackJack Game!!!!");
	move(25,0);
	addstr("If you want ready, Press <r>");
	move(29,0);
	addstr("--------------------chat room--------------------");
	move(41,0);
	refresh();
}

void start(int sock){
	char msg[BUF_SIZE];
	int row=4;
	int str_len;
	while((str_len = read(sock, msg, sizeof(msg))) != 0){
		if(!strncmp(msg,"end\0",4)){
			memset(msg,0,sizeof(msg));
			break;
		}
		msg[str_len] =0;
		move(row, 0);
		addstr(BLANK);
		move(row, 0);
		addstr(msg);
		refresh();
		row +=4;
		if(row > 16)
			row = 4;
	}
}

void game(int sock){
	int str_len;
	int pos =4;
	int card = 0;
	int test=0;
	char gmsg[BUF_SIZE];
	
	char buf[100];

	redraw();

	while((str_len = read(sock, gmsg, sizeof(gmsg))) != 0){
		if(!strncmp(gmsg,"end\0",4))
			break;
		gmsg[str_len] = 0;
		move(pos,20);
		addstr(gmsg);
		refresh();
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
			if(card >45){
				card = 0;
			}
			memset(gmsg,0,sizeof(gmsg));
			continue;
		}
		gmsg[str_len] = 0;
		move(pos, card);
		addstr("          ");
		move(pos,card);
		addstr(gmsg);
		refresh();
		pos++;
		memset(gmsg,0,sizeof(gmsg));
	}
}

void end(int sock){
	int str_len;
	str_len = read(sock,msg, sizeof(msg));
	msg[str_len]=0;
	move(15,3);
	addstr(msg);
	refresh();
        str_len = read(sock,msg, sizeof(msg));
        msg[str_len]=0;
        move(15,18);
        addstr(msg);
	refresh();
        str_len = read(sock,msg, sizeof(msg));
        msg[str_len]=0;
        move(15,33);
        addstr(msg);
	refresh();
        str_len = read(sock,msg, sizeof(msg));
        msg[str_len]=0;
        move(15,48);
        addstr(msg);
	refresh();
	str_len = read(sock, msg, sizeof(msg));
	msg[str_len] =0 ;
	move(20,0);
	addstr("Hidden : ");
	addstr(msg);
	refresh();
}

void redraw(){
	int i=0;
	for(i=0; i<29; i++){
		move(i,0);
		addstr(BLANK);
	}
	move(1,0);
	addstr("--------------------GAME START--------------------");
	move(3,20);
	addstr("Dealer");
	move(10,3);
	addstr("P1");
	move(10,18);
	addstr("P2");
	move(10,33);
	addstr("P3");
	move(10,48);
	addstr("P4");
	move(25,0);
	addstr("If you want more card, Press <space>");
	move(26,0);
	addstr("If you want stop, Press <e>");
	refresh();
}
