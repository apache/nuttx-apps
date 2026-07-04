/****************************************************************************
 * apps/netutils/dropbear/dropbear_nshsession.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "includes.h"
#include "channel.h"
#include "chansession.h"
#include "dbutil.h"
#include "session.h"

#include "nsh.h"
#include "nsh_console.h"

#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

struct dropbear_nshsession_s
{
  int pty_readfd;
  int pty_writefd;
  pid_t nsh_pid;
  volatile bool done;
  pthread_t waiter;
  bool waiter_started;
  bool have_winsize;
  bool has_pty;
  struct winsize win;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int dropbear_nsh_main(int argc, FAR char *argv[])
{
  FAR struct console_stdio_s *pstate;
  int ret;

  pstate = nsh_newconsole(true);
  if (pstate == NULL)
    {
      dropbear_log(LOG_WARNING, "failed to create NSH console");
      return -ENOMEM;
    }

  ret = nsh_session(pstate, NSH_LOGIN_NONE, argc, argv);
  dropbear_log(LOG_INFO, "NSH session exited: %d", ret);

  nsh_exit(&pstate->cn_vtbl, ret);
  return ret;
}

static FAR void *dropbear_nsh_waiter(FAR void *arg)
{
  FAR struct dropbear_nshsession_s *sess = arg;
  unsigned char ch = 0;
  int status;
  int ret;

  ret = waitpid(sess->nsh_pid, &status, 0);
  if (ret < 0)
    {
      dropbear_log(LOG_WARNING, "NSH session wait failed: %s",
                   strerror(errno));
    }

  sess->nsh_pid = -1;
  sess->done = true;

  /* Match Dropbear's SIGCHLD wakeup path.  The session loop checks channel
   * close conditions when this pipe becomes readable.
   */

  if (ses.signal_pipe[1] >= 0)
    {
      write(ses.signal_pipe[1], &ch, sizeof(ch));
    }

  return NULL;
}

static int dropbear_newchansess(FAR struct dropbear_channel *channel)
{
  FAR struct dropbear_nshsession_s *sess;

  sess = m_malloc(sizeof(*sess));
  memset(sess, 0, sizeof(*sess));
  sess->pty_readfd = -1;
  sess->pty_writefd = -1;
  sess->nsh_pid = -1;

  channel->typedata = sess;
  channel->prio = DROPBEAR_PRIO_LOWDELAY;
  return 0;
}

static int dropbear_sesscheckclose(FAR struct dropbear_channel *channel)
{
  FAR struct dropbear_nshsession_s *sess = channel->typedata;

  return sess != NULL && sess->done;
}

static int dropbear_setup_spawn_attrs(FAR posix_spawnattr_t *attr,
                                      FAR const char **errmsg)
{
  struct sched_param param;
  int rc;

  param.sched_priority = CONFIG_NETUTILS_DROPBEAR_SHELL_PRIORITY;
  *errmsg = "spawn priority setup";
  rc = posix_spawnattr_setschedparam(attr, &param);
  if (rc != 0)
    {
      return rc;
    }

  *errmsg = "spawn stack setup";
  rc = posix_spawnattr_setstacksize(
         attr, CONFIG_NETUTILS_DROPBEAR_SHELL_STACKSIZE);
  if (rc != 0)
    {
      return rc;
    }

  *errmsg = "spawn flags setup";
  return posix_spawnattr_setflags(attr, POSIX_SPAWN_SETSCHEDPARAM);
}

static int
dropbear_setup_spawn_stdio(FAR posix_spawn_file_actions_t *actions,
                           int slavefd)
{
  static const int stdio_fds[] =
  {
    STDIN_FILENO,
    STDOUT_FILENO,
    STDERR_FILENO
  };

  int i;
  int rc;

  for (i = 0; i < nitems(stdio_fds); i++)
    {
      rc = posix_spawn_file_actions_adddup2(actions, slavefd, stdio_fds[i]);
      if (rc != 0)
        {
          return rc;
        }
    }

  return 0;
}

