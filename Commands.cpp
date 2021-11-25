#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <time.h>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;
    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

char* _removeConstToCmdLine(char* cmd_line) {
    return cmd_line;
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx)] = 0;
}

// <---------- START JobEntry ------------>
JobEntry::JobEntry(int job_id, const char* cmd_line, pid_t process_id, time_t time_inserted, bool isStopped) :
        job_id(job_id), cmd_line(cmd_line), process_id(process_id), time_inserted(time_inserted), isStopped(isStopped) {}
JobEntry::~JobEntry() {}
void JobEntry::printJob() {
    if (this->isStopped) {
        std::cout << "[" << this->job_id << "] " << this->cmd_line << " : " <<
                  this->process_id << " " << difftime(time(NULL), this->time_inserted) << " secs (stopped)" << endl;
    }
    else {
        std::cout << "[" << this->job_id << "] " << this->cmd_line << " : " <<
                  this->process_id << " " << difftime(time(NULL), this->time_inserted) << " secs" << endl;
    }
}
pid_t JobEntry::getProcessID() {
    return this->process_id;
}
int JobEntry::getJobID() {
    return this->job_id;
}
bool JobEntry::isStoppedProcess() {
    return this->isStopped;
}
const char* JobEntry::getCmdLine() {
    return this->cmd_line;
}
void JobEntry::setIsStopped(bool setStopped) {
    this->isStopped = setStopped;
}
// <---------- END JobEntry ------------>

// <---------- START JobsList ------------>
JobsList::JobsList() {
    jobs_vec = new std::vector<JobEntry>;
    max_job_id = 0;
    max_stopped_jod_id = 0;
}
JobsList::~JobsList() {
    delete jobs_vec;
}
void JobsList::addJob(const char* cmd_line, pid_t pid, bool isStopped) {
    JobEntry* job = new JobEntry(max_job_id+1, strdup(cmd_line), pid, time(NULL), isStopped);
    jobs_vec->push_back(*job);
    updateMaxJobID();
    updateMaxStoppedJobID();
}
void JobsList::printJobsList() {
    vector<JobEntry>::iterator it;
    for(it = jobs_vec->begin(); it != jobs_vec->end(); it++) {
        it->printJob();
    }
}
void JobsList::removeFinishedJobs() {
    if (!this->isVecEmpty()) {
        pid_t kidpid = 1;
        int status;
        while (kidpid > 0) {
            kidpid = waitpid(-1, &status, WNOHANG);
            if (kidpid > 0) {
                this->removeJobByProcessId(kidpid);
            }
        }
        // need to do the rows below after every change in the vec
        updateMaxJobID();
        updateMaxStoppedJobID();
    }
}
void JobsList::updateMaxJobID() {
    if (jobs_vec->size() != 0)
        max_job_id = jobs_vec->back().getJobID(); // back() is the last element in the vec
}
void JobsList::updateMaxStoppedJobID() {
    if (jobs_vec->size() != 0){
        JobEntry* last_stopped = getLastStoppedJob();
        if (last_stopped) // if there is stopped job in the vec
            max_stopped_jod_id = last_stopped->getJobID();
        else
            max_stopped_jod_id = 0;
    }
}
JobEntry* JobsList::getJobById(int jobId) {
    vector<JobEntry>::iterator it;
    for(it = jobs_vec->begin(); it != jobs_vec->end(); it++)
        if(it->getJobID() == jobId)
            return &(*it);
    return NULL;
}
JobEntry* JobsList::getJobByProcessId(pid_t process_id) {
    vector<JobEntry>::iterator it;
    for(it = jobs_vec->begin(); it != jobs_vec->end(); it++)
        if(it->getProcessID() == process_id)
            return &(*it);
    return NULL;
}
void JobsList::removeJobByProcessId(pid_t process_to_delete) {
    vector<JobEntry>::iterator it;
    for(it = jobs_vec->begin(); it != jobs_vec->end(); it++) {
        if(it->getProcessID() == process_to_delete) {
            jobs_vec->erase(it);
            break;
        }
    }
}
JobEntry* JobsList::getLastStoppedJob() {
    vector<JobEntry>::reverse_iterator it;
    for(it = jobs_vec->rbegin(); it != jobs_vec->rend(); it++) // reverse loop!!
        if(it->isStoppedProcess())
            return &(*it);
    return NULL;
}
bool JobsList::isVecEmpty() {
    return (jobs_vec->size() == 0);
}
int JobsList::getMaxJobID() {
    return max_job_id;
}
int JobsList::getMaxStoppedJobID() {
    return max_stopped_jod_id;
}
void JobsList::turnToForeground(JobEntry* bg_or_stopped_job) {
    if (bg_or_stopped_job == NULL) { // something wrong!!
        std::cerr << "something wrong!!" << endl;
    }
    else {
        pid_t job_pid = bg_or_stopped_job->getProcessID();
        std::cout << bg_or_stopped_job->getCmdLine() << " : " << job_pid << endl;
        int kill_status = kill(job_pid, 18); //SIGCONT
        if (kill_status < 0) {
            perror("smash error: kill failed");
            return;
        }
        pid_t wait_status = waitpid(job_pid, NULL, 0);
        if (wait_status < 0) {
            perror("smash error: waitpid failed");
            return;
        }
        removeJobByProcessId(job_pid); //remove from vec
        updateMaxJobID();
        updateMaxStoppedJobID();
    }
}
void JobsList::killAllJobs() {
    std::cout << "smash: sending SIGKILL signal to " << jobs_vec->size() << " jobs:" << endl;
    vector<JobEntry>::iterator it;
    for(it = jobs_vec->begin(); it != jobs_vec->end(); it++) {
        std::cout << it->getProcessID() << ": " << it->getCmdLine() << endl;
        int kill_status = kill(it->getProcessID(), 9); //SIGKILL
        if (kill_status < 0) {
            perror("smash error: kill failed");
        }
    }
}
// <---------- END JobsList ------------>

