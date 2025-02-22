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
#define _Q_CMD	2
#define E_CMD	3
#define F_CMD	4
#define _E_CMD	5
#define P_CMD	6
#define N_CMD	7
#define L_CMD	8
#define EQ_CMD	9
#define W_CMD	10
#define D_CMD	11

char		mainbuf[65535];
int		mode=CMDMODE;
u8		changeflag=0;
size_t		lastline=0;
size_t		curline=0;
char		cmdin[2048];
char		lastfile[1024]={0};
int		cmdcode;
size_t		x,y;
u8		err;
char		param[2048];
int		tmpfd=-1;
char		template[256];

inline static void opentmp(void)
{
	snprintf(template,sizeof(template),"/tmp/tempfileXXXXXX");
	tmpfd=mkstemp(template);
}

inline static void closetmp(void)
{
	close(tmpfd);
	unlink(template);
	tmpfd=-1;
}

inline static noreturn void quit(int code)
{
	if (tmpfd>=0)
		closetmp();
	exit(0);
}

inline static u8 writefile(void)
{
	ssize_t	r,i,cur,w,l,tot;
	char	buf[65535];
	u8	flag;
	int	fd;

	r=i=cur=w=l=tot=0;
	flag = 0;

	if (tmpfd<0)
		return 0;
	if (y>lastline||x>lastline||x>y||!x||!y)
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
	changeflag=0;
	close(fd);
	return 1;
}


