#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char* cmd)
{
    /*
     *  Call the system() function with the command set in the cmd
     *   and return a boolean true if the system() call completed with success
     *   or false() if it returned a failure
    */
    int status = system(cmd);
    if (status == 0) {
        return true;
    }
    else {
        return false;
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/
bool do_exec(int count, ...)
{
    openlog("do_exec", 0, LOG_USER);

    va_list args;
    va_start(args, count);
    char* command[count + 1];
    int i;
    for (i = 0; i < count; i++) {
        command[i] = va_arg(args, char*);
    }
    command[count] = NULL;

    char command_binary[1024];
    strcpy(command_binary, command[0]);
    if (command_binary[0] != '/') {
        // The command binary is not specified.
        return false;
    }

    // Find the last occurrence of the directory separator '/'
    char* filename;
    filename = strrchr(command[0], '/');
    if (filename != NULL) {
        filename += 1;
        command[0] = filename;
    }

    char command_str[1024];
    int offset = 0;
    // Concatenate the command and arguments into a single string
    for (i = 0; i < count + 1; i++) {
        offset += sprintf(command_str + offset, "%s ", command[i]);
    }

    /*
     *   Execute a system command by calling fork, execv(),
     *   and wait instead of system (see LSP page 161).
     *   Use the command[0] as the full path to the command to execute
     *   (first argument to execv), and use the remaining arguments
     *   as second argument to the execv() command.
     *
    */
    int status;
    syslog(LOG_INFO, "Start forking with the command: %s, \"%s\"\n", command_binary, command_str);
    fflush(stdout);
    pid_t pid = fork();
    if (pid == -1) {
        // Fail to fork.
        syslog(LOG_ERR, "Fail to fork with the command: %s, \"%s\"\n", command_binary, command_str);
        va_end(args);
        return false;
    }
    else if (pid == 0) {
        // Child process
        execv(command_binary, command);

        syslog(LOG_ERR, "Args: %s, \"%s\". pid = %d\n", command_binary, command_str, pid);
        // execv returns if and only if there is an error.
        va_end(args);
        return false;
    }
    else if (waitpid(pid, &status, 0) == -1) {
        // Error occurred while waiting for child process.
        syslog(LOG_ERR, "waitpid fails with the command: %s, \"%s\".\n", command_binary, command_str);
        perror("waitpid");
        va_end(args);
        return false;
    }
    else if (WIFEXITED(status)) {
        // Successfully executed the command.
        syslog(LOG_USER, "Successfully executed the command: %s, \"%s\". status = %i, WEXITSTATUS(status) = %i. pid = %d\n", command_binary, command_str, status, WEXITSTATUS(status), pid);
        va_end(args);
        return !WEXITSTATUS(status);
    }
    else {
        // Command executed failed.
        syslog(LOG_ERR, "Command executed failed with the command: %s, \"%s\".\n", command_binary, command_str);
        va_end(args);
        return false;
    }

    syslog(LOG_INFO, "return true: %s, \"%s\".\n", command_binary, command_str);
    fflush(stdout);
    va_end(args);
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char* outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char* command[count + 1];
    int i;
    for (i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char*);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


    /*
     *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
     *   redirect standard out to a file specified by outputfile.
     *   The rest of the behaviour is same as do_exec()
     *
    */
    int fd = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd < 0)
    {
        syslog(LOG_ERR, "failed to open the file: %s.", outputfile);
        va_end(args);
        return false;
    }
    fflush(stdout);

    pid_t pid = fork();
    if (pid == -1) {
        // Fail to fork.
        syslog(LOG_ERR, "Fail to fork.");
        va_end(args);
        return false;
    }
    else if (pid == 0) {
        // Child process
        if (dup2(fd, 1) < 0) {
            syslog(LOG_ERR, "Fail to dup2.");
            close(fd);
            va_end(args);
            return false;
        }

        close(fd);
        execvp(command[0], command);

        syslog(LOG_ERR, "execvp failed\n");
        // execvp returns if and only if there is an error.
        va_end(args);
        return false;
    }
    else {
        close(fd);
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            // Error occurred while waiting for child process.
            syslog(LOG_ERR, "waitpid fails.\n");
            va_end(args);
            return false;
        }
        else if (WIFEXITED(status)) {
            // Successfully executed the command.
            va_end(args);
            syslog(LOG_USER, "Successfully executed the command.\n");
            return !WEXITSTATUS(status);;
        }
        else {
            // Command execution failed
            syslog(LOG_ERR, "Command executed failed.\n");
            va_end(args);
            return false;
        }
    }

    va_end(args);
    return true;
}
