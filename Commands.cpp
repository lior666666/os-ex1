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

// <---------- START Command ------------>
Command::Command(const char* cmd_line) : cmd_line(cmd_line) {
    this->args_length = _parseCommandLine(cmd_line, this->args);
    _removeBackgroundSign(this->args[this->args_length-1]);
}
Command::~Command() {
    for(int i = 0; i < this->args_length; i++) {
        free(this->args[i]);
    }
}
const char* Command::getCmdLine() {
    return this->cmd_line;
}
// <---------- END Command ------------>

// <---------- START BuiltInCommand ------------>
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}
// <---------- END BuiltInCommand ------------>

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
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, SmallShell* smash);: BuiltInCommand(cmd_line), smash(smash) {}
void ChangeDirCommand:: execute(){
    if(args_length > 2){
        std::cerr << "smash error: cd: too many arguments" << endl;
    }
    else if(length == 2){
        char sign[] = "-";
        if(strcmp(args[1], sign) == 0){
            if(*(this->last_pwd) == NULL){
                std::cerr << "smash error: cd: OLDPWD not set" << endl;
            }
            else{
                char* copy_last_pwd = (char*) malloc(strlen(*last_pwd) + 1);
                strcpy(copy_last_pwd, *last_pwd);
                free(*last_pwd);
                *this->last_pwd = getcwd(NULL, 0);
                if(chdir(copy_last_pwd) == -1){
                    perror("smash error: chdir failed");
                    free(*last_pwd);
                    *this->last_pwd = getcwd(NULL, 0);
                }
                else{
                    std::cout << copy_last_pwd << "\n";
//                    free(*last_pwd);
//                    this->last_pwd = &prev_location;
//                    std::cout << *this->last_pwd << "\n";
                }
                free(copy_last_pwd);
            }
        }
        else{
            free(*last_pwd);
            *this->last_pwd = getcwd(NULL, 0);
            if (chdir(args[1]) == -1){
                perror("smash error: chdir failed");
                free(*last_pwd);
                *this->last_pwd = getcwd(NULL, 0);
            }
            else{
                std::cout << args[1] << "\n";
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

// <---------- START JobEntry ------------>
JobEntry::JobEntry(int job_id, const char* cmd_line, pid_t process_id, time_t time_inserted, bool isStopped) :
        job_id(job_id), cmd_line(cmd_line), process_id(process_id), time_inserted(time_inserted), isStopped(isStopped) {}
JobEntry::~JobEntry() {}
void JobEntry::printJob() {
    if (this->isStopped) {
        std::cout << "[" << this->job_id << "] " << this->cmd_line << " : " <<
            this->process_id << " " << difftime(this->time_inserted, time(NULL)) << " (stopped)" << endl;
    }
    else {
        std::cout << "[" << this->job_id << "] " << this->cmd_line << " : " <<
            this->process_id << " " << difftime(this->time_inserted, time(NULL)) << endl;
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
void JobsList::addJob(Command* cmd, bool isStopped) {
    JobEntry* job = new JobEntry(max_job_id++, cmd->getCmdLine(), getpid(), time(NULL), isStopped);
    jobs_vec->push_back(*job);
}
void JobsList::printJobsList() {
    vector<JobEntry>::iterator it;
    for(it = jobs_vec->begin(); it != jobs_vec->end(); it++) {
        it->printJob();
    }
}
void JobsList::removeFinishedJobs() {
    pid_t kidpid;
    int status;
    while((kidpid = waitpid(-1, &status, WNOHANG)) > 0)
        this->removeJobById(kidpid);
    max_job_id = jobs_vec->back().getJobID(); // back() is the last element in the vec
    JobEntry* last_stopped = getLastStoppedJob();
    if (last_stopped) // if there is stopped job in the vec
        max_stopped_jod_id = last_stopped->getJobID();
    else
        max_stopped_jod_id = 0;
}
JobEntry* JobsList::getJobById(int jobId) {
    vector<JobEntry>::iterator it;
    for(it = jobs_vec->begin(); it != jobs_vec->end(); it++)
        if(it->getJobID() == jobId)
            return &(*it);
    return NULL;
}
void JobsList::removeJobById(pid_t process_to_delete) {
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
// <---------- END JobsList ------------>

// <---------- START SmallShell ------------>
SmallShell::SmallShell() : prompt("smash") {}
const char* SmallShell::getPrompt(){
    return this->prompt;
}
void SmallShell::setPrompt(const char* prompt){
    this->prompt = prompt;
}
SmallShell::~SmallShell() {}
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
	// For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}