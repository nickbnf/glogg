#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <curses.h>
#include "SyncPipe.h"
#include "Process.h"
#include <system_error>

using namespace std;

#define MAXLINE 100

void tmpThread(int fd, bool &run, SyncPipe& pipe)
{
    cout << "Starting tmp thread\n";
    //Sequencer seqencer("config.json", fd);
    //seqencer.start();

    while(run){
        char chr[5];

        scanf("%s", chr);
        write(fd, chr, strlen(chr));
        //for(char ch:pipe.pop()){
        //    //seqencer.addCharacter(ch);
        //}
    }

    cout << "Finishing tmp thread\n";
}

void closeRemoteProcess(int fd)
{
    char CTRL_Q = 17;
    write(fd, &CTRL_Q, sizeof(CTRL_Q));
    sleep(1);
}

void sequencerThread(int fd, bool &run, SyncPipe& pipe, const string configDir, ExitReason& reason)
{
    cout << "Starting sequence thread\n";

    try{
//        Sequencer seqencer(configDir, "/config.json", fd);
//        seqencer.start();

        while(run){

            for(char ch:pipe.pop()){
//                if(seqencer.addCharacter(ch)){
//                    reason = ExitReason::DONE;
//                    run = false;
//                    break;
//                }
            }
            //seqencer.timerTick();
        }
//    }catch(const system_error& se){
//        cout << "! FATAL SYSTEM ERROR! \n" << se.what() << "\n";
//        reason = ExitReason::SYSTEM_ERROR;
    }catch(const exception& exc){
        cout << "! FATAL ERROR! \n" << exc.what() << "\n";
        reason = ExitReason::LOGIC_ERROR;
    }

    run = false;
    closeRemoteProcess(fd);
    cout << "Finishing sequence thread\n";
}

int readFromSubprocess(int fd, char &ch, int subprocess_restart_after_timeout)
{
    fd_set  rset;
    int rv = 0;
    int sel_ret = 0;
    struct timeval tv;

    tv.tv_sec = subprocess_restart_after_timeout;
    tv.tv_usec = 0;

    //always need to zero it first, then add our new descripto
    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    //wait for subprocess_restart_after_timeout seconds
    if( (sel_ret = select(fd + 1, &rset, NULL, NULL, &tv)) < 0){
        throw logic_error("\nERROR! readFromSubprocess - select failed\n");
    }
    else if(sel_ret == 0){
        throw logic_error("\nERROR! readFromSubprocess - timeout\n");
    }

    //ret must be positive
    if(FD_ISSET(fd, &rset)){
        rv = read(fd, &ch, 1);
    }

    return rv;
}


//int wrapForkedProcess(const string& subprocess)
ExitReason wrapForkedProcess(char **cmdLine, const string &configDir, int subprocess_restart_after_timeout)
{
    int fd1[2];
    int fd2[2];
    pid_t pid;
    char line[MAXLINE];
    ExitReason reason = ExitReason::UNKNOWN;

    if ( (pipe(fd1) < 0) || (pipe(fd2) < 0) )
    {
        cerr << "PIPE ERROR" << endl;
        return ExitReason::PIPE_ERROR;
    }
    if ( (pid = fork()) < 0 )
    {
        cerr << "FORK ERROR" << endl;
        return ExitReason::FORK_ERROR;
    }
    else  if (pid == 0)     // CHILD PROCESS
    {
        close(fd1[1]);
        close(fd2[0]);

        if (fd1[0] != STDIN_FILENO)
        {
            if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO)
            {
                cerr << "dup2 error to stdin" << endl;
            }
            close(fd1[0]);
        }

        printf("przed zamianą\n");

        if (fd2[1] != STDOUT_FILENO)
        {
            if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO)
            {
                cerr << "dup2 error to stdout" << endl;
            }
            close(fd2[1]);
        }

        write(1, "DAL_AUTOMATION\n", sizeof("DAL_AUTOMATION\n"));
        printf("po zamianie\n");

        fflush(stdout);
        setvbuf(stdout, NULL, _IONBF, 0);

        //std::vector<char> cstr(subprocess.begin(), subprocess.end() + 1);
        //char* args[]={"/usr/bin/stdbuf", "-o0", cstr.data(), NULL};

        //execvp(args[0],args);
        execvp(cmdLine[0], cmdLine);
        printf("nie udało się\n");

        return ExitReason::EXECVP_ERROR;
    }
    else        // PARENT PROCESS
    {
        int rv;
        close(fd1[0]);
        close(fd2[1]);

        char ch;
        bool run = true;
        SyncPipe pipe;

        thread th(sequencerThread, fd1[1],
                reference_wrapper(run),
                reference_wrapper(pipe),
                configDir,
                reference_wrapper(reason));
        //thread tmp(tmpThread, fd1[1], reference_wrapper(run), reference_wrapper(pipe));


        try{
        while(run){
            if ( (rv = readFromSubprocess(fd2[0], ch, subprocess_restart_after_timeout)) < 0 ){
            //if ( (rv = read(fd2[0], &ch, 1)) < 0 ){
                cerr << "READ ERROR FROM PIPE" << endl;
                if(reason == ExitReason::UNKNOWN){
                    cout << "ERROR! reason = ExitReason::PIPE_READ_ERROR";
                    reason = ExitReason::PIPE_READ_ERROR;
                }
                run = false;
            }
            else if (rv == 0){
                cerr << "Child Closed Pipe" << endl;
                if(reason == ExitReason::UNKNOWN){
                    cout << "ERROR! reason = ExitReason::CHILD_CLOSED_PIPE";
                    reason = ExitReason::CHILD_CLOSED_PIPE;
                }
                run = false;

            }else{
                pipe.push(ch);
                //putchar(ch);
            }
        }
        }catch(const exception& exc){
            cout << "\nERROR! Reading data from subprocess has been stopped by exception\n";
            cout << exc.what();
            reason = ExitReason::SYSTEM_ERROR;
            run = false;
        }

        cout << "\nReading data from subprocess has been stopped\n";

        run = false;
        pipe.push('x');

        cout << "Joining Sequencer thread...";

        th.join();

        cout << "DONE\n";
        //tmp.join();

        cout << "Leaving wrapForkedProcess function with reason: " << (int)reason << '\n';
    }

    return reason;
}
