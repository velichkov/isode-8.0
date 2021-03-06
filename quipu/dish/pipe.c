/* pipe.c - */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/quipu/dish/RCS/pipe.c,v 9.0 1992/06/16 12:35:39 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/quipu/dish/RCS/pipe.c,v 9.0 1992/06/16 12:35:39 isode Rel $
 *
 *
 * $Log: pipe.c,v $
 * Revision 9.0  1992/06/16  12:35:39  isode
 * Release 8.0
 *
 */

/*
 *				  NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */


#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include "general.h"

#ifdef SOCKETS
#include "internet.h"
#endif
#include "sys.file.h"
#include <sys/stat.h>
#include "tailor.h"

#ifdef SOCKETS
int             sd, sd_current;
#else
char 		retpipe[LINESIZE];
int             fd, wfd;
#endif

#ifndef MIN
#define MIN(a,b) (( (b) < (a) ) ? (b) : (a) )
#endif

int parent_pid;
extern int	errno;

int
init_pipe (void) {
	char parent [BUFSIZ];
	char   *cp;

#ifdef SOCKETS
	struct sockaddr_in      sin_buf;
	struct sockaddr_in      *sin = &sin_buf;
#else
#endif

	if ((cp = getenv ("DISHPARENT")) == NULLCP) {
		sprintf (parent, "%d", getppid ());
		setenv ("DISHPARENT", cp = parent);
	}


	if (sscanf (cp, "%d", &parent_pid) != 1) {
		fprintf (stderr,"DISHPARENT malformed");
		return (NOTOK);
	}

#ifdef SOCKETS
	if (get_dish_sock (sin) != 0)
		return (NOTOK);

	if ((sd = start_tcp_server (sin, SOMAXCONN, 0, 0)) == NOTOK) {
		perror ("start_tcp_server");
		return NOTOK;
	}
#ifdef	FIOCLEX
	ioctl (sd, FIOCLEX, NULLCP);
#endif
#else

	if ((cp = getenv ("DISHPROC")) == NULL) {
		fprintf (stderr, "no DISHPROC in environment\n");
		return (NOTOK);
	}

	strcpy (retpipe, cp);
	umask (0);

	if ((fd = open (retpipe, O_RDONLY)) < 0) {
		mknod (retpipe, S_IFIFO | 0600, 0);
		if ((fd = open (retpipe, O_RDONLY)) < 0) {
			fprintf (stderr, "ropen failed\n");
			unlink (retpipe);
			return (NOTOK);
		}
	}

	if ((wfd = open (retpipe, O_WRONLY)) < 0) {
		fprintf (stderr, "wr open failed\n");
		unlink (retpipe);
		close (fd);
		return (NOTOK);
	}
#endif

#ifdef	SETSID
	setsid ();
#endif
#ifdef	TIOCNOTTY
	{
		int	    xsd;

		if ((xsd = open ("/dev/tty", O_RDWR)) != NOTOK) {
			ioctl (xsd, TIOCNOTTY, NULLCP);
			close (xsd);
		}
	}
#else
#ifdef	SYS5
	setpgrp ();
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
#endif
#endif
	return (OK);
}

int
exit_pipe (void) {
#ifdef SOCKETS
	close_tcp_socket (sd);
#else
	close (fd);
	close (wfd);
	unlink (retpipe);
#endif
}

int
read_pipe (char *buf, int len) {
#ifdef SOCKETS
	struct sockaddr_in sock;

	while ((sd_current = join_tcp_client (sd, &sock)) == NOTOK) {
		if (errno != EINTR) {
			perror("join_tcp_client");
			return (-1);
		}
	}
#ifdef	FIOCLEX
	ioctl (sd_current, FIOCLEX, NULLCP);
#endif
#endif
	return (read_pipe_aux(buf,len));
}


int
read_pipe_aux (char *buf, int len) {
	int res;
#ifdef	SOCKETS
	char *cp,
		 *ep;
#endif

	*buf = '\0';
#ifdef SOCKETS
	ep = (cp = buf) + len - 1;
	for (;;) {
		switch (res = recv (sd_current, cp, ep - cp, 0)) {
		case NOTOK:
			perror ("recv");
			close (sd_current);
			return NOTOK;

		case OK:
			break;

		default:
			cp += res - 1;
			if (*cp == '\n')
				break;
			if (++cp < ep)
				continue;
			break;
		}
		break;
	}
	*cp = NULL;

	return (cp - buf);
#else
	if ((res = read (fd, buf, len)) <= 0) {
		perror ("read error");
		reopen_ret ();
		return (-1);
	}
	*(buf + res) = 0;

	return (res);
#endif
}


