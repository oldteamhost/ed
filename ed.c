/*
 * q
 * Q
 * f [file]
 * e [file]
 * E [file]
 * (.,.)p
 * (.,.)n
 * u
 */

#include <stdio.h>
#include <stdnoreturn.h>
#include <ctype.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define INPUTMODE 1
#define CMDMODE 0

#define twochar(c1,c2) \
	putchar(c1), putchar(c2)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define Q_CMD	0
#define U_CMD	1
#define _Q_CMD 2
#define E_CMD	3
#define F_CMD	4
#define _E_CMD 5
#define P_CMD	6
#define N_CMD	7

/* main */
char mainbuf[65535];
int mode=CMDMODE;
u8 changeflag=0;
size_t lastline=0;
size_t curline=0;

/* cmd */
char cmdin[1024];
char lastfile[1024]={0}; /* efE */
int cmdcode;
size_t x,y; /* range */
u8 err;

/* buffer */
char template[]="/tmp/tempfileXXXXXX";
int tmpfd=-1; /* ls -lt /tmp */

inline static void opentmp(void);
inline static void closetmp(void);

inline static void init(void);
inline static void exec(void);
inline static void cmd(void);
inline static int loop(void);
inline static noreturn void quit(int code);

inline static void qcmd_exec(void);
inline static void ucmd_exec(void);
inline static void fcmd_exec(void);
inline static u8 ecmd_exec(void);
inline static u8 pcmd_exec(u8 number);

inline static void opentmp(void)
{
	tmpfd=mkstemp(template);
}
inline static void closetmp(void)
{
	close(tmpfd);
	unlink(template);
	tmpfd=-1;
}


inline static u8 ecmd_exec(void)
{
	ssize_t r=0,tot=0,i=0;
	char buf[1024];
	int o=0;

	if (changeflag) {
		changeflag=0;
		return 0;
	}
	lastline=0;
	if (tmpfd>=0)
		closetmp();
	opentmp();
	o=open(lastfile,O_RDONLY);
	if (o<0)
		return 0;
	for (;(r=read(o, buf, sizeof(buf)))>0;) {
		write(tmpfd, buf, r), tot+=r;
		for (i=0;i<r;i++)
			if (buf[i]=='\n') lastline++;
	}
	close(o);
	printf("%ld [%ld lines]\n", tot, lastline);
	curline=lastline;
	return 1;
}

inline static u8 pcmd_exec(u8 number)
{
	char buf[65535];
	ssize_t i,r,l;
	size_t cur;
	u8 flag=0;
	cur=0;
	l=0;

	if (tmpfd<0)
		return 0;
	if (y>lastline||x>y)
		return 0;
	lseek(tmpfd, 0, SEEK_SET); /* to start */
	while ((r=read(tmpfd,buf,sizeof(buf)))>0) {
		l=0;
		for (i=0;i<r;i++) {
			if (buf[i]==0x0a) {
				buf[i]='\0';
				cur++;
				if (cur>=x)
					flag=1;
				if (flag)
					if (number)
						printf("%ld\t%s\n",cur,buf+l);
					else
						printf("%s\n",buf+l);
				l=i+1;
				if (cur==y)
					goto exit;
			}
		}
	}
exit:
	curline=y;
	return 1;
}

inline static void fcmd_exec(void)
{
	printf("%s\n",lastfile);
}

inline static void ucmd_exec(void)
{
}

inline static void qcmd_exec(void)
{
	if (changeflag) {
		changeflag=0;
		return;
	}
	puts("goodbye");
	quit(0);
}

inline static void init(void)
{
	memset(cmdin,0,sizeof(cmdin));
	cmdcode=-1;
	err=0;
	x=y=0;
}

inline static void exec(void)
{
	if (err)
		goto err;
	switch(cmdcode) {
		case Q_CMD:
			qcmd_exec();
			break;
		case U_CMD:
			ucmd_exec();
			break;
		case _Q_CMD:
			changeflag=0, qcmd_exec();
			break;
		case F_CMD:
			fcmd_exec();
			break;
		case E_CMD:
			if (!ecmd_exec())
				goto err;
			break;
		case P_CMD:
			if (!pcmd_exec(0))
				goto err;
			break;
		case N_CMD:
			if (!pcmd_exec(1))
				goto err;
			break;
	}
	return;
err:
	twochar('?',0x0a),err=0;
}