inline static u8 edit(void)
{
	ssize_t	r=0,tot=0,i=0;
	char	buf[1024];
	int	o;

	if (changeflag) {
		changeflag=0;
		return 0;
	}
	lastline=0;
	if (tmpfd>=0)
		closetmp();
	opentmp();
	if (tmpfd==-1)
		return 0;
	if ((o=open(lastfile,O_RDONLY))<0)
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

inline static u8 delete(void)
{
	char	tp[]="/tmp/mytmpfileXXXXXX";
	int	fd;
	char	buf[65535];
	ssize_t	r,i,j,l;
	u8	flag;

	flag=0;
	j=i=l=r=0;

	if (tmpfd<0)
		return 0;
	if (y>lastline||x>lastline||x>y||!x||!y)
		return 0;
	if ((fd=mkstemp(tp))<0)
		return 0;
	lseek(tmpfd,0,SEEK_SET);
	for (;(r=read(tmpfd,buf,sizeof(buf)))>0;) {
		l=0;
		for (i=0;i<r;i++) {
			if (buf[i]!=0x0a)
				continue;
			buf[i]='\0';
			j++;
			flag=(j==x)?1:flag;
			flag=(j>y)?0:flag;
			if (!flag) {
				if ((write(fd,buf+l,strlen(buf+l)))==-1) {
					close(fd);
					return 0;
				}
				u8 t[1]={0x0a};
				write(fd,t,1);
			}
			l=i+1;
		}
	}

	closetmp();
	rename(tp,template); /* aee */
	tmpfd=fd;

	/* fix current and last linea */
	changeflag=1;
	curline=x;
	curline=(x!=1&&y==lastline)?x-1:curline;
	curline=(x==1)?1:curline;
	if (lastline>y)
		lastline-=(y-x+1);
	else if (lastline>=x&&lastline<=y)
		lastline=x-1;
	lastline=(lastline<1)?0:lastline;
	curline=(!lastline)?lastline:curline;	/* all file deleted */

	return 1;
}

inline static u8 linenum(void)
{
	if (x>lastline||!x)
		return 0;
	printf("%ld\n",x);
	return 1;
}

inline static u8 print(u8 number, u8 list)
{
	char	buf[65535];
	ssize_t	i,r,l;
	size_t	cur,j;
	u8	flag;

	flag=0;
	cur=0;
	l=0;
	j=0;

	if (tmpfd<0)
		return 0;
	if (y>lastline||x>lastline||x>y||!x||!y)
		return 0;
	lseek(tmpfd, 0, SEEK_SET); /* to start */
	while ((r=read(tmpfd,buf,sizeof(buf)))>0) {
		l=0;
		for (i=0;i<r;i++) {
			if (buf[i]==0x0a) {
				buf[i]='\0';
				cur++;
				flag=(cur>=x)?1:flag;
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

inline static void filename(void)
{
	printf("%s\n",lastfile);
}

inline static void undo(void)
{
}

inline static void quited(void)
{
	if (changeflag) {
		changeflag=0;
		return;
	}
	puts("goodbye");
	quit(0);
}


inline static void exec(void)
{
	if (err)
		goto err;
	switch(cmdcode) {
		case Q_CMD:
			quited();
			break;
		case U_CMD:
			undo();
			break;
		case _Q_CMD:
			changeflag=0, quited();
			break;
		case F_CMD:
			filename();
			break;
		case E_CMD:
			if (!edit())
				goto err;
			break;
		case P_CMD:
			if (!print(0,0))
				goto err;
			break;
		case N_CMD:
			if (!print(1,0))
				goto err;
			break;
		case L_CMD:
			if (!print(0,1))
				goto err;
			break;
		case EQ_CMD:
			if (!linenum())
				goto err;
			break;
		case W_CMD:
			if (!writefile())
				goto err;
			break;
		case D_CMD:
			if (!delete())
				goto err;
			break;
	}
	return;
err:
	twochar('?',0x0a),err=0;
}


inline static void commands(void)
{
	char	a[2048];
	char	b[2048];
	size_t	l,i,j,m;
	u8	op;
	u8	stopflag;
	u8	del;
	u8	nullx;
	size_t	anum;
	size_t	snum;
	size_t	pos;

	memset(param,0,sizeof(param));
	cmdcode=-1;
	err=0;
	x=y=0;

	memset(a,0,sizeof(a));
	memset(b,0,sizeof(b));
	l=strlen(cmdin), cmdin[--l]='\0';
	del=stopflag=nullx=op=0;

	/* null command */
	if (!l) {
		x=curline+1;
		y=x;
		op='p';
		cmdcode=P_CMD;
		goto exit;
	}

	/* [[x][,|;][y]]<cmd>[ ][param] */
	if (cmdin[0]=='$'||cmdin[0]=='.'||cmdin[0]==','||
		cmdin[0]==';'||isdigit(cmdin[0])||
		isalpha(cmdin[0])) {

		/* no args */
		if (isalpha(cmdin[0])) {
			op=cmdin[0];
			goto L0;
		}

		/* get x */
		for (i=0;i<l&&(a[i]=cmdin[i])!=';'&&cmdin[i]!=',';i++);
		a[i]='\0',pos=i,del=cmdin[pos];
L1:
		/* expression parsing 100-1-1-4+4+43 */
		for (m=anum=snum=j=i=0;i<strlen(a);i++) {
			if (a[i]=='-'||a[i]=='+') {
				for (++i,j=i;j<strlen(a)&&(a[j]!='+'&&a[j]!='-');j++);
				if (a[i-1]=='-')
					snum+=atoll(a+i);
				else
					anum+=atoll(a+i);
				i=j-1;
			}
			else
				b[m++]=a[i];
		}
		b[m]='\0';

		/* arg2? */
		if (stopflag) {
			stopflag=0;
			goto L2;
		}

		/* processing arg1 */
		switch (b[0]) {
			case ';': case '.': x=curline; break;
			case '$': x=lastline; break;
			case ',': x=1; break;
			default: x=atoll(b); break;
		}
		x-=snum,x+=anum;
		if (!strlen(b)) {
			switch (del) {
				case ',': x=1; break;
				case ';': x=curline; break;
			}
			nullx=1;
		}

		/* get arg2 */
		memset(a,0,sizeof(a)),j=i=0,++pos /* skip ;/, */;
		for (i=pos;i<l&&!isalpha(cmdin[i]);i++)
  			a[j++]=cmdin[i];
		if (i<l)
  			a[j++]=cmdin[i];
		a[j]='\0',pos=i,stopflag=1;
		
		/* get op */
		if (isalpha(a[j-1])||a[j-1]=='=')
			op=a[j-1];
		goto L1; /* goto exp parse */
L2:
		/* processing */
		switch (b[0]) {
			case '.': y=curline; break;
			case '$': y=lastline; break;
			case ';': case ',': y=x; break;
			default: y=atoll(b); break;
		}
		y-=snum,y+=anum;
		if (!(strlen(a)-1)||!strlen(a)) {
			if (op) {
				if (del==','||del==';')
					y=x;
				if (nullx&&del==',')
					x=1,y=lastline;
				else if (nullx&&del==';')
					x=curline,y=lastline;
			}
		}
		if (del==0&&!op) {
			for (y=x,i=0;i<l&&!isalpha(cmdin[i])&&cmdin[i]!='=';i++);
			op=cmdin[i];
		}

		/* only num = p */
		if (!op) {
			op='p';
			if (!y)
				y=x;
			else
				x=y;
		}

L0:
		/* param */
		memset(a,0,sizeof(a));
		for (i=0;i<l;i++) { if (isalpha(cmdin[i])&&(i+1)!=l) { stopflag=1; break; } }
		for (i++,j=0;i<l&&stopflag;i++) param[j++]=cmdin[i];

		/* get cmdcode */
		switch(op) { 
			case 'p': cmdcode=P_CMD; goto exit;
			case 'n': cmdcode=N_CMD; goto exit;
			case 'l': cmdcode=L_CMD; goto exit;
			case '=': cmdcode=EQ_CMD; goto exit;
			case 'w': cmdcode=W_CMD; goto L4;
			case 'q': cmdcode=Q_CMD; goto exit;
			case 'u': cmdcode=U_CMD; goto exit;
			case 'Q': cmdcode=_Q_CMD; goto exit;
			case 'e': cmdcode=E_CMD; goto L4;
			case 'E': cmdcode=_E_CMD; goto L4;
			case 'f': cmdcode=F_CMD; goto L4;
			case 'd': cmdcode=D_CMD; goto exit;
		}
		goto err;
	}
L4:
	if (!strlen(lastfile)&&!(strlen(param)-1))
		goto err;
	if ((strlen(param)))
		memset(lastfile,0,sizeof(lastfile)),
			memcpy(lastfile,param+1,strlen(param)-1);
	goto exit;

err:
	err=1;
exit:
	/* default x,y */
	if (!x&&!y) {
		switch (op) {
			case 'p': case 'd': case 'n':
			case 'l':
				x=curline,y=x;
				break;
			case 'w':
				x=1,y=lastline;
				break;
			case '=':
				x=lastline,y=x;
				break;
		}
	}
#if 0
	printf("x=%ld, y=%ld, op=%c\n",x,y,op);
	printf("%s\n",param);
	printf("file=%s\n",lastfile);
#endif
	return;
}


int main(int argc, char **argv)
{
	signal(SIGINT, quit);
	for (;;) {
		memset(cmdin,0,sizeof(cmdin));
		fgets(cmdin,sizeof(cmdin),stdin);
		commands();
		exec();
		fflush(stdout);
	}
	return 0;
}
