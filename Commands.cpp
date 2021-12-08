#include <unistd.h>
#include <fcntl.h>
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

bool _isTimeCommand(const char* cmd_line){
    char* tmp_args[COMMAND_MAX_ARGS];
    int arr_length = _parseCommandLine(cmd_line, tmp_args);
    std::string str(tmp_args[0]);
    bool check = (strcmp("timeout",str.c_str()) == 0);
    for(int i = 0; i < arr_length; i++) {
        free(tmp_args[i]);
    }
    return check;
}

int _isPipeCommand(const char* cmd_line) {
    const string cmd_line_copy(cmd_line);
    std::string pipe_sign1 = " | ";
    std::string pipe_sign2 = " |& ";
    if (cmd_line_copy.find(pipe_sign1) != std::string::npos) {
        return 1; // pipe cout
    }
    if (cmd_line_copy.find(pipe_sign2) != std::string::npos) {
        return 2; // pipe cerr
    }
    return 0; // no pipe
}

void _splitPipeCommands(const char* cmd_line, std::string* left, std::string* right) {
    const string cmd_line_copy(cmd_line);
    std::string pipe_sign1 = " | ";
    std::string pipe_sign2 = " |& ";
    int pipe = _isPipeCommand(cmd_line);
    if (pipe == 1) {
        int pipe_sign_position = cmd_line_copy.find(pipe_sign1);
        *left = cmd_line_copy.substr(0, pipe_sign_position);
        *right = cmd_line_copy.substr(pipe_sign_position + 2, strlen(cmd_line));
    }
    else { // pipe==2
        int pipe_sign_position = cmd_line_copy.find(pipe_sign2);
        *left = cmd_line_copy.substr(0, pipe_sign_position);
        *right = cmd_line_copy.substr(pipe_sign_position + 3, strlen(cmd_line));
    }
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

std::string removeTimeOut(const char* cmd_line, char* arg){
    string str(cmd_line);
    string str_arg(arg);
    string sub_str = str.substr(str.find("timeout")+7, str.length());
    const string sub_str2 = sub_str.substr(sub_str.find(arg) + str_arg.length(), sub_str.length());
    sub_str = _ltrim(sub_str2);
    return sub_str.c_str();
}

int checkForFile(const char* cmd_line, string* new_cmd_line , string* file_name)
{
    std::string cmd_line_copy(cmd_line);
    std::string file_sign1 = " > ";
    std::string file_sign2 = " >> ";
    int file_sign1_position1 = cmd_line_copy.find(file_sign1);
    int file_sign2_position2 = cmd_line_copy.find(file_sign2);
    if(cmd_line_copy.find(file_sign1) != std::string::npos && cmd_line_copy.find(file_sign2) != std::string::npos) { // there is >> and also >
        if(file_sign1_position1< file_sign2_position2) {
            *new_cmd_line =  cmd_line_copy.substr(0,file_sign1_position1);
            *file_name = _trim(cmd_line_copy.substr(file_sign1_position1+2, strlen(cmd_line)));
            return 0;
        }
        else {
            *new_cmd_line =  cmd_line_copy.substr(0,file_sign2_position2);
            *file_name = _trim(cmd_line_copy.substr(file_sign2_position2+3, strlen(cmd_line)));
            return 1;
        }
    }
    else if (cmd_line_copy.find(file_sign1) == std::string::npos && cmd_line_copy.find(file_sign2) == std::string::npos) { // there is no >> and no >
        std::string file_sign1_last = " >";
        std::string file_sign2_last = " >>";
        if(strlen(cmd_line)>3 && cmd_line_copy.find(file_sign2_last) == cmd_line_copy.length()-3) { // if the last sub-string is " >>"
            *new_cmd_line =  cmd_line_copy.substr(0,cmd_line_copy.length()-3);
            *file_name = _trim(cmd_line_copy.substr(cmd_line_copy.length(), cmd_line_copy.length()));
            return 1;
        }
        if(strlen(cmd_line)>2 && cmd_line_copy.find(file_sign1_last) == cmd_line_copy.length()-2) { // if the last sub-string is is " >"
            *new_cmd_line =  cmd_line_copy.substr(0,cmd_line_copy.length()-2);
            *file_name = _trim(cmd_line_copy.substr(cmd_line_copy.length(), cmd_line_copy.length()));
            return 0;
        }
        file_name = NULL;
        return 2;
    }
    else if (cmd_line_copy.find(file_sign1) == std::string::npos && cmd_line_copy.find(file_sign2) != std::string::npos) // there is >> and no >
    {
        *new_cmd_line = cmd_line_copy.substr(0,file_sign2_position2);
        *file_name = _trim(cmd_line_copy.substr(file_sign2_position2+3, strlen(cmd_line)));
        return 1;
    }
    else { // there is > and no >>
        *new_cmd_line =  cmd_line_copy.substr(0,file_sign1_position1);
        *file_name = _trim(cmd_line_copy.substr(file_sign1_position1+2, strlen(cmd_line)));
        return 0;
    }
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
JobEntry::JobEntry(int job_id, char* cmd_line, pid_t process_id, time_t time_inserted, bool isStopped, int time_up) :
        job_id(job_id), cmd_line(cmd_line), process_id(process_id), time_inserted(time_inserted), isStopped(isStopped),time_up(time_up) {}
JobEntry::~JobEntry() {
    free(cmd_line);
}
void JobEntry::printJob(Command* cmd, int IO_status) {
    if(IO_status == 2) {
        if (this->isStopped) {
            std::cout << "[" << this->job_id << "] " << this->cmd_line << " : " <<
                      this->process_id << " " << difftime(time(NULL), this->time_inserted) << " secs (stopped)" << endl;
        }
        else {
            std::cout << "[" << this->job_id << "] " << this->cmd_line << " : " <<
                      this->process_id << " " << difftime(time(NULL), this->time_inserted) << " secs" << endl;
        }
    }
    else {
        string buff("[");
        char* s_job_id = (char*) malloc(sizeof(this->job_id));
        sprintf(s_job_id, "%d", this->job_id);
        buff.append(s_job_id);
        buff.append("] ");
        char* s_cmd_line = (char*) malloc(sizeof(this->cmd_line));
        sprintf(s_cmd_line, "%s", this->cmd_line);
        buff.append(s_cmd_line);
        buff.append(" : ");
        char* spid = (char*) malloc(sizeof((long)this->process_id));
        sprintf(spid, "%ld", (long)this->process_id);
        buff.append(spid);
        buff.append(" ");
        char* s_time = (char*) malloc(sizeof((long)difftime(time(NULL), this->time_inserted)));
        sprintf(s_time, "%ld", (long)difftime(time(NULL), this->time_inserted));
        buff.append(s_time);
        if (this->isStopped) {
            buff.append(" secs (stopped)");
        }
        else {
            buff.append(" secs");
        }
        buff.append("\n");
        cmd->ChangeIO(IO_status, buff.c_str(), strlen(buff.c_str()));
        free(s_job_id);
        free(s_cmd_line);
        free(spid);
        free(s_time);
    }
}
pid_t JobEntry::getProcessID() {
    return this->process_id;
}
int JobEntry::getJobID() {
    return this->job_id;
}
int JobEntry::getTimeUp(){
    return this->time_up;
}
bool JobEntry::isStoppedProcess() {
    return this->isStopped;
}
time_t JobEntry::getTImeInserted(){
    return this->time_inserted;
}
char* JobEntry::getCmdLine() {
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
//    vector<JobEntry>::iterator it;
//    for(it = jobs_vec->begin(); it != jobs_vec->end(); it++) {
//        jobs_vec->erase(it);
//    }
    //std::cout << "666666" << endl;
    delete jobs_vec;
}
void JobsList::addJob(int job_id, const char* cmd_line, pid_t pid, bool isStopped) {
    JobEntry* job;
    int effective_job_id;
    if (job_id == -1) { // new job (not return from fg)
        effective_job_id = max_job_id + 1;
    }
    else {
        effective_job_id = job_id;
    }
    job = new JobEntry(effective_job_id, strdup(cmd_line), pid, time(NULL), isStopped, -1);
    vector<JobEntry>::iterator it;
    if (jobs_vec->size() == 0) {
        jobs_vec->push_back(*job);
    }
    else if (jobs_vec->front().getJobID() > effective_job_id) {
        jobs_vec->insert(jobs_vec->begin(), *job);
    }
    else if (jobs_vec->back().getJobID() < effective_job_id) {
        jobs_vec->insert(jobs_vec->end(), *job);
    }
    else {
        for (it = jobs_vec->begin() + 1; it != jobs_vec->end(); it++) {
            if ((it - 1)->getJobID() < effective_job_id && it->getJobID() > effective_job_id) {
                jobs_vec->insert(it, *job);
            }
        }
    }
    updateMaxJobID();
    updateMaxStoppedJobID();
}
void JobsList::printJobsList(Command* cmd, int IO_status) {
    vector<JobEntry>::iterator it;
    bool first_print = true;
    for(it = jobs_vec->begin(); it != jobs_vec->end(); it++) {
        if (IO_status == 2) {
            it->printJob(cmd, IO_status);
            first_print = false;
        }
        else {
            if (first_print) {
                it->printJob(cmd, IO_status);
                first_print = false;
            }
            else {
                it->printJob(cmd, 1);
                first_print = false;
            }
        }
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
        vector<JobEntry>::iterator it;
        for(it = jobs_vec->begin(); it != jobs_vec->end(); it++) {
            kidpid = it->getProcessID();
            if (kidpid == 0)
                break;
            if (kill(kidpid, 0) != 0) {
                this->removeJobByProcessId(kidpid);
            }
        }
        // need to do the rows below after every change in the vec
        updateMaxJobID();
        updateMaxStoppedJobID();
    }
}
void JobsList::updateMaxJobID() {
    if (jobs_vec->size() != 0) {
        max_job_id = jobs_vec->back().getJobID(); // back() is the last element in the vec
    }
    else {
        max_job_id = 0;
    }
}
void JobsList::updateMaxStoppedJobID() {
    if (jobs_vec->size() != 0){
        JobEntry* last_stopped = getLastStoppedJob();
        if (last_stopped) // if there is stopped job in the vec
            max_stopped_jod_id = last_stopped->getJobID();
        else
            max_stopped_jod_id = 0;
    }
    else {
        max_stopped_jod_id = 0;
    }
}
std::vector<JobEntry>* JobsList::getJobsVec() {
    return this->jobs_vec;
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
void JobsList::turnToForeground(JobEntry* bg_or_stopped_job, Command* cmd, SmallShell* smash) {
    if (bg_or_stopped_job == NULL) { // something wrong!!
        std::cerr << "something wrong!!" << endl;
    }
    else {
        pid_t job_pid = bg_or_stopped_job->getProcessID();
        int job_id = bg_or_stopped_job->getJobID();
        const char* job_cmd_line = bg_or_stopped_job->getCmdLine();
        if(cmd->getIOStatus() == 2) {
            std::cout << bg_or_stopped_job->getCmdLine() << " : " << job_pid << endl;
        }
        else {
            char* s_cmd_line = (char*) malloc(sizeof(bg_or_stopped_job->getCmdLine()));
            sprintf(s_cmd_line, "%s", bg_or_stopped_job->getCmdLine());
            string buff(s_cmd_line);
            buff.append(" : ");
            char* spid = (char*) malloc(sizeof((long)job_pid));
            sprintf(spid, "%ld", (long)job_pid);
            buff.append(spid);
            buff.append("\n");
            cmd->ChangeIO(cmd->getIOStatus(), buff.c_str(), strlen(buff.c_str()));
            free(s_cmd_line);
            free(spid);
        }
        int kill_status = kill(job_pid, 18); //SIGCONT
        if (kill_status < 0) {
            perror("smash error: kill failed");
            return;
        }
        removeJobByProcessId(job_pid); //remove from vec
        smash->setCurrProcessID(job_pid);
        smash->setCurrJobID(job_id);
        smash->setCurrCmdLine(job_cmd_line);
        pid_t wait_status = waitpid(job_pid, NULL, WUNTRACED);
        if (wait_status < 0) {
            perror("smash error: waitpid failed");
            return;
        }
        smash->setCurrProcessID(getpid());
        smash->setCurrJobID(-1);
        smash->setCurrCmdLine(NULL);
        updateMaxJobID();
        updateMaxStoppedJobID();
    }
}
void JobsList::resumesStoppedJob(JobEntry* stopped_job, Command* cmd) {
    if (stopped_job == NULL) { // something wrong!!
        std::cerr << "something wrong!!" << endl;
    }
    else {
        if(kill(stopped_job->getProcessID(), 18) != -1 ) {// sending signal for job to continue.
            stopped_job->setIsStopped(false);
            if(cmd->getIOStatus() == 2) {
                std::cout << stopped_job->getCmdLine() << " : " << stopped_job->getProcessID() << endl;
            }
            else {
                char* s_cmd_line = (char*) malloc(sizeof(stopped_job->getCmdLine()));
                sprintf(s_cmd_line, "%s", stopped_job->getCmdLine());
                string buff(s_cmd_line);
                buff.append(" : ");
                char* spid = (char*) malloc(sizeof((long)stopped_job->getProcessID()));
                sprintf(spid, "%ld", (long)stopped_job->getProcessID());
                buff.append(spid);
                buff.append("\n");
                cmd->ChangeIO(cmd->getIOStatus(), buff.c_str(), strlen(buff.c_str()));
                free(s_cmd_line);
                free(spid);
            }
            updateMaxJobID();
            updateMaxStoppedJobID();
        }
        else {
            perror("smash error: kill failed");
        }
    }
}
void JobsList::killAllJobs(Command* cmd) {
    if(cmd->getIOStatus() == 2) {
        std::cout << "smash: sending SIGKILL signal to " << jobs_vec->size() << " jobs:" << endl;
    }
    else {
        string buff("smash: sending SIGKILL signal to ");
        char* s_size = (char*) malloc(sizeof((long)jobs_vec->size()));
        sprintf(s_size, "%ld", (long)jobs_vec->size());
        buff.append(s_size);
        buff.append(" jobs:\n");
        cmd->ChangeIO(cmd->getIOStatus(), buff.c_str(), strlen(buff.c_str()));
        free(s_size);
    }
    vector<JobEntry>::iterator it;
    for(it = jobs_vec->begin(); it != jobs_vec->end(); it++) {
        if(cmd->getIOStatus() == 2) {
            std::cout << it->getProcessID() << ": " << it->getCmdLine() << endl;
        }
        else {
            char* spid = (char*) malloc(sizeof((long)it->getProcessID()));
            sprintf(spid, "%ld", (long)it->getProcessID());
            string buff(spid);
            buff.append(": ");
            char* s_cmd_line = (char*) malloc(sizeof(it->getCmdLine()));
            sprintf(s_cmd_line, "%s", it->getCmdLine());
            buff.append(s_cmd_line);
            buff.append("\n");
            cmd->ChangeIO(1, buff.c_str(), strlen(buff.c_str()));
            free(s_cmd_line);
            free(spid);
        }
        int kill_status = kill(it->getProcessID(), 9); //SIGKILL
        if (kill_status < 0) {
            perror("smash error: kill failed");
        }
    }
}
// <---------- END JobsList ------------>

// <---------- START Command ------------>
Command::Command(const char* cmd_line) : cmd_line(cmd_line) {
    string new_cmd_line;
    IO_status = checkForFile(cmd_line,&new_cmd_line ,&file_name);
    if(IO_status == 2) {
        this->args_length = _parseCommandLine(cmd_line, this->args);
        this->cmd_line_without_const = strdup(cmd_line);
    }
    else {
        this->args_length = _parseCommandLine(new_cmd_line.c_str(), this->args);
        this->cmd_line_without_const = strdup(new_cmd_line.c_str());
    }
    _removeBackgroundSign(this->args[this->args_length-1]);
}
Command::~Command() {
    for(int i = 0; i < this->args_length; i++) {
        if (this->args[i] != NULL)
            free(this->args[i]);
    }
    free(this->cmd_line_without_const);
}
const char* Command::getCmdLine() {
    return this->cmd_line;
}
int Command::getIOStatus() {
    return this->IO_status;
}

void Command::ChangeIO(int isAppend, const char* buff = "", int length = 0) {
    int open_fd = 0;
    if (isAppend == 1) {
        open_fd = open(file_name.c_str(), O_WRONLY|O_CREAT|O_APPEND, S_IRWXU|S_IRWXG|S_IRWXO);
    }
    else {
        open_fd = open(file_name.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);
    }
    if (open_fd == -1) {
        perror("smash error: open failed");
        return;
    }
    if(length != 0) {
        if (write(open_fd, buff, length) == -1) {
            perror("smash error: write failed");
            return;
        }
        if (close(open_fd) == -1) {
            perror("smash error: close failed");
        }
    }
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
    if (IO_status ==2) {
        int execv_status = execv("/bin/bash", argv);
        if (execv_status < 0) {
            perror("smash error: execv failed");
        }
    }
    else
    {
        int open_fd;
        if (IO_status == 1) {
            open_fd = open(file_name.c_str(), O_WRONLY|O_CREAT|O_APPEND, S_IRWXU|S_IRWXG|S_IRWXO);
            if (open_fd == -1) {
                perror("smash error: open failed");
                return;
            }
        }
        else {
            open_fd = open(file_name.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);
            if (open_fd == -1) {
                perror("smash error: open failed");
                return;
            }
        }
        dup2(open_fd, 1);
        int execv_status = execv("/bin/bash", argv);
        if (close(open_fd) == -1) {
            perror("smash error: close failed");
        }
        if (execv_status < 0) {
            perror("smash error: execv failed");
        }
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
ShowPidCommand::ShowPidCommand(const char* cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line), smash(smash) {}
void ShowPidCommand::execute(){
    if (IO_status == 2) {
        std::cout << "smash pid is " << smash->getSmashPid() << endl;  // need to check if that is the proper way.
    }
    else {
        string buff = "smash pid is ";
        char* spid = (char*) malloc(sizeof((long)getpid()));
        sprintf(spid, "%ld", (long)getpid());
        buff.append(spid);
        buff.append("\n");
        ChangeIO(IO_status, buff.c_str(), strlen(buff.c_str()));
        free(spid);
    }
}
// <---------- END ShowPidCommand ------------>

// <---------- START GetCurrDirCommand ------------>
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void GetCurrDirCommand::execute() {
    char* curr_dir = getcwd(NULL, 0);
    if(IO_status == 2) {
        std::cout << curr_dir << endl;
    }
    else {
        string buff(curr_dir);
        buff.append("\n");
        ChangeIO(IO_status, buff.c_str(), strlen(buff.c_str()));
    }
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
                char* curr_dir = getcwd(NULL, 0);
                smash->setLastPwd(curr_dir);
                free(curr_dir);
                if(chdir(copy_last_pwd) == -1){
                    perror("smash error: chdir failed");
                    smash->setLastPwd(copy_last_pwd);
                }
                free(copy_last_pwd);
            }
        }
        else { // change to arg[1]
            char* copy_last_pwd = NULL;
            if (smash->isLastPwdInitialized()) {
                copy_last_pwd = (char *) malloc(strlen(smash->getLastPwd()) + 1);
                strcpy(copy_last_pwd, smash->getLastPwd());
            }
            char* curr_dir = getcwd(NULL, 0);
            smash->setLastPwd(curr_dir);
            free(curr_dir);
            if (chdir(args[1]) == -1){
                perror("smash error: chdir failed");
                smash->setLastPwd(copy_last_pwd);
            }
            else {
                smash->changeLastPwdStatus(); // from this point there is last_pwd in the system!!
            }
            free(copy_last_pwd);
        }
    }
}
// <---------- END ChangeDirCommand ------------>

// <---------- START JobsCommand ------------>
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void JobsCommand::execute() {
    jobs->removeFinishedJobs();
    jobs->printJobsList(this, IO_status);
}
// <---------- END JobsCommand ------------>

// <---------- START KillCommand ------------>
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs) {}
void KillCommand::execute() {
    jobs->removeFinishedJobs();
    if(args_length!=3 || atoi(args[1])>-1 || atoi(args[2]) == 0)
    {
        if(IO_status!=2)
            ChangeIO(IO_status);
        std::cerr << "smash error: kill: invalid arguments" << endl;
    }
    else
    {
        JobEntry* job_to_send_signal = jobs->getJobById(atoi(args[2]));
        if(job_to_send_signal == NULL){
            if(IO_status!=2)
                ChangeIO(IO_status);
            std::cerr <<  "smash error: kill: job-id " << args[2] << " does not exist" << endl;
        }
        else
        {
            if (kill(job_to_send_signal->getProcessID(), abs(atoi(args[1]))) != -1) {
                if(IO_status == 2) {
                    std::cout << "signal number " << abs(atoi(args[1])) << " was sent to pid "
                              << job_to_send_signal->getProcessID() << endl;
                }
                else {
                    string buff("signal number ");
                    char* s_signal = (char*) malloc(sizeof(abs(atoi(args[1]))));
                    sprintf(s_signal, "%d", abs(atoi(args[1])));
                    buff.append(s_signal);
                    buff.append(" was sent to pid ");
                    char* spid = (char*) malloc(sizeof((long)job_to_send_signal->getProcessID()));
                    sprintf(spid, "%ld", (long)job_to_send_signal->getProcessID());
                    buff.append(spid);
                    buff.append("\n");
                    ChangeIO(IO_status, buff.c_str(), strlen(buff.c_str()));
                    free(s_signal);
                    free(spid);
                }
            }
            else {
                if(IO_status!=2)
                    ChangeIO(IO_status);
                perror("smash error: kill failed");
            }
        }
    }
}
// <---------- END KillCommand ------------>

// <---------- START ForegroundCommand ------------>
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs, SmallShell* smash) : BuiltInCommand(cmd_line), jobs(jobs), smash(smash) {}
void ForegroundCommand::execute() {
    jobs->removeFinishedJobs();
    if (args_length > 2) { // more than 1 arg
        if(IO_status!=2)
            ChangeIO(IO_status);
        std::cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    else if (args_length == 2) { // one arg
        JobEntry* bg_or_stopped_job = jobs->getJobById(atoi(args[1]));
        if (bg_or_stopped_job == NULL) { // there is no such bg/stopped job with given ID
            if(IO_status!=2)
                ChangeIO(IO_status);
            std::cerr << "smash error: fg: job-id " << atoi(args[1]) << " does not exist" << endl;
            return;
        }
        jobs->turnToForeground(bg_or_stopped_job, this, smash);
    }
    else { // zero arg
        if (jobs->isVecEmpty()) { // there is no jobs in the vec
            if(IO_status!=2)
                ChangeIO(IO_status);
            std::cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
        JobEntry* bg_or_stopped_job = jobs->getJobById(jobs->getMaxJobID());
        jobs->turnToForeground(bg_or_stopped_job, this, smash);
    }
}
// <---------- END ForegroundCommand ------------>

// <---------- START BackgroundCommand ------------>
BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void BackgroundCommand::execute() {
    jobs->removeFinishedJobs();
    if (args_length > 2) { // more than 1 arg
        if(IO_status!=2)
            ChangeIO(IO_status);
        std::cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    else if (args_length == 2) { // one arg
        JobEntry* bg_or_stopped_job = jobs->getJobById(atoi(args[1]));
        if (bg_or_stopped_job == NULL) { // there is no such bg/stopped job with given ID
            if(IO_status!=2)
                ChangeIO(IO_status);
            std::cerr << "smash error: bg: job-id " << atoi(args[1]) << " does not exist" << endl;
            return;
        }
        if (bg_or_stopped_job->isStoppedProcess()) {
            jobs->resumesStoppedJob(bg_or_stopped_job, this);
        }
        else { // the process still running in the background
            if(IO_status!=2)
                ChangeIO(IO_status);
            std::cerr << "smash error: bg: job-id " << atoi(args[1])<< " is already running in the background" << endl;
        }
    }
    else { // zero arg
        JobEntry* last_stopped_job = jobs->getLastStoppedJob();
        if (last_stopped_job == NULL) { // there are no stopped jobs in the vec
            if(IO_status!=2)
                ChangeIO(IO_status);
            std::cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
        else {
            jobs->resumesStoppedJob(last_stopped_job, this);
        }
    }
}
// <---------- END BackgroundCommand ------------>

// <---------- START QuitCommand ------------>
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void QuitCommand::execute() {
    jobs->removeFinishedJobs();
    char sign[] = "kill";
    if (args[1] != NULL && strcmp(args[1], sign) == 0) {
        jobs->killAllJobs(this);
    }
    delete this; //delete command
    exit(0);
}
// <---------- END QuitCommand ------------>

// <---------- START HeadCommand ------------>
HeadCommand::HeadCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void HeadCommand::execute() {
    if(args_length < 2) {
        if(IO_status!=2)
            ChangeIO(IO_status);
        std::cerr << "smash error: head: not enough arguments" << endl;
        return;
    }
    else {
        int line_numbers;
        int open_fd;
        if(args_length == 2) {
            line_numbers = 10; // default value.
            open_fd = open(args[1], O_RDONLY, 0666);
        }
        else {
            line_numbers = abs(atoi(args[1]));
            open_fd = open(args[2], O_RDONLY, 0666);
        }
        if (open_fd == -1) {
            if(IO_status!=2)
                ChangeIO(IO_status);
            perror("smash error: open failed");
            return;
        }
        if (line_numbers == 0) {
            if(IO_status!=2)
                ChangeIO(IO_status);
            return; // stop if we should not print any lines
        }
        int size = 3000;
        char* buff = (char*) malloc(size);
        double lines = 0;
        int i =0;
        int round = 1;
        int status = read(open_fd, &buff[i], 1);
        if(status == -1) {
            if(IO_status!=2)
                ChangeIO(IO_status);
            perror("smash error: read failed");
        }
        if(buff[i] == '\n') {
            lines = lines + 1;
        }
        while(status == 1) {
            i++;
            if(i/round == size) {
                round = round +1;
                buff = (char*)realloc(buff, size*round); // they said almost no limitations on line length (MAX_INT) so array won't work.
            }
            status = read(open_fd, &buff[i], 1);
            if(status == -1) {
                if(IO_status!=2)
                    ChangeIO(IO_status);
                perror("smash error: read failed");
                free(buff);
                return;
            }
            if(buff[i] == '\n' || buff[i] == 0x0) {
                lines = lines + 1;
            }
            if(lines == line_numbers) {
                break;
            }
        }
        if(IO_status == 2) {
            buff = (char*)realloc(buff, i+2);
            write(STDOUT_FILENO,buff, strlen(buff));
        }
        else {
            buff = (char*)realloc(buff, i+2);
            ChangeIO(IO_status, buff, strlen(buff));
        }
        if(close(open_fd) == -1)
            perror("smash error: close failed");
        free(buff);
    }
}
// <---------- END HeadCommand ------------>

// <---------- START SmallShell ------------>
SmallShell::SmallShell() : prompt("smash"), last_pwd(NULL), lastPwdInitialized(false), curr_process_id(getpid()), smash_pid(getpid()) {}
SmallShell::~SmallShell(){
    free(last_pwd);
}
const char* SmallShell::getPrompt(){
    return prompt.c_str();
}
void SmallShell::setPrompt(std::string prompt){
    this->prompt = prompt;
}
std::vector<JobEntry>* SmallShell::getTimeJobVec(){
    return &this->time_jobs_vec;
}
JobsList* SmallShell::getJobsList() {
    return &this->jobs_list;
}
char* SmallShell::getLastPwd(){
    return this->last_pwd;
}
int SmallShell::getCurrJobID(){
    return this->curr_job_id;
}
int SmallShell::getSmashPid(){
    return this->smash_pid;
}
int SmallShell::getCurrProcessID(){
    return this->curr_process_id;
}
const char* SmallShell::getCurrCmdLine() {
    return this->curr_cmd_line;
}
const char* SmallShell::getLastCmd(){
    return last_cmd.c_str();
}
void SmallShell::setLastPwd(const char* update_last_pwd) {
    free(this->last_pwd);
    if (update_last_pwd)
        this->last_pwd = strdup(update_last_pwd);
    else
        this->last_pwd = NULL;
}
void SmallShell::setCurrJobID(int job_id) {
    this->curr_job_id = job_id;
}
void SmallShell::setCurrProcessID(int pid) {
    this->curr_process_id = pid;
}
void SmallShell::setCurrCmdLine(const char* cmd_line) {
    this->curr_cmd_line = cmd_line;
}
bool SmallShell::isLastPwdInitialized() {
    return this->lastPwdInitialized;
}
void SmallShell::changeLastPwdStatus() {
    this->lastPwdInitialized = true;
}
int SmallShell::findMinAlarm(){
    if(time_jobs_vec.empty())
        return -1;
    vector<JobEntry>::iterator it;
    vector<JobEntry>::iterator min = time_jobs_vec.begin();
    for(it = time_jobs_vec.begin(); it != time_jobs_vec.end(); it++) {
        if (it->getTimeUp() - difftime(time(NULL), it->getTImeInserted()) < min->getTimeUp() - difftime(time(NULL), min->getTImeInserted())) {
            min = it;
        }
    }
    return (min->getTimeUp() - difftime(time(NULL), min->getTImeInserted()));
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
        return new ShowPidCommand(cmd_line, this);
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
        return new ForegroundCommand(cmd_line, &jobs_list, this);
    }
    else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line, &jobs_list);
    }
    else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line, &jobs_list);
    }
    else if (firstWord.compare("head") == 0) {
        return new HeadCommand(cmd_line, &jobs_list);
    }
    else {
        bool isBackground = _isBackgroundComamnd(cmd_line);
        pid_t pid = fork();
        if (pid == 0) { //child
            setpgrp();
            if (_isTimeCommand(cmd_line)) {
                char* tmp_args[COMMAND_MAX_ARGS];
                int args_length = _parseCommandLine(cmd_line,tmp_args);
                std::string new_cmd_line = removeTimeOut(cmd_line, tmp_args[1]);
                for(int i = 0; i < args_length; i++) {
                    free(tmp_args[i]);
                }
                return new ExternalCommand(new_cmd_line.c_str(), &jobs_list);
            }
            return new ExternalCommand(cmd_line, &jobs_list);
        } else if (pid > 0) { //parent
            if (isBackground == false) {
                this->curr_process_id = pid;
                this->curr_cmd_line = cmd_line;
                this->curr_job_id = -1;
                pid_t wait_status = waitpid(pid, NULL, WUNTRACED);
                if (wait_status < 0) {
                    perror("smash error: waitpid failed");
                }
                this->curr_process_id = getpid();
                this->curr_cmd_line = NULL;
                this->curr_job_id = -1;
            } else {
                jobs_list.removeFinishedJobs(); // if we are going to add to the vec so remove jobs from the shell process (father for all the bg commands)
                jobs_list.addJob(-1, cmd_line, pid, false);
                if (_isTimeCommand(cmd_line)) {
                    char* tmp_args[COMMAND_MAX_ARGS];
                    int args_length = _parseCommandLine(cmd_line,tmp_args);
                    JobEntry* job = new JobEntry(-1, strdup(cmd_line), pid, time(NULL), false, atoi(tmp_args[1]));
                    time_jobs_vec.push_back(*job);
                    alarm(findMinAlarm());
                    for(int i = 0; i < args_length; i++) {
                        free(tmp_args[i]);
                    }
                }
            }
        } else {
            perror("smash error: fork failed");
        }
    }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    int pipe_status = _isPipeCommand(cmd_line);
    if (pipe_status > 0) { // pipe
        jobs_list.removeFinishedJobs();
        std::string left;
        std::string right;
        _splitPipeCommands(cmd_line, &left, &right);
        int pipe_write_channel;
        if (pipe_status == 1) {
            pipe_write_channel = STDOUT_FILENO;
        }
        else { // pipe_status==2
            pipe_write_channel = STDERR_FILENO;
        }
        int pipe_arr[2] = {0};
        pid_t pid = fork();
        if (pid == 0) { //child
            setpgrp();
            if (pipe(pipe_arr) == -1) {
                perror("smash error: pipe failed");
            }
            else {
                pid_t pipe_pid = fork();
                if (pipe_pid == 0) { //child - left command - write
                    setpgrp();
                    if (dup2(pipe_arr[1], pipe_write_channel) == -1) {
                        perror("smash error: dup2 failed");
                    }
                    else {
                        if (close(pipe_arr[0]) == -1) {
                            perror("smash error: close failed");
                        } else {
                            executeCommand(left.c_str());
                        }
                    }
                    if (close(pipe_arr[1]) == -1) {
                        perror("smash error: close failed");
                    }
                    if (close(pipe_write_channel) == -1) {
                        perror("smash error: close failed");
                    }
                } else if (pipe_pid > 0) { //parent - right command - read
                    if (dup2(pipe_arr[0], STDIN_FILENO) == -1) {
                        perror("smash error: dup2 failed");
                    }
                    else {
                        if (close(pipe_arr[1]) == -1) {
                            perror("smash error: close failed");
                        } else {
                            executeCommand(right.c_str());
                        }
                    }
                    if (close(pipe_arr[0]) == -1) {
                        perror("smash error: close failed");
                    }
                    if (close(STDIN_FILENO) == -1) {
                        perror("smash error: close failed");
                    }
                } else {
                    perror("smash error: fork failed");
                }
            }
        } else if (pid > 0) { //parent - smash
            pid_t wait_status1 = wait(NULL);
            if (wait_status1 < 0) {
                perror("smash error: wait failed");
            }
            wait(NULL);
            wait(NULL);
        } else {
            perror("smash error: fork failed");
        }
    }
    else {
        if (_isTimeCommand(cmd_line) && !_isBackgroundComamnd(cmd_line)) {
            char *tmp_args[COMMAND_MAX_ARGS];
            int args_length = _parseCommandLine(cmd_line, tmp_args);
            std::string new_cmd_line = removeTimeOut(cmd_line, tmp_args[1]);
            alarm(atoi(tmp_args[1]));
            string last_cmd_str(cmd_line);
            last_cmd = last_cmd_str;
            for(int i = 0; i < args_length; i++) {
                free(tmp_args[i]);
            }
            executeCommand(new_cmd_line.c_str());
        } else {
            Command *cmd = CreateCommand(cmd_line);
            if (cmd != NULL) {
                cmd->execute();
                delete cmd;
            }
        }
    }
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}