#ifdef CONFIG_NETUTILS_DROPBEAR_SCP
static int
dropbear_setup_spawn_stdio3(FAR posix_spawn_file_actions_t *actions,
                            int stdinfd, int stdoutfd, int stderrfd)
{
  int rc;

  rc = posix_spawn_file_actions_adddup2(actions, stdinfd, STDIN_FILENO);
  if (rc != 0)
    {
      return rc;
    }

  rc = posix_spawn_file_actions_adddup2(actions, stdoutfd, STDOUT_FILENO);
  if (rc != 0)
    {
      return rc;
    }

  return posix_spawn_file_actions_adddup2(actions, stderrfd, STDERR_FILENO);
}
#endif /* CONFIG_NETUTILS_DROPBEAR_SCP */

static int
dropbear_setup_spawn_close(FAR posix_spawn_file_actions_t *actions,
                           int fd)
{
  if (fd <= STDERR_FILENO)
    {
      return 0;
    }

  return posix_spawn_file_actions_addclose(actions, fd);
}

static void dropbear_close_fd(FAR int *fd)
{
  if (*fd >= 0)
    {
      close(*fd);
      *fd = -1;
    }
}

static int dropbear_start_nsh(FAR struct dropbear_channel *channel,
                              FAR struct dropbear_nshsession_s *sess)
{
  posix_spawn_file_actions_t actions;
  posix_spawnattr_t attr;
  FAR const char *errmsg;
  int masterfd;
  int slavefd;
  int writefd;
  int rc;

  if (sess->nsh_pid >= 0)
    {
      dropbear_log(LOG_WARNING, "NSH session already running");
      return DROPBEAR_FAILURE;
    }

  rc = openpty(&masterfd, &slavefd, NULL, NULL,
               sess->have_winsize ? &sess->win : NULL);
  if (rc < 0)
    {
      dropbear_log(LOG_WARNING, "openpty failed: %s", strerror(errno));
      return DROPBEAR_FAILURE;
    }

  writefd = dup(masterfd);
  if (writefd < 0)
    {
      dropbear_log(LOG_WARNING, "pty dup failed: %s", strerror(errno));
      close(masterfd);
      close(slavefd);
      return DROPBEAR_FAILURE;
    }

  rc = posix_spawn_file_actions_init(&actions);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn actions init failed: %s",
                   strerror(rc));
      close(masterfd);
      close(writefd);
      close(slavefd);
      return DROPBEAR_FAILURE;
    }

  rc = posix_spawnattr_init(&attr);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn attr init failed: %s", strerror(rc));
      goto err_with_actions;
    }

  rc = dropbear_setup_spawn_attrs(&attr, &errmsg);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "%s failed: %s", errmsg, strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_stdio(&actions, slavefd);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn stdio setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_close(&actions, masterfd);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn master close setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_close(&actions, writefd);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn write close setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_close(&actions, slavefd);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn slave close setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  sess->nsh_pid = task_spawn("dropbear nsh", dropbear_nsh_main, &actions,
                             &attr, NULL, NULL);
  if (sess->nsh_pid < 0)
    {
      dropbear_log(LOG_WARNING, "failed to create NSH task: %s",
                   strerror(-sess->nsh_pid));
      goto err_with_attr;
    }

  close(slavefd);
  slavefd = -1;

  rc = pthread_create(&sess->waiter, NULL, dropbear_nsh_waiter, sess);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "failed to create NSH waiter: %s",
                   strerror(rc));
      close(masterfd);
      close(writefd);
      masterfd = -1;
      writefd = -1;
      kill(sess->nsh_pid, SIGTERM);
      waitpid(sess->nsh_pid, NULL, 0);
      sess->nsh_pid = -1;
      goto err_with_attr;
    }

  sess->waiter_started = true;
  sess->has_pty = true;
  sess->pty_readfd = masterfd;
  sess->pty_writefd = writefd;

  channel->readfd = masterfd;
  channel->writefd = writefd;
  channel->bidir_fd = 0;

  setnonblocking(channel->readfd);
  setnonblocking(channel->writefd);
  ses.maxfd = MAX(ses.maxfd, channel->readfd);
  ses.maxfd = MAX(ses.maxfd, channel->writefd);

  posix_spawnattr_destroy(&attr);
  posix_spawn_file_actions_destroy(&actions);

  dropbear_log(LOG_INFO, "NSH PTY session started");
  return DROPBEAR_SUCCESS;

