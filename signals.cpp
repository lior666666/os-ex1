#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    // TODO: Add your implementation here
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    if (smash.getCurrProcessID() != getpid()) {
        std::cout << "smash: got ctrl-C" << endl;
        if (kill(smash.getCurrProcessID(), sig_num) == -1) {
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