// <---------- START Command ------------>
Command::Command(const char* cmd_line) : cmd_line(cmd_line) {
    this->cmd_line_without_const = strdup(cmd_line);
    this->args_length = _parseCommandLine(cmd_line, this->args);
    _removeBackgroundSign(this->args[this->args_length-1]);
}
Command::~Command() {
    for(int i = 0; i < this->args_length; i++) {
        free(this->args[i]);
    }
    free(this->cmd_line_without_const);
}
const char* Command::getCmdLine() {
    return this->cmd_line;
}
// <---------- END Command ------------>

// <---------- START BuiltInCommand ------------>
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}
// <---------- END BuiltInCommand ------------>

// <---------- START ExternalCommand ------------>
ExternalCommand::ExternalCommand(const char* cmd_line, JobsList* jobs) : Command(cmd_line), jobs(jobs) {}
void ExternalCommand::execute() {
    char file[] = "/bin/bash";
    char sign[] = "-c";
    _removeBackgroundSign(cmd_line_without_const);
    char* const argv[] = {file, sign, cmd_line_without_const, NULL};
    int execv_status = execv("/bin/bash", argv);
    if (execv_status < 0) {
        perror("smash error: execv failed");
    }
}
// <---------- END ExternalCommand ------------>

// <---------- START ChangePromptCommand ------------>
ChangePromptCommand::ChangePromptCommand(const char* cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line), smash(smash) {}
void ChangePromptCommand::execute() {
    if (args_length == 1)
        smash->setPrompt("smash");
    else
        smash->setPrompt(args[1]);
}
// <---------- END ChangePromptCommand ------------>

// <---------- START ShowPidCommand ------------>
ShowPidCommand::ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}
void ShowPidCommand::execute(){
    std::cout << "smash pid is " << getpid() << endl;  // need to check if that is the proper way.
}
// <---------- END ShowPidCommand ------------>

