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
        JobsList vec = smash.getJobsList();
        vec.removeFinishedJobs();
        vec.addJob(smash.getCurrJobID(), smash.getCurrCmdLine(), smash.getCurrProcessID(), true);
        vec.updateMaxJobID();
        vec.updateMaxStoppedJobID();
        (smash.CreateCommand("jobs"))->execute();
        if (kill(smash.getCurrProcessID(), SIGSTOP) == -1) {
            perror("smash error: kill failed");
        } else {
            std::cout << "smash: process " << smash.getCurrProcessID() << " was stopped" << endl;
        }
        (smash.CreateCommand("jobs"))->execute();
    }
    std::cout << "*****pid: " << getpid() << endl;
    (smash.CreateCommand("jobs"))->execute();
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

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