err_with_attr:
  posix_spawnattr_destroy(&attr);

err_with_actions:
  posix_spawn_file_actions_destroy(&actions);

  if (masterfd >= 0)
    {
      close(masterfd);
    }

  if (writefd >= 0)
    {
      close(writefd);
    }

  if (slavefd >= 0)
    {
      close(slavefd);
    }

  sess->nsh_pid = -1;
  return DROPBEAR_FAILURE;
}

#ifdef CONFIG_NETUTILS_DROPBEAR_SCP
static int dropbear_start_exec(FAR struct dropbear_channel *channel,
                               FAR struct dropbear_nshsession_s *sess,
                               FAR char *cmd)
{
  posix_spawn_file_actions_t actions;
  posix_spawnattr_t attr;
  FAR const char *errmsg;
  FAR char * const argv[] =
  {
    "-c",
    cmd,
    NULL
  };

  int inpipe[2] =
  {
    -1,
    -1
  };

  int outpipe[2] =
  {
    -1,
    -1
  };

  int errpipe[2] =
  {
    -1,
    -1
  };

  int rc;

  if (sess->nsh_pid >= 0)
    {
      dropbear_log(LOG_WARNING, "NSH exec already running");
      return DROPBEAR_FAILURE;
    }

  if (pipe(inpipe) < 0 || pipe(outpipe) < 0 || pipe(errpipe) < 0)
    {
      dropbear_log(LOG_WARNING, "exec pipe setup failed: %s",
                   strerror(errno));
      goto err_with_pipes;
    }

  rc = posix_spawn_file_actions_init(&actions);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn actions init failed: %s",
                   strerror(rc));
      goto err_with_pipes;
    }

  rc = posix_spawnattr_init(&attr);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn attr init failed: %s", strerror(rc));
      goto err_with_actions;
    }

  rc = dropbear_setup_spawn_attrs(&attr, &errmsg);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "%s failed: %s", errmsg, strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_stdio3(&actions, inpipe[0], outpipe[1],
                                   errpipe[1]);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn stdio setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_close(&actions, inpipe[0]);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn stdin close setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_close(&actions, inpipe[1]);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn stdin writer close setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_close(&actions, outpipe[0]);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn stdout reader close setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_close(&actions, outpipe[1]);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn stdout close setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_close(&actions, errpipe[0]);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn stderr reader close setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  rc = dropbear_setup_spawn_close(&actions, errpipe[1]);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "spawn stderr close setup failed: %s",
                   strerror(rc));
      goto err_with_attr;
    }

  sess->nsh_pid = task_spawn("dropbear exec", dropbear_nsh_main, &actions,
                             &attr, argv, NULL);
  if (sess->nsh_pid < 0)
    {
      dropbear_log(LOG_WARNING, "failed to create NSH exec task: %s",
                   strerror(-sess->nsh_pid));
      goto err_with_attr;
    }

  dropbear_close_fd(&inpipe[0]);
  dropbear_close_fd(&outpipe[1]);
  dropbear_close_fd(&errpipe[1]);

  rc = pthread_create(&sess->waiter, NULL, dropbear_nsh_waiter, sess);
  if (rc != 0)
    {
      dropbear_log(LOG_WARNING, "failed to create NSH exec waiter: %s",
                   strerror(rc));
      dropbear_close_fd(&inpipe[1]);
      dropbear_close_fd(&outpipe[0]);
      dropbear_close_fd(&errpipe[0]);
      kill(sess->nsh_pid, SIGTERM);
      waitpid(sess->nsh_pid, NULL, 0);
      sess->nsh_pid = -1;
      goto err_with_attr;
    }

  sess->waiter_started = true;
  sess->has_pty = false;
  sess->pty_readfd = -1;
  sess->pty_writefd = -1;

  channel->readfd = outpipe[0];
  channel->writefd = inpipe[1];
  channel->errfd = errpipe[0];
  channel->bidir_fd = 0;

  setnonblocking(channel->readfd);
  setnonblocking(channel->writefd);
  setnonblocking(channel->errfd);
  ses.maxfd = MAX(ses.maxfd, channel->readfd);
  ses.maxfd = MAX(ses.maxfd, channel->writefd);
  ses.maxfd = MAX(ses.maxfd, channel->errfd);

  posix_spawnattr_destroy(&attr);
  posix_spawn_file_actions_destroy(&actions);

  dropbear_log(LOG_INFO, "NSH exec started: %s", cmd);
  return DROPBEAR_SUCCESS;

