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
#define L_CMD	8
#define EQ_CMD	9
#define W_CMD	10

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
char	param[2048];

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
inline static u8 pcmd_exec(u8 number, u8 list);

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

inline static u8 wcmd_exec(void)
{
	ssize_t r,i,cur,w,l,tot;
	char buf[65535];
	u8 flag;
	int fd;

	r=i=cur=w=l=tot=0;
	flag = 0;

	if (tmpfd<0)
		return 0;
	if (y>lastline||x>y)
		return 0;
	if ((fd=open(lastfile,O_WRONLY|O_CREAT|O_TRUNC, 0644))==-1)
		return 0;
	lseek(tmpfd, 0, SEEK_SET);
	for (;(r=read(tmpfd, buf, sizeof(buf)))>0;) {
		l=0;
		for (i=0;i<r;i++) {
			if (buf[i]!=0x0a)
				continue;
			buf[i]='\0';
			cur++;
			flag=(cur>=x)?1:flag;
			if (flag) {
				 if ((w=write(fd,buf+l,strlen(buf+l)))==-1) {
					close(fd);
					return 0;
				}
				u8 tmp[1]={'\n'};
				write(fd,tmp,1);
				tot+=w+1;
			}
			l=i+1;
			if (cur==y)
				goto exit;
		}
	}

exit:
	printf("%ld [%ld lines]\n",tot,cur);
	close(fd);
	return 1;
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

inline static u8 eqcmd_exec(void)
{
	if (x>lastline)
		return 0;
	printf("%ld\n",x);
	return 1;
}

inline static u8 pcmd_exec(u8 number, u8 list)
{
	char buf[65535];
	ssize_t i,r,l;
	size_t cur,j;
	u8 flag=0;
	cur=0;
	l=0;
	j=0;

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
				if (flag) {
					if (number)
						printf("%ld\t",cur);
					if (!list)
						printf("%s\n",buf+l);
					else {
						for (j=0;j<strlen(buf+l);j++) {
							switch (buf[j+l]) {
								case '\\': twochar('\\','\\'); break;
								case '\a': twochar('\\','a'); break;
								case '\b': twochar('\\','b'); break;
								case '\f': twochar('\\','f'); break;
								case '\r': twochar('\\','r'); break;
								case '\t': twochar('\\','t'); break;
								case '\v': twochar('\\','v'); break;
								case '$': twochar('\\','$'); break;
								default: putchar(buf[j+l]); break;
							}
							if ((j+1)==strlen(buf+l))
								twochar('$',0x0a);
						}
					}
				}
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
	memset(param,0,sizeof(param));
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
			if (!pcmd_exec(0,0))
				goto err;
			break;
		case N_CMD:
			if (!pcmd_exec(1,0))
				goto err;
			break;
		case L_CMD:
			if (!pcmd_exec(0,1))
				goto err;
			break;
		case EQ_CMD:
			if (!eqcmd_exec())
				goto err;
			break;
		case W_CMD:
			if (!wcmd_exec())
				goto err;
			break;
	}
	return;
err:
	twochar('?',0x0a),err=0;
}


inline static void cmd(void)
{
	char	num1[2048];
	char	num2[2048];
	size_t	l,i,j;
	u8	op;
	u8	stopflag;
	u8	del;
	u8	nullx;
	size_t	anum;
	size_t	snum;
	size_t	pos;
	u8	anumflag;
	u8	snumflag;

	del=stopflag=nullx=op=0;

	memset(num1,0,sizeof(num1));
	memset(num2,0,sizeof(num2));
	l=strlen(cmdin), cmdin[--l]='\0';

	/* null command */
	if (!l) {
		x=curline+1;
		y=x;
		op='p';
		cmdcode=P_CMD;
		goto exit;
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
		goto exit;
	}
	/* [[x][,|;][y]]<cmd>[ ][param] */
	if (cmdin[0]=='$'||cmdin[0]=='.'||cmdin[0]==','||
		cmdin[0]==';'||isdigit(cmdin[0])) {

		for (i=0;i<l;i++) {
			num1[i]=cmdin[i];
			if (cmdin[i]==';'||cmdin[i]==',')
				break;
		}
		num1[i]='\0',pos=i,del=cmdin[pos];
L1:
		/* expression parsing -4-3+4+3 */
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

		/* get num from arg1 */
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

		/* processing */
		switch (num2[0]) {
			case ';': case '.': x=curline; break;
			case '$': x=lastline; break;
			case ',': x=1; break;
			default: x=atoll(num2); break;
		}
		x-=snum,x+=anum;
		if (!strlen(num2)) {
			if (del==',')
				x=1;
			if (del==';')
				x=curline;
			nullx=1;
		}


		/* nxt arg */
		memset(num1,0,sizeof(num1)),j=i=0,++pos /* skip ;/, */;
		for (i=pos;i<l;i++) {
			num1[j++]=cmdin[i];
			if (isalpha(cmdin[i]))
				break;
		}
		num1[j]='\0',pos=i,stopflag=1;
		if (isalpha(num1[j-1])||num1[j-1]=='=')
			op=num1[j-1];
		goto L1;
L2:
		/* processing */
		switch (num2[0]) {
			case '.': y=curline; break;
			case '$': y=lastline; break;
			case ';': case ',': y=x; break;
			default: y=atoll(num2); break;
		}
		y-=snum,y+=anum;
		if (!(strlen(num1)-1)||!strlen(num1)) {
			if (op) {
				if (del==','||del==';')
					y=x;
				if (nullx&&del==',')
					x=1,y=lastline;
				else if (nullx&&del==';')
					x=curline,y=lastline;
				else if (del==0) {
					y=x;
					for (i=0;i<l;i++)
						if (isalpha(cmdin[i])||cmdin[i]=='=')
							break;
					op=cmdin[i];
				}
			}
		}

		/* param */
		memset(num1,0,sizeof(num1));
		for (i=0;i<l;i++) { if (isalpha(cmdin[i])&&(i+1)!=l) { stopflag=1; break; } }
		for (i++,j=0;i<l&&stopflag;i++) param[j++]=cmdin[i];

		/* only num */
		if (!op) {
			op='p';
			if (!y)
				y=x;
			else
				x=y;
		}

		/* get cmdcode */
		switch(op) { 
			case 'p': cmdcode=P_CMD; goto exit;
			case 'n': cmdcode=N_CMD; goto exit;
			case 'l': cmdcode=L_CMD; goto exit;
			case '=': cmdcode=EQ_CMD; goto exit;
			case 'w': cmdcode=W_CMD; goto exit;
		}
		goto err;
	}

err:
	err=1;
exit:
#if 1
	printf("x=%ld, y=%ld, op=%c\n",x,y,op);
	printf("%s\n",param);
#endif
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
	return 0;
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
