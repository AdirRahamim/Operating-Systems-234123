#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
#include <vector>

using namespace std;
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
protected:
    string command;
    char* arguments[COMMAND_ARGS_MAX_LENGTH];
    int num_arguments;

 public:
  explicit Command(const char* cmd_line);
  virtual ~Command(){
      for(int i = 0; i<num_arguments; i++){
          free(arguments[i]);
      }
  }
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  explicit BuiltInCommand(const char* cmd_line);
  ~BuiltInCommand() override =default;
};

class JobsList;
class ExternalCommand : public Command {
 JobsList* jobs_list;
 public:
  explicit ExternalCommand(const char* cmd_line, JobsList* jobs) : Command(cmd_line), jobs_list(jobs) {};
  virtual ~ExternalCommand() = default;
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
  JobsList* jobs_list;
 public:
  PipeCommand(const char* cmd_line, JobsList* jobs) : Command(cmd_line), jobs_list(jobs) {};
  virtual ~PipeCommand() = default;
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 JobsList* jobs_list;
 public:
  explicit RedirectionCommand(const char* cmd_line, JobsList* jobs) : Command(cmd_line), jobs_list(jobs) {};
  virtual ~RedirectionCommand() = default;
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
  string& last_PWD;
public:
  ChangeDirCommand(const char *cmd_line, string& plastPwd) : BuiltInCommand(cmd_line), last_PWD(plastPwd){};
  virtual ~ChangeDirCommand() = default;
  void execute() override;
};

// This is the PWD command
class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
  virtual ~GetCurrDirCommand() = default;
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
  virtual ~ShowPidCommand()  = default;
  void execute() override;
};

class QuitCommand : public BuiltInCommand {
    JobsList* jobs_list;
// TODO: Add your data members public:
public:
  QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs_list(jobs) {};
  virtual ~QuitCommand() = default;
  void execute() override;
};


class JobsList {
 public:
    enum JobStat {Running, Stopped};
    class JobEntry {
   // TODO: Add your data members

    string cmd_line;
    int id_job;
    pid_t pid_job;
    time_t start_time;
    JobStat status;

  public:
      JobEntry(string cmd_line, int id, pid_t pid, time_t time, JobStat status) : cmd_line(cmd_line), id_job(id),
        pid_job(pid), start_time(time), status(status) {}

      ~JobEntry() = default;

      bool operator< (const JobEntry& job) const{
          return (id_job < job.id_job);
      }

      time_t getStartTime(){
          return start_time;
      }

      int getJobId(){
          return id_job;
      }

      string getCmd(){
          return cmd_line;
      }

      pid_t getJobPid(){
          return pid_job;
      }

      JobStat getJobStatus(){
          return status;
      }

      void setJobStatus(JobStat new_status){
          status = new_status;
      }
  };
 // TODO: Add your data members
private:
    int max_id;
    string fg_command;
    pid_t fg_pid;
    vector<JobEntry> jobs;

 public:
    JobsList() : max_id(0), fg_command(""), fg_pid(-1) {};
  ~JobsList() = default;
  void addJob(string cmd, pid_t pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob();
  JobEntry *getLastStoppedJob();
  int getJobsSize();
  void fgJob(int jobId);
  void setFg(string cmd, pid_t pid);
  string getFgCmd();
  pid_t getFgPid();
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList* jobs_list;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs_list(jobs) {};
  virtual ~JobsCommand() = default;
  void execute() override;
};

class KillCommand : public BuiltInCommand {
  JobsList* jobs_list;
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs_list(jobs) {};
  virtual ~KillCommand() = default;
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 JobsList* jobs_list;
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs_list(jobs) {};
  virtual ~ForegroundCommand() = default;
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 JobsList* jobs_list;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs_list(jobs) {};
  virtual ~BackgroundCommand() = default;
  void execute() override;
};

class ChpromptCommand : public BuiltInCommand {
    // TODO: Add your data members
    string& prompt;
public:
    ChpromptCommand(const char* cmd_line, string& prompt) : BuiltInCommand(cmd_line), prompt(prompt){};
    virtual ~ChpromptCommand() = default;
    void execute() override;
};


// TODO: should it really inhirit from BuiltInCommand ?
class CopyCommand : public Command {
    JobsList* jobs_list;
 public:
  CopyCommand(const char* cmd_line, JobsList* jobs) : Command(cmd_line), jobs_list(jobs) {};
  virtual ~CopyCommand() = default;
  void execute() override;
};

// TODO: add more classes if needed 
// maybe chprompt , timeout ?

class SmallShell {
 private:
  // TODO: Add your data members
  string prompt;
  string last_pwd;
  JobsList* jobs_list;
  SmallShell();
 public:

  Command *CreateCommand(const char* cmd_line);
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
  string getPrompt(){
      return prompt;
  }
  JobsList* getJobsList(){
      return jobs_list;
  }
};

#endif //SMASH_COMMAND_H_