#ifdef	SOCKETS
int
read_pipe_aux2 (char **buf, int *len) {
	int	    cc,
			i,
			j,
			res;
	char   *cp,
		   *dp,
		   *ep;
	char    buffer[BUFSIZ];

	*buf = NULL, *len = 0;

	switch (res = read_pipe_aux (buffer, sizeof buffer)) {
	case NOTOK:
	case OK:
		return res;

	case 1:
		*buf = buffer, *len = res;
		return res;

	default:
		if (sscanf (buffer + 1, "%d", &cc) != 1 || cc < 0) {
			fprintf (stderr, "protocol botch\n");
			return NOTOK;
		}
		if ((cp = malloc ((unsigned) cc + 1)) == NULL) {
			perror ("malloc");
			return NOTOK;
		}
		*buf = cp, *len = cc;

		dp = cp, j = cc;
		if (ep = index (buffer + 1, '\n')) {
			strcpy (dp, ++ep);
			i = strlen (ep);
			dp += i, j -= i;
		}
		break;
	}

	for (; j > 0; dp += i, j -= i)
		switch (i = recv (sd_current, dp, j, 0)) {
		case NOTOK:
			perror ("recv");
out:
			;
			free (cp);
			*buf = NULL, *len = 0;
			close (sd_current);
			return NOTOK;

		case OK:
			fprintf (stderr, "premature eof from peer\n");
			goto out;

		default:
			break;
		}
	*dp = NULL;

	return res;
}
#endif


#ifndef	SOCKETS
int
send_pipe (char *buf) {
	send_pipe_aux (buf);

	close (file);
	reopen_ret ();
}
#endif

int
send_pipe_aux (char *buf) {
	send_pipe_aux2 (buf, strlen (buf));
}

int
send_pipe_aux2 (char *buf, int i) {
	int res;

#ifndef	SOCKETS
	if ((file = open (inbuf, O_WRONLY)) <= 0) {
		fprintf (stderr, "error %s on %s\n",sys_errname (errno), inbuf);
		reopen_ret ();
		return;
	}
#endif

	while (i > 0) {
#ifdef	SOCKETS
		if ( (res= send(sd_current, buf, i, 0)) == -1) {
			perror("send");
			close (sd_current);
			return;
		}
#else
		if ((res = write (file, buf, MIN (BUFSIZ,i))) == -1 ) {
			fprintf (stderr,"result write error (2)\n");
			reopen_ret ();
			return;
		}
#endif
		buf += res, i -= res;
	}
}


#ifdef SOCKETS
int
get_dish_sock (struct sockaddr_in *isock) {
	char * getenv ();
	char * ptr;
	char buffer [BUFSIZ];
	int     portno;
	char   *dp;
	struct hostent *hp;

	if ((ptr = getenv ("DISHPROC")) == NULLCP) {
#ifdef	notanymore
		char   *cp,
#endif
			   portno = (getppid () & 0xffff) | 0x8000;
#ifdef	notanymore
		if ((hp = gethostbystring (cp = getlocalhost ())) == NULL) {
			fprintf (stderr,"%s: unknown host", cp);
			return (-1);
		}
		sprintf (buffer, "%s %d",
				 inet_ntoa (*(struct in_addr *) hp -> h_addr),
				 portno);
#else
		sprintf (buffer, "127.0.0.1 %d", portno);
#endif
		setenv ("DISHPROC", ptr = buffer);
	}

	if ((dp = index (ptr, ' ')) == NULLCP || sscanf (dp + 1, "%d", &portno) != 1) {
		fprintf (stderr,"DISHPROC malformed");
		return (-1);
	}
	*dp = NULL;

	if ((hp = gethostbystring (ptr)) == NULL) {
		fprintf (stderr,"%s: unknown host in DISHPROC", ptr);
		return (-1);
	}
	*dp = ' ';

	bzero ((char *) isock, sizeof *isock);
	isock -> sin_family = hp -> h_addrtype;
	isock -> sin_port = htons ((u_short) portno);
	inaddr_copy (hp, isock);

	return (0);

}

#else

int
reopen_ret (void) {
	close (fd);
	close (wfd);

	if ((fd = open (retpipe, O_RDONLY)) < 0) {
		if ( errno == EINTR ) {
			reopen_ret ();
			return;
		}
		fprintf (stderr, "re-ropen failed\n");
		unlink (retpipe);
		exit (-72);
	}
	if ((wfd = open (retpipe, O_WRONLY)) < 0) {
		fprintf (stderr, "re-wr open failed\n");
		unlink (retpipe);
		close (fd);
		exit (-73);
	}
}
#endif
