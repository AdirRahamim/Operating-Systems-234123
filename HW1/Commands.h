#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
#include <vector>
#include <unistd.h>
#include <algorithm>
#include <sys/wait.h>


using namespace std;
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
protected:
    string command;
    //char* arguments[COMMAND_ARGS_MAX_LENGTH];
    vector<string> arguments;
    int num_arguments;

 public:
  explicit Command(const char* cmd_line);
  virtual ~Command(){
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
    int pid;
 public:
  ShowPidCommand(const char* cmd_line, int pid) : BuiltInCommand(cmd_line), pid(pid) {};
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

      void resetTime(){
          start_time = time(nullptr);
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

      void setJobStatus(bool new_status){
          status = (new_status == true) ? Running : Stopped;
      }
  };
 // TODO: Add your data members
private:
    int max_id;
    string fg_command;
    pid_t fg_pid;
    bool isBgToFg;
    int fgId;
    vector<JobEntry> jobs;

 public:
    JobsList() : max_id(0), fg_command(""), fg_pid(-1) , isBgToFg(false), fgId(-1){};
  ~JobsList() = default;
  int addJob(string cmd, pid_t pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob();
  JobEntry *getLastStoppedJob();
  int getJobsSize();
  void fgJob(int jobId);
  void bgJob(int jobId);
  void setFg(string cmd, pid_t pid, int jobId);
  string getFgCmd();
  pid_t getFgPid();
  void resetTime(int jobId);
  void setBgToFg(bool update);
  bool getBgToFg();
  void setStop(int jobId);
  int getFgId();
  // TODO: Add extra methods or modify exisitng ones as needed
};

class TimeoutList{
public:
    class TimeoutJobs{
        string cmd;
        int job_id;
        time_t start_time;
        int duration;
        pid_t job_pid;
    public:
        TimeoutJobs(string cmd, int job_id, int duration, pid_t pid) : cmd(cmd), job_id(job_id) , duration(duration), job_pid(pid){
            start_time = time(nullptr);
        }
        ~TimeoutJobs() = default;

        bool operator< (const TimeoutJobs& job) const{
            return (duration < job.duration);
        }

        int getDuration() const {
            return duration;
        }

        void updateDuration(int update){
            duration = update;
        }

        void resetTime(){
            start_time = time(nullptr);
        }

        time_t getStartTime() const{
            return start_time;
        }

        string getCmd(){
            return cmd;
        }

        pid_t getPid() const{
            return job_pid;
        }

        int getJobId(){
            return job_id;
        }

    };
private:
    vector<TimeoutJobs> timeout_vec;
    JobsList* jobs;
public:
    TimeoutList(JobsList* jobs) : jobs(jobs){};

    void addJob(string cmd, int job_id, int duration, pid_t pid){
        TimeoutJobs job = TimeoutJobs(cmd, job_id, duration, pid);
        timeout_vec.push_back(job);
    }

    bool checkAreFinished() {
        auto it = timeout_vec.begin();
        while (it != timeout_vec.end()) {
            if (it->getDuration() <= 0) {
                if (jobs->getJobById(it->getJobId()) == nullptr && jobs->getFgPid() != it->getPid()) {
                    //Jobs already finished..
                    it = timeout_vec.erase(it);
                    continue;
                } else {
                    //Found unfinished job
                    return true;
                }
            }
            return false;
        }
    }
    void removeTimeoutJobs(){
        auto it = timeout_vec.begin();
        while(it != timeout_vec.end()){
            if(it->getDuration() <= 0){
                cout << "smash: " << it->getCmd() << " timed out!" << endl;
                if(kill(it->getPid(), SIGKILL) == -1){
                    perror("smash error: kill failed");
                }
                it = timeout_vec.erase(it);
            }
            else{
                it++;
            }
        }
    }

    void InsertAndCall(string cmd, int job_id, int duration, pid_t pid){
        pid_t current_pid = -1;
        if(!timeout_vec.empty()){
            current_pid = timeout_vec.begin()->getPid();
        }
        TimeoutJobs job = TimeoutJobs(cmd, job_id, duration, pid);
        timeout_vec.push_back(job);
        sort(timeout_vec.begin(), timeout_vec.end());
        if(current_pid == -1){
            alarm(duration);
            return;
        }
        //current_pid != -1
        if(current_pid != timeout_vec.begin()->getPid()){
            alarm(timeout_vec.begin()->getDuration());
        }
        //Dont call now new alarm because we entered alarm after the current one...
    }

    void update(){
        auto it = timeout_vec.begin();
        int time_diff = 0;
        while(it != timeout_vec.end()){
            time_diff = time(nullptr) - it->getStartTime();
            it->updateDuration(it->getDuration()-time_diff);
            it->resetTime();
            it++;

        }
    }

    void callNext(){
        sort(timeout_vec.begin(), timeout_vec.end());
        if(!timeout_vec.empty()){
            auto it = timeout_vec.begin();
            alarm(it->getDuration());
        }
    }

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

class CopyCommand : public Command {
    JobsList* jobs_list;
 public:
  CopyCommand(const char* cmd_line, JobsList* jobs) : Command(cmd_line), jobs_list(jobs) {};
  virtual ~CopyCommand() = default;
  void execute() override;
};

// TODO: add more classes if needed 
// maybe chprompt , timeout ?

class TimeoutCommand : public Command {
    JobsList* jobs_list;
    TimeoutList* timeout_list;
public:
    TimeoutCommand(const char* cmd_line, JobsList* jobs, TimeoutList* timeout_list) : Command(cmd_line), jobs_list(jobs),
        timeout_list(timeout_list) {};
    virtual ~TimeoutCommand() = default;
    void execute() override;
};

class SmallShell {
    // TODO: Add your data members
  string prompt;
  string last_pwd;
  JobsList* jobs_list;
  TimeoutList* timeout_list;
  SmallShell();
  int pid;
  vector<TimeoutList::TimeoutJobs> alarms;

 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    instance.pid = getpid();
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

  TimeoutList* getTimeoutList(){
      return timeout_list;
  }

};

#endif //SMASH_COMMAND_H_
