#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

/********************************/
/* definitions globales		*/
/********************************/
int log_fd;
char log_filename[80];
int log_file = 0;

#define MAXFILESIZE 200000
extern int log_print;
extern int erreur, t_err;

#if 0
static void PANIC (char *str, int flg)
{
    char msg[128];

    sprintf (msg, "Panic! Cannot %s errno[%d] ", str, errno);
    perror (msg);
    exit (errno);
}

static void Serv_Log (char *str)
{
    int timeval;
    struct tm *tmbf;
    char timestr[80], msg[128];
    struct stat file_stat;

    time (&timeval);
    tmbf = localtime (&timeval);
    sprintf (timestr, "%d-%02d-%02d-%02d:%02d:%02d", tmbf->tm_year + 1900,
	     tmbf->tm_mon + 1, tmbf->tm_mday, tmbf->tm_hour, tmbf->tm_min,
	     tmbf->tm_sec);

    if (fstat (log_fd, &file_stat) == -1)
	PANIC ("fstat on logfile", 3);
    if (file_stat.st_size > MAXFILESIZE) {	/* file too big. Restart it */
	close (log_fd);
	/* pch 5 Apr 93 - mv current filename in .old before restarting it */
	sprintf (msg, "%s.old", log_filename);
	unlink (msg);
	if (link (log_filename, msg) == -1)
	    PANIC ("link to .old");
	if (unlink (log_filename) == -1)
	    PANIC ("unlink to .old");
	/* pch 5/4/93 end */
	if ((log_fd = open (log_filename, O_WRONLY | O_CREAT, 0644)) == -1)
	    PANIC ("re-create filename");
	sprintf (msg, "%s:\t!!! FILE RESTARTS !!!\n", timestr);
	if (write (log_fd, msg, strlen (msg)) == -1)
	    PANIC ("restart write to filename", 3);
    }
    sprintf (msg, "%s:%s", timestr, str);
    if (write (log_fd, msg, strlen (msg)) == -1)
	PANIC ("write to logfile", 3);
}
#endif
/* log some comments depending on flag */
void print (int flg, char *str, ...)
{
    va_list Argp;

    if ((log_print == 0) && (erreur == 0) && (flg == 2))
	return;
    if ((erreur != 0) && (t_err == 0))
	return;

    va_start (Argp, str);
    vprintf (str, Argp);
    va_end (Argp);
}
