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

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}
