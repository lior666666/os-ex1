#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    if (smash.getCurrProcessID() != getpid()) {
        std::cout << "smash: got ctrl-Z" << endl;
        //add to vec
        JobsList vec = smash.getJobsList();
        vec.removeFinishedJobs();
        vec.addJob(smash.getCurrJobID(), smash.getCurrCmdLine(), smash.getCurrProcessID(), true);
        vec.updateMaxJobID();
        vec.updateMaxStoppedJobID();
        if (kill(smash.getCurrProcessID(), 19) == -1) {
            perror("smash error: kill failed");
        } else {
            std::cout << "smash: process " << smash.getCurrProcessID() << " was stopped" << endl;
        }
    }
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    if (smash.getCurrProcessID() != getpid()) {
        std::cout << "smash: got ctrl-C" << endl;
        if (kill(smash.getCurrProcessID(), 9) == -1) {
            perror("smash error: kill failed");
        } else {
            std::cout << "smash: process " << smash.getCurrProcessID() << " was killed" << endl;
        }
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

