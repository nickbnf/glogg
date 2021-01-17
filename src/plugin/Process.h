#ifndef PROCESS_H
#define PROCESS_H

enum class ExitReason
{
    UNKNOWN,
    DONE,
    LOGIC_ERROR,
    PIPE_ERROR,
    PIPE_READ_ERROR,
    CHILD_CLOSED_PIPE,
    FORK_ERROR,
    EXECVP_ERROR,
    SYSTEM_ERROR
};

ExitReason wrapForkedProcess(char **cmdLine, const string &configDir, int subprocess_restart_after_timeout);

#endif // PROCESS_H