err_with_attr:
  posix_spawnattr_destroy(&attr);

err_with_actions:
  posix_spawn_file_actions_destroy(&actions);

err_with_pipes:
  dropbear_close_fd(&inpipe[0]);
  dropbear_close_fd(&inpipe[1]);
  dropbear_close_fd(&outpipe[0]);
  dropbear_close_fd(&outpipe[1]);
  dropbear_close_fd(&errpipe[0]);
  dropbear_close_fd(&errpipe[1]);
  sess->nsh_pid = -1;
  return DROPBEAR_FAILURE;
}
#endif /* CONFIG_NETUTILS_DROPBEAR_SCP */

static void dropbear_parse_winsize(FAR struct dropbear_nshsession_s *sess)
{
  unsigned int cols = buf_getint(ses.payload);
  unsigned int rows = buf_getint(ses.payload);
  unsigned int width = buf_getint(ses.payload);
  unsigned int height = buf_getint(ses.payload);

  sess->win.ws_col = cols;
  sess->win.ws_row = rows;
  sess->win.ws_xpixel = width;
  sess->win.ws_ypixel = height;
  sess->have_winsize = true;

  if (sess->pty_readfd >= 0)
    {
      ioctl(sess->pty_readfd, TIOCSWINSZ, (unsigned long)&sess->win);
    }
}

static int dropbear_handle_pty_req(FAR struct dropbear_nshsession_s *sess)
{
  unsigned int len;
  FAR char *term;
  FAR char *modes;

  term = buf_getstring(ses.payload, &len);
  dropbear_parse_winsize(sess);
  modes = buf_getstring(ses.payload, &len);

  m_free(term);
  m_free(modes);
  return DROPBEAR_SUCCESS;
}

static int dropbear_signal_from_name(FAR const char *name)
{
  int i;

  for (i = 0; signames[i].name != NULL; i++)
    {
      if (strcmp(name, signames[i].name) == 0)
        {
          return signames[i].signal;
        }
    }

  return -EINVAL;
}

static int dropbear_write_terminal_signal(
                            FAR struct dropbear_nshsession_s *sess,
                            int signo)
{
#ifdef CONFIG_TTY_SIGINT
  unsigned char ch;
  ssize_t nwritten;

  if (signo == SIGINT && sess->has_pty && sess->pty_writefd >= 0)
    {
      ch = CONFIG_TTY_SIGINT_CHAR;
      nwritten = write(sess->pty_writefd, &ch, sizeof(ch));
      if (nwritten == sizeof(ch))
        {
          return DROPBEAR_SUCCESS;
        }

      dropbear_log(LOG_WARNING, "SSH terminal INT failed: %s",
                   strerror(errno));
      return DROPBEAR_FAILURE;
    }
#endif

  return DROPBEAR_FAILURE;
}

