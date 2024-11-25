/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>

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
 * 				前台状态，		 后台状态，		  停止状态
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
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
bool do_bgfg(char **argv);
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

//#define LOG(fmt, ...) fprintf(stderr, "INFO: [%s:%d] %s: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#define LOG()

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
    // 将以下信号（大写字母）注册到函数上，即触发信号后会调用对应函数
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
void eval(char *cmdline) 
{
  	LOG();
    char* argv[MAXARGS];
	int state = parseline(cmdline, argv) ? BG : FG;
    if(argv[0] == NULL) {
    	return;
    }
    if(builtin_cmd(argv)) {
      	return;
    }
	sigset_t mask_all, mask_prev;
	sigfillset(&mask_all);
	sigprocmask(SIG_BLOCK, &mask_all, &mask_prev);
    pid_t pid=fork();
    if(pid != 0) { // 父进程
    	addjob(jobs, pid, state, cmdline);
    	sigprocmask(SIG_SETMASK, &mask_prev, NULL);
      	if(state == FG) {
      		waitfg(pid);
        } else {
        	printf("[%d] (%d) %s\n", pid2jid(pid), pid, cmdline);
        }
    } else { // 子进程
    	// 创建新进程组
    	setpgid(0, 0);
    	sigprocmask(SIG_SETMASK, &mask_prev, NULL);

//        int dev_null = open("/dev/null", O_WRONLY);
//        if (dev_null == -1) {
//        	perror("Failed to open /dev/null");
//        	exit(1);
//        }
//        // 将 stdout 重定向到 /dev/null
//        if (dup2(dev_null, STDOUT_FILENO) == -1) {
//        	perror("dup2 failed");
//        	close(dev_null);
//        	exit(1);
//        }
//        close(dev_null); // 关闭多余的文件描述符

    	if (execve(argv[0], argv, environ) < 0) {
    		printf("%s: Command not found\n", argv[0]);
    		exit(0);
    	}
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
  	LOG();
	// 生命周期是整个程序的运行周期，因此不必担心此内存会被覆盖
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
  	LOG();
	if(strcmp(argv[0], "quit") == 0 && argv[1] == NULL) {
        int i;
		for(i = 0; i < MAXJOBS; ++i) {
			deletejob(jobs, jobs[i].pid);
		}
        exit(0);
	}
    if(strcmp(argv[0], "jobs") == 0 && argv[1] == NULL) {
		listjobs(jobs);
        return 1;
	}
    if(do_bgfg(argv))
      	return 1;
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
bool do_bgfg(char **argv)
{
  	LOG();
	if((strcmp(argv[0], "bg") != 0 && strcmp(argv[0], "fg") != 0) || argv[1] == NULL || argv[2] != NULL) {
		return 0;
	}
    struct job_t* job;
    if(argv[1][0] == '%') {
    	job = getjobjid(jobs, atoi(argv[1]+1));
    } else {
    	job = getjobpid(jobs, atoi(argv[1]));
    }
	if(strcmp(argv[0], "bg") == 0) {
    	job->state = BG;
        kill(job->pid, SIGCONT);
    } else {
    	job->state = FG;
        kill(job->pid, SIGCONT);
    }
    return 0;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
  	LOG();
  	struct job_t* job = NULL;
  	do {
        job = getjobpid(jobs, pid);
    } while(job != NULL && job->state == FG);
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
/*
在操作系统中，当一个子进程结束（变为僵尸进程），或者被停止（如通过 SIGSTOP 或 SIGTSTP 信号），内核会向父进程发送 SIGCHLD 信号。
这个函数的作用是对这些信号进行处理，主要目的是清理僵尸进程，避免系统资源泄露。
 */
void sigchld_handler(int sig) 
{
  	LOG();
  	int i, status;
    for(i = 0; i < MAXJOBS; ++i) {
      	if(jobs[i].pid == 0)
          	continue;
      	if(waitpid(jobs[i].pid, &status, WNOHANG) != jobs[i].pid)
        	continue;
//      	if (WIFEXITED(status)) {
//      		printf("Child %d terminated with exit status %d\n", jobs[i].pid, WEXITSTATUS(status));
//      	} else if (WIFSIGNALED(status)) {
//      		printf("Child %d terminated by signal %d\n", jobs[i].pid, WTERMSIG(status));
//      	} else if (WIFSTOPPED(status)) {
//      		printf("Child %d stopped by signal %d\n", jobs[i].pid, WSTOPSIG(status));
//      	}
      	deletejob(jobs, jobs[i].pid);
    }
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
/*
这里是指收到 ctrl-c 会触发的函数，这里会将此信号转发前台信号而不是停止本 shell
 */
void sigint_handler(int sig) 
{
  	LOG();
	int pid = fgpid(jobs);
    if(pid == 0) {
//    	exit(0);
    } else {
		kill(pid, sig);
		deletejob(jobs, pid);
    }
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig)
{
  	LOG();
	int olderrno = errno;
	pid_t pid = fgpid(jobs);
	if (pid) {
		kill(pid, SIGTSTP);
	}
	// 恢复 errno
	errno = olderrno;
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
  	LOG();
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
  	LOG();
    int i;
    if (pid < 1)
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
/*
当 SIGQUIT 信号发送到这个程序：
	信号处理器 sigquit_handler 被调用。
	输出一条提示信息：Terminating after receipt of SIGQUIT signal。
	程序以状态码 1 退出。
 */
void sigquit_handler(int sig) 
{
  	LOG();
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



