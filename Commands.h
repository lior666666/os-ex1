#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <string.h>
#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (21)

class Command {
 protected:
    const char* cmd_line;
    char* cmd_line_without_const;
    char* args[COMMAND_MAX_ARGS];
    int args_length;
 public:
  Command(const char* cmd_line);
  const char* getCmdLine();
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
 public:
  ExternalCommand(const char* cmd_line);
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

class JobsList;
class QuitCommand : public BuiltInCommand {
    JobsList* jobs;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class JobEntry {
    int job_id;
    const char* cmd_line;
    pid_t process_id;
    time_t time_inserted;
    bool isStopped;
 public:
    JobEntry(int job_id, const char* cmd_line, pid_t process_id, time_t time_inserted, bool isStopped);
    ~JobEntry();
    void printJob();
    int getJobID();
    pid_t getProcessID();
    bool isStoppedProcess();
    const char* getCmdLine();
};

class JobsList {
    std::vector<JobEntry>* jobs_vec;
    int max_job_id;
    int max_stopped_jod_id;
 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList();
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
  void turnToForeground(JobEntry* bg_or_stopped_job);
  void killAllJobs();
};

class JobsCommand : public BuiltInCommand {
    JobsList* jobs;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
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
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class HeadCommand : public BuiltInCommand {
 public:
  HeadCommand(const char* cmd_line);
  virtual ~HeadCommand() {}
  void execute() override;
};


class SmallShell {
 private:
    JobsList jobs_list;
    const char* prompt;
    const char* last_pwd;
    bool lastPwdInitialized;
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  const char* getPrompt();
  const char* getLastPwd();
  bool isLastPwdInitialized();
  void setPrompt(const char* prompt);
  void setLastPwd(const char* last_pwd);
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
