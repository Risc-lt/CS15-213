/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <bits/types/sigset_t.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */

/* selfdefined veriabals */
volatile sig_atomic_t fgchld_flag = 1; /* flag of sifnal SIGCHLD */ 

/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/******************************
 * Error handling wrapper function
 ******************************/

/*
    Selfmade part for simplying error handling functions
*/


pid_t Fork(void) {
    pid_t pid;
    if ((pid = fork()) < 0) {
        unix_error("Fork error");
    }
    return pid;
}

void Kill(pid_t pid, int sig) {
    if (kill(pid, sig) < 0) {
        unix_error("Kill error");
    }
    return;
}

pid_t Wait(int *status) {
    pid_t pid;
    if ((pid = wait(status)) < 0) {
        unix_error("Wait error");
    }
    return pid;
}

pid_t Waitpid(pid_t pid, int *status, int options) {
    pid_t p;
    if ((p = waitpid(pid, status, options)) < 0) {
        unix_error("Waitpid error");
    }
    return p;
}

void Sigemptyset(sigset_t *mask) {
    if (sigemptyset(mask) < 0) {
        unix_error("Sigemptyset error");
    }
    return;
}

void Sigfillset(sigset_t *mask) {
    if (sigfillset(mask) < 0) {
        unix_error("Sigfillset error");
    }
    return;
}

void Sigaddset(sigset_t *mask, int sign) {
    if (sigaddset(mask, sign) < 0) {
        unix_error("Sigaddset error");
    }
    return;
}

void Sigprocmask(int how, sigset_t *mask, sigset_t *oldmask) {
    if (sigprocmask(how, mask, oldmask) < 0) {
        unix_error("Sigprocmask error");
    }
    return;
}

void Setpgid(pid_t pid, pid_t gpid) {
    if (setpgid(pid, gpid) < 0) {
        unix_error("Setpgid error");
    }
    return;
}

void Execve(const char *filename, char *const argv[], char *const envp[]) {
    if (execve(filename, argv, envp) < 0) {
        printf("%s: Command not found.\n", filename);
        exit(0);
    }
}
/******************************
 * End Error handling wrapper function
 ******************************/

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) {
    char *argv[MAXARGS]; /* Argument list execv() */
    char buf[MAXLINE];   /* Holds modified commandline */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline);
    bg = parseline(buf, argv); // parseline() will return bg depending on &
    if (argv[0] == NULL) {
        return; /* Ignore empty lines */
    }

    if (!builtin_cmd(argv)) {
        sigset_t mask_all, mask_one, prev_one;

        Sigfillset(&mask_all);
        Sigemptyset(&mask_one);
        Sigaddset(&mask_one, SIGCHLD);

        /* in case if recieve "SIGCHLD" while forking */
        Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);  
        
        if ((pid = Fork()) == 0) {
            /* Child runs user job */
            Sigprocmask(SIG_SETMASK, &prev_one, NULL); // The child process should be restored as before
            setpgid(0, 0); 
            Execve(argv[0], argv, environ); 
        }
        /* Parent process */
        Sigprocmask(SIG_BLOCK, &mask_all, NULL); // protect the global variable "jobs"
        
        /* Add the child to the job list */
        addjob(jobs, pid, bg ? BG : FG, cmdline); 
        
        
        /* Parent waits for foreground job to terminate */
        if (!bg) {
          	Sigprocmask(SIG_BLOCK, &mask_one, NULL);
            waitfg(pid);
        } else {
            Sigprocmask(SIG_SETMASK, &mask_all, NULL);
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        }
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);
    }

    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    if (!strcmp(argv[0], "quit")) {
        // exit the shell
        exit(0);
    } else if (!strcmp(argv[0], "jobs")) {
        sigset_t mask_all, prev_mask;
        sigfillset(&mask_all);

        // protect the global variable "jobs"
        sigprocmask(SIG_SETMASK, &mask_all, &prev_mask); 
        listjobs(jobs);
        sigprocmask(SIG_SETMASK, &prev_mask, NULL); 

        return 1;
    } else if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) { 
        // switch to fg/bg operation
        do_bgfg(argv);

        return 1;
    } else if (!strcmp(argv[0], "&")) {
        return 1;
    }
    return 0; /* not a builtin command */
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 */