static int dropbear_handle_signal(FAR struct dropbear_nshsession_s *sess)
{
  unsigned int len;
  FAR char *name;
  int signo;
  int ret;

  name = buf_getstring(ses.payload, &len);
  signo = dropbear_signal_from_name(name);
  if (signo < 0)
    {
      dropbear_log(LOG_WARNING, "unsupported SSH signal '%s'", name);
      m_free(name);
      return DROPBEAR_FAILURE;
    }

  ret = dropbear_write_terminal_signal(sess, signo);
  if (ret == DROPBEAR_SUCCESS)
    {
      dropbear_log(LOG_INFO, "SSH signal '%s' sent to PTY", name);
      m_free(name);
      return DROPBEAR_SUCCESS;
    }

  if (sess->nsh_pid <= 0)
    {
      dropbear_log(LOG_WARNING, "SSH signal '%s' has no NSH session", name);
      m_free(name);
      return DROPBEAR_FAILURE;
    }

  ret = kill(sess->nsh_pid, signo);
  if (ret < 0)
    {
      dropbear_log(LOG_WARNING, "SSH signal '%s' failed: %s",
                   name, strerror(errno));
      m_free(name);
      return DROPBEAR_FAILURE;
    }

  dropbear_log(LOG_INFO, "SSH signal '%s' sent to NSH session", name);
  m_free(name);
  return DROPBEAR_SUCCESS;
}

static void dropbear_chansessionrequest(FAR struct dropbear_channel *channel)
{
  unsigned int typelen;
  FAR char *type = buf_getstring(ses.payload, &typelen);
  unsigned char wantreply = buf_getbool(ses.payload);
  FAR struct dropbear_nshsession_s *sess = channel->typedata;
  int ret = DROPBEAR_FAILURE;

  TRACE(("dropbear_chansessionrequest: type='%s'", type))

  if (strcmp(type, "pty-req") == 0)
    {
      ret = dropbear_handle_pty_req(sess);
    }
  else if (strcmp(type, "shell") == 0)
    {
      ret = dropbear_start_nsh(channel, sess);
    }
  else if (strcmp(type, "exec") == 0)
    {
#ifdef CONFIG_NETUTILS_DROPBEAR_SCP
      unsigned int cmdlen;
      FAR char *cmd;

      cmd = buf_getstring(ses.payload, &cmdlen);
      ret = dropbear_start_exec(channel, sess, cmd);
      m_free(cmd);
#else
      dropbear_log(LOG_WARNING, "SSH exec requests are not supported");
#endif
    }
  else if (strcmp(type, "window-change") == 0)
    {
      dropbear_parse_winsize(sess);
      ret = DROPBEAR_SUCCESS;
    }
  else if (strcmp(type, "signal") == 0)
    {
      ret = dropbear_handle_signal(sess);
    }
  else if (strcmp(type, "break") == 0)
    {
      ret = DROPBEAR_SUCCESS;
    }
  else
    {
      TRACE(("dropbear_chansessionrequest: unhandled type '%s'", type))
    }

  if (wantreply)
    {
      if (ret == DROPBEAR_SUCCESS)
        {
          send_msg_channel_success(channel);
        }
      else
        {
          send_msg_channel_failure(channel);
        }
    }

  m_free(type);
}

static void
dropbear_closechansess(FAR const struct dropbear_channel *channel)
{
  FAR struct dropbear_nshsession_s *sess = channel->typedata;

  if (sess != NULL)
    {
      sess->done = true;
    }
}

static void
dropbear_cleanupchansess(FAR const struct dropbear_channel *channel)
{
  FAR struct dropbear_nshsession_s *sess = channel->typedata;

  if (sess == NULL)
    {
      return;
    }

  sess->done = true;

  if (sess->nsh_pid > 0)
    {
      kill(sess->nsh_pid, SIGTERM);
    }

  if (sess->waiter_started)
    {
      pthread_join(sess->waiter, NULL);
      sess->waiter_started = false;
    }

  m_free(sess);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Chansession lifecycle hooks invoked from svr-session.c.  Upstream
 * svr-chansession.c uses these to set up and reap the global childpids[] /
 * SIGCHLD machinery; the NSH bridge tracks each session's child through its
 * own waiter thread (dropbear_nsh_waiter), so both are no-ops here.
 */

void svr_chansessinitialise(void)
{
}

void svr_chansess_checksignal(void)
{
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct dropbear_chantype svrchansess =
{
  "session",
  dropbear_newchansess,
  dropbear_sesscheckclose,
  dropbear_chansessionrequest,
  dropbear_closechansess,
  dropbear_cleanupchansess,
};
