#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-Z" << endl;
    if (smash.getCurrProcessID() != getpid()) {
        //add to vec
        JobsList* vec = smash.getJobsList();
        vec->removeFinishedJobs();
        vec->addJob(smash.getCurrJobID(), (smash.getCurrCmdLine()).c_str(), smash.getCurrProcessID(), true);
        vec->updateMaxJobID();
        vec->updateMaxStoppedJobID();
        if (kill(smash.getCurrProcessID(), SIGSTOP) == -1) {
            perror("smash error: kill failed");
        } else {
            std::cout << "smash: process " << smash.getCurrProcessID() << " was stopped" << endl;
        }
    }
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-C" << endl;
    if (smash.getCurrProcessID() != getpid()) {
        if (kill(smash.getCurrProcessID(), SIGKILL) == -1) {
            perror("smash error: kill failed");
        } else {
            std::cout << "smash: process " << smash.getCurrProcessID() << " was killed" << endl;
        }
    }
}

void alarmHandler(int signum) {
    write(STDOUT_FILENO,"smash: got an alarm\n", 20);
    SmallShell& smash = SmallShell::getInstance();
    smash.getJobsList()->removeFinishedJobs();
    vector<JobEntry>::iterator it;
    for(it = smash.getTimeJobVec()->begin(); it != smash.getTimeJobVec()->end(); it++) {
        if (difftime(time(NULL), it->getTImeInserted()) == it->getTimeUp()) {
            if (kill(it->getProcessID(), 9) == -1) {
                perror("smash error: kill failed");
            } else {
                std::cout << "smash: " << it->getCmdLine() << " timed out!" << endl;
            }
            smash.getTimeJobVec()->erase(it);
            int alarm_num = smash.findMinAlarm();
            if (alarm_num != -1)
                alarm(alarm_num);
            return;
        }
    }
    if (smash.getCurrProcessID() != getpid()) {
        if (kill(smash.getCurrProcessID(), 9) == -1) {
            perror("smash error: kill failed");
        }
        else {
            std::cout << "smash: " << smash.getLastCmd() << " timed out!" << endl;
        }
    }
}