// self-defined function to check if param is digit
int isDigit(char *str) {
    while (str) {
        if (!isdigit(str)) return 0;
        str++;
    }
    return 1;
}

void do_bgfg(char **argv) {
    struct job_t *job;
    char *param;
    int jid;
    pid_t pid;

    param = argv[1];
    sigset_t mask_all, prev_mask;
    sigfillset(&mask_all);
    // protect the global variable "jobs"
    sigprocmask(SIG_SETMASK, &mask_all, &prev_mask);
    if (param == NULL) {  // lack of parameters
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    // if it's JID
    if (param[0] == '%') {  
        jid = atoi(&param[1]);
        job = getjobjid(jobs, jid);

        // if none of the job meets the JID
        if (job == NULL) { 
            printf("%s: No such job\n", param);
            return;
        } else {
            pid = job->pid;
        }
    } else if (isDigit(param)) { // if it's PID
        pid = atoi(&param[0]);
        job = getjobpid(jobs, pid);
        if (job == NULL) {
            printf("(%d): No such process\n", pid);
            return;
        } else {
            jid = job->jid;
        }
    } else {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }

    Kill(-pid, SIGCONT); // send the signal of "SIGCONT" to all child process in the group

    if (!strcmp(argv[0], "bg")) { // If background, set status and print
        job->state = BG;
        printf("[%d] (%d) %s", jid, pid, job->cmdline);
    } else if (!strcmp(argv[0], "fg")) { // If foreground, set status and hang
        job->state = FG;
        waitfg(job->pid);
    }
    sigprocmask(SIG_SETMASK, &prev_mask, NULL); 

    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{   
    sigset_t mask_empty;
    sigemptyset(&mask_empty); // emptify the mask
    fgchld_flag = 0; // 1 for frontground job

    while (!fgchld_flag) {
        sigsuspend(&mask_empty); // onlt the pause inside the function will not be masked
    }

    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    int oldErrno = errno;  // save previous errno

    sigset_t mask_all, prev;
    Sigfillset(&mask_all);
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) 
    /* Waiting for an arbitrary child process to change state without mask */
    {  
        Sigprocmask(SIG_SETMASK, &mask_all, &prev);  // protect the global variable "jobs"
        struct job_t *job = getjobpid(jobs, pid);
        if (job->state == FG) {  // If foreground, change the flag to notify function "waitfg()"
            fgchld_flag = 1;
        }
        if (WIFEXITED(status)) {  // If it exits normally, delete the current child process directly from the job table
            deletejob(jobs, pid);
        } else if (WIFSIGNALED(status)) {  // If it is a received signal termination, print the message
            printf("Job [%d] (%d) terminated by signal %d\n", job->jid, pid,
                   WTERMSIG(status));
            deletejob(jobs, pid);
        } else if (
            WIFSTOPPED(status)) {  // If it is suspended, change the job status and print the message
            job->state = ST;
            printf("Job [%d] (%d) stopped by signal %d\n", job->jid, pid,
                   WSTOPSIG(status));
        }
        Sigprocmask(SIG_SETMASK, &prev, NULL);  // reload the mask
    }
    errno = oldErrno;  // reload the errno
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    sigset_t mask_all, prev;
    Sigfillset(&mask_all);

    Sigprocmask(SIG_SETMASK, &mask_all, &prev); // mask all signals and keep the original value in prev
    pid_t pid = fgpid(jobs);
    Sigprocmask(SIG_SETMASK, &prev, NULL); // reload the previous masks

    if(pid == 0) // if no fg_job, then continue
        return;
    else 
        Kill(-pid, sig); //send signal to the current foreground task and its child processes 

    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{   
    /* the same as sigint_hander */
    sigset_t mask_all, prev;
    Sigfillset(&mask_all);

    Sigprocmask(SIG_SETMASK, &mask_all, &prev); 
    pid_t pid = fgpid(jobs);
    Sigprocmask(SIG_SETMASK, &prev, NULL); 

    if(pid == 0) 
        return;
    else 
        Kill(-pid, sig); 

    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1) // in case if pid is invalid
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
