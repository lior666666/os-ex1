#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <string.h>
#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (21)

class Command;
class JobEntry {
    int job_id;
    const char* cmd_line;
    pid_t process_id;
    time_t time_inserted;
    bool isStopped;
public:
    JobEntry(int job_id, const char* cmd_line, pid_t process_id, time_t time_inserted, bool isStopped);
    ~JobEntry();
    void printJob(Command* cmd, int IO_status);
    int getJobID();
    pid_t getProcessID();
    bool isStoppedProcess();
    void setIsStopped(bool setStopped);
    const char* getCmdLine();
};

class JobsList {
    std::vector<JobEntry>* jobs_vec;
    int max_job_id;
    int max_stopped_jod_id;
public:
    JobsList();
    ~JobsList();
    void addJob(const char* cmd_line, pid_t pid, bool isStopped = false);
    void printJobsList(Command* cmd, int IO_status);
    void removeFinishedJobs();
    void updateMaxJobID();
    void updateMaxStoppedJobID();
    JobEntry* getJobById(int jobId);
    JobEntry* getJobByProcessId(pid_t process_id);
    void removeJobByProcessId(pid_t process_to_delete);
    JobEntry* getLastStoppedJob();
    bool isVecEmpty();
    int getMaxJobID();
    int getMaxStoppedJobID();
    void turnToForeground(JobEntry* bg_or_stopped_job, Command* cmd);
    void resumesStoppedJob(JobEntry* stopped_job, Command* cmd);
    void killAllJobs(Command* cmd);
};

class Command {
protected:
    const char* cmd_line;
    char* cmd_line_without_const;
    char* args[COMMAND_MAX_ARGS];
    std::string file_name;
    int IO_status;
    int args_length;
public:
    Command(const char* cmd_line);
    const char* getCmdLine();
    int getIOStatus();
    void ChangeIO(int isAppend, const char* buff, int length);
    virtual ~Command();
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
    JobsList* jobs;
public:
    ExternalCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class SmallShell;
class ChangePromptCommand : public BuiltInCommand {
    SmallShell* smash;
public:
    ChangePromptCommand(const char* cmd_line, SmallShell* smash);
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    SmallShell* smash;
public:
    ChangeDirCommand(const char* cmd_line, SmallShell* smash);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class HeadCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    HeadCommand(const char* cmd_line, JobsList* jobs);
    virtual ~HeadCommand() {}
    void execute() override;
};


class SmallShell {
private:
    JobsList jobs_list;
    const char* prompt;
    const char* last_pwd;
    bool lastPwdInitialized;
    int curr_job_id;
    SmallShell();
public:
    Command *CreateCommand(const char* cmd_line);
    const char* getPrompt();
    const char* getLastPwd();
    int getCurrJobID();
    bool isLastPwdInitialized();
    void setPrompt(const char* prompt);
    void setLastPwd(const char* last_pwd);
    void setCurrJobID(int job_id);
    void changeLastPwdStatus();
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_