// <---------- START GetCurrDirCommand ------------>
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void GetCurrDirCommand::execute() {
    char* curr_dir = getcwd(NULL, 0);
    std::cout << curr_dir << endl;
    free(curr_dir);
}
// <---------- END GetCurrDirCommand ------------>

// <---------- START ChangeDirCommand ------------>
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, SmallShell* smash): BuiltInCommand(cmd_line), smash(smash){}
void ChangeDirCommand::execute(){
    if(args_length > 2){ // too many args
        std::cerr << "smash error: cd: too many arguments" << endl;
    }
    else if(args_length == 2){
        char sign[] = "-";
        if(strcmp(args[1], sign) == 0){ // change to last pwd
            if(smash->isLastPwdInitialized() == false){ // there is not last pwd
                std::cerr << "smash error: cd: OLDPWD not set" << endl;
            }
            else{
                char* copy_last_pwd = (char*)malloc(strlen(smash->getLastPwd()) + 1);
                strcpy(copy_last_pwd, smash->getLastPwd());
                smash->setLastPwd(getcwd(NULL, 0));
                if(chdir(copy_last_pwd) == -1){
                    smash->setLastPwd(copy_last_pwd);
                    free(copy_last_pwd);
                    perror("smash error: chdir failed");
                }
                else{
                    std::cout << copy_last_pwd << endl;
                }
                free(copy_last_pwd);
            }
        }
        else{ // change to arg[1]
            smash->changeLastPwdStatus(); // from this point there is last_pwd in the system!!
            smash->setLastPwd(getcwd(NULL, 0));
            if (chdir(args[1]) == -1){
                perror("smash error: chdir failed");
            }
            else{
                std::cout << args[1] << endl;
            }
        }
    }
}
// <---------- END ChangeDirCommand ------------>

// <---------- START JobsCommand ------------>
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void JobsCommand::execute() {
    jobs->removeFinishedJobs();
    jobs->printJobsList();
}
// <---------- END JobsCommand ------------>

// <---------- START KillCommand ------------>
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs) {}
void KillCommand::execute() {
    if(args_length!=3 || atoi(args[1])>-1 || atoi(args[1])<-64)
    {
        std::cerr << "mash error: kill: invalid arguments" << endl;
    }
    else
    {
        JobEntry* job_to_send_signal = jobs->getJobById(atoi(args[2]));
        if(job_to_send_signal == NULL){
            std::cerr <<  "smash error: kill: job-id " << args[2] << " does not exist" << endl;
        }
        else
        {
            const string str(cmd_line);
            if (kill(job_to_send_signal->getProcessID(), abs(atoi(args[1]))) != -1) {
                std::cout << "signal number " << abs(atoi(args[1])) << " was sent to pid "
                          << job_to_send_signal->getProcessID() << endl;
                if(abs(atoi(args[1])) == 19)
                {
                    job_to_send_signal->setIsStopped(true);
                }
            }
            else {
                perror("smash error: kill failed");
            }
        }
    }
}
// <---------- END KillCommand ------------>

// <---------- START ForegroundCommand ------------>
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void ForegroundCommand::execute() {
    jobs->removeFinishedJobs();
    if (args_length > 2) { // more than 1 arg
        std::cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    else if (args_length == 2) { // one arg
        JobEntry* bg_or_stopped_job = jobs->getJobById(atoi(args[1]));
        if (bg_or_stopped_job == NULL) { // there is no such bg/stopped job with given ID
            std::cerr << "smash error: fg: job-id " << atoi(args[1]) << " does not exist" << endl;
            return;
        }
        jobs->turnToForeground(bg_or_stopped_job);
    }
    else { // zero arg
        if (jobs->isVecEmpty()) { // there is no jobs in the vec
            std::cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
        JobEntry* bg_or_stopped_job = jobs->getJobById(jobs->getMaxJobID());
        jobs->turnToForeground(bg_or_stopped_job);
    }
}
// <---------- END ForegroundCommand ------------>

// <---------- START BackgroundCommand ------------>
BackgroundCommand::BackgroundCommand (const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void BackgroundCommand::execute() {
    jobs->removeFinishedJobs();
    if (args_length > 2) { // more than 1 arg
        std::cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    else if (args_length == 2) { // one arg
        JobEntry* bg_or_stopped_job = jobs->getJobById(atoi(args[1]));
        if (bg_or_stopped_job == NULL) { // there is no such bg/stopped job with given ID
            std::cerr << "smash error: bg: job-id " << atoi(args[1]) << " does not exist" << endl;
            return;
        }
        if(bg_or_stopped_job->isStoppedProcess())
        {
            if(kill(bg_or_stopped_job->getProcessID(), 18) != -1 )// sending signal for job to continue.
            {
                bg_or_stopped_job->setIsStopped(false);
                std::cout << bg_or_stopped_job->getCmdLine() << " : " << bg_or_stopped_job->getProcessID() << endl;
                jobs->updateMaxStoppedJobID();
            }
            else
            {
                perror("smash error: kill failed");
            }

        }
        else // the process still running in the background
        {
            std::cerr << "smash error: bg: job-id " << atoi(args[1])<<" is already running in the background" << endl;
        }
    }
    else { // zero arg
        JobEntry* last_stopped_job = jobs->getLastStoppedJob();
        if (last_stopped_job == NULL) { // there are no stopped jobs in the vec
            std::cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    }
}

// <---------- END BackgroundCommand ------------>


// <---------- START QuitCommand ------------>
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void QuitCommand::execute() {
    char sign[] = "kill";
    if (args[1] != NULL && strcmp(args[1], sign) == 0) {
        jobs->killAllJobs();
    }
    exit(0);
}
// <---------- END QuitCommand ------------>

// <---------- START SmallShell ------------>
SmallShell::SmallShell() : prompt("smash"), last_pwd(NULL), lastPwdInitialized(false) {}
SmallShell::~SmallShell() {}
const char* SmallShell::getPrompt(){
    return this->prompt;
}
void SmallShell::setPrompt(const char* prompt){
    this->prompt = prompt;
}
const char* SmallShell::getLastPwd(){
    return this->last_pwd;
}
void SmallShell::setLastPwd(const char* update_last_pwd) {
    this->last_pwd = update_last_pwd;
}
bool SmallShell::isLastPwdInitialized() {
    return this->lastPwdInitialized;
}
void SmallShell::changeLastPwdStatus() {
    this->lastPwdInitialized = true;
}
// <---------- END SmallShell ------------>



/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("chprompt") == 0) {
        return new ChangePromptCommand(cmd_line, this);
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line, this);
    }
    else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, &jobs_list);
    }
    else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, &jobs_list);
    }
    else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line, &jobs_list);
    }
    else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line, &jobs_list);
    }
    else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line, &jobs_list);
    }
    else {
        bool isBackground = _isBackgroundComamnd(cmd_line);
        pid_t pid = fork();
        if (pid == 0) { //child
            setpgrp();
            return new ExternalCommand(cmd_line, &jobs_list);
        }
        else if (pid > 0) { //parent
            if (isBackground == false) {
                pid_t wait_status = waitpid(pid, NULL, 0);
                if (wait_status < 0) {
                    perror("smash error: waitpid failed");
                }
            }
            else {
                jobs_list.removeFinishedJobs(); // if we are going to add to the vec so remove jobs from the shell process (father for all the bg commands)
                jobs_list.addJob(cmd_line, pid, false);
            }
        }
        else {
            perror("smash error: fork failed");
        }
    }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    if (cmd != NULL) {
        cmd->execute();
    }
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}