inline static void cmd(void)
{
	char num1[1024],num2[1024];
	size_t l,i,m,p,j;
	u8 flag, op,minus,plus;
	u8 stopflag=0;
	u8 del=0;

	memset(num1,0,sizeof(num1));
	memset(num2,0,sizeof(num2));
	l=strlen(cmdin), cmdin[--l]='\0';

	/* null command */
	if (!l) {
		x=curline+1;
		y=x;
		op='p';
		cmdcode=P_CMD;
		return;
	}
	/* <cmd> [opt] */
	if (isalpha(cmdin[0])) {
		op=cmdin[0];
		x=0;
		y=0;
		switch(op) {
			case 'q': cmdcode=Q_CMD; return;
			case 'u': cmdcode=U_CMD; return;
			case 'Q': cmdcode=_Q_CMD; return;
			case 'e': cmdcode=E_CMD; break;
			case 'f': cmdcode=F_CMD; break;
			case 'E': cmdcode=_E_CMD; break;
			default: goto err;
		}
		if (!strlen(lastfile)&&!(l-1))
			goto err;
		if ((l-1))
			memset(lastfile,0,sizeof(lastfile)),
				memcpy(lastfile,(cmdin+2),(l-2));
		return;
	}
	/* [[x][,|;][y]]<cmd> */
	if (cmdin[0]=='$'||cmdin[0]=='.'||cmdin[0]==','||cmdin[0]==';'||isdigit(cmdin[0])) {

		/* ARG1 */
		size_t anum,snum,pos;
		u8 anumflag,snumflag;
		for (i=0;i<l;i++) {
			num1[i]=cmdin[i];
			if (cmdin[i]==';'||cmdin[i]==',')
				break;
		}
		num1[i]='\0';
		pos=i;
		del=cmdin[pos];
L1:
		anumflag=snumflag=0;
		anum=snum=0;
		for (j=i=0;i<strlen(num1);i++) {
			if (num1[i]=='-'||num1[i]=='+') {
				if (j>0) {
					num2[j]='\0';
					if (snumflag)
						snum+=atoll(num2);
					else if (anumflag)
						anum+=atoll(num2);
					j=0;
					memset(num2,0,sizeof(num2));
				}
			}
			if (num1[i]=='-') {
				snumflag=1;
				anumflag=0;
				continue;
			}
			else if (num1[i]=='+') {
				anumflag=1;
				snumflag=0;
				continue;
			}
			if (snumflag||anumflag)
				num2[j++]=num1[i];
		}
		if (j>0) {
			num2[j]='\0';
			if (snumflag)
				snum+=atoll(num2);
			else if (anumflag)
				anum+=atoll(num2);
			memset(num2,0,sizeof(num2));
		}
		for (j=i=0;i<strlen(num1);i++) {
			if (num1[i]=='-'||num1[i]=='+')
				break;
			num2[j++]=num1[i];
		}
		num2[j]='\0';
		if (stopflag) {
			stopflag=0;
			goto L2;
		}
		if (num2[0]=='.')
			x=(curline-snum)+anum;
		else if (num2[0]=='$')
			x=(lastline-snum)+anum;
		else if (num2[0]==',')
			x=(1-snum)+anum;
		else if (num2[0]==';')
			x=(curline-snum)+anum;
		else
			x=(atoll(num2)-snum)+anum;


		/* ARG 2 */
		memset(num1,0,sizeof(num1));
		j=i=0;
		++pos;	/* skip , */
		for (i=pos;i<l;i++) {
			num1[j++]=cmdin[i];
			if (isalpha(cmdin[i]))
				break;
		}
		num1[j]='\0';
		pos=i;
		stopflag=1;
		goto L1;
L2:
	printf("%ld and %ld\n",x,y);
		if (num2[0]=='.')
			y=(curline-snum)+anum;
		else if (num2[0]=='$')
			y=(lastline-snum)+anum;
		else if (num2[0]==','||num2[0]==';')
			y=(x-snum)+anum;
		else
			y=(atoll(num2)-snum)+anum;
		if (!strlen(num2)-1) {
			if (del==','||del==';')
				y=x;
		}
L0:
		switch(op) {	
			case 'p': cmdcode=P_CMD; return;	
			case 'n': cmdcode=N_CMD; return;	
		}	
		goto err;	
	}

err:
	err=1;
exit:
	return;
}

inline static int loop(void)
{
	for (;;) {
		init();
		if (fgets(cmdin,sizeof(cmdin),stdin))
			cmd();
		exec(), fflush(stdout);
	}
}

inline static noreturn void quit(int code)
{
	if (tmpfd>=0)
		closetmp();
	exit(0);
}

int main(int argc, char **argv)
{
	signal(SIGINT, quit);
	return loop();
}
