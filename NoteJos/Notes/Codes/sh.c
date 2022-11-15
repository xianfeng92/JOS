#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Simplified xv6 shell

#define MAXARGS 10

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type;  //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;             // ' '
  char *argv[MAXARGS];  // arguments to the command to be exec-ed
};

struct redircmd {
  int type;         // < or >
  struct cmd *cmd;  // the command to be run (e.g., an execcmd)
  char *file;       // the input/output file
  int flags;        // flags for open() indicating read or write
  int fd;           // the file descriptor number to use for the file
};

struct pipecmd {
  int type;           // |
  struct cmd *left;   // left side of pipe
  struct cmd *right;  // right side of pipe
};

int fork1(void);  // Fork but exits failure

struct cmd *parsecmd(char *);

// Execute cmd, Never returns
void runcmd(struct cmd *cmd) {
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if (cmd == 0) {
    _exit(0);
  }

  switch (cmd->type) {
    default:
      fprintf(stderr, "Unknown runcmd\n");
      _exit(-1);
    case ' ':
      ecmd = (struct execcmd *)cmd;
      if (ecmd->argv[0] == 0) {
        __exit(-1);
      }
      fprintf(stderr, "exec not implemented\n");
      // Your code here
      break;
    case '>':
    case '<':
      rcmd = (struct redircmd *)cmd;
      fprintf(stderr, "redir not implemented\n");
      // Your code here
      runcmd(rcmd->cmd);
      break;
    case '|':
      pcmd = (struct pipecmd *)cmd;
      fprintf(stderr, "pipe not implemented\n");
      // Your code here
      break;
  }
  _exit(0);
}

int getcmd(char *buf, int nbuf) {
  if (isatty(fileno(stdin))) fprintf(stdout, "6.828$ ");
  memset(buf, 0, nbuf);
  if (fgets(buf, nbuf, stdin) == 0) return -1;  // EOF
  return 0;
}

