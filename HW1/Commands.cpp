#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <algorithm>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

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
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

void _printError(const string err){
    cout << "smash error: " << err << endl;
}

// TODO: Add your implementation for classes in Commands.h 

void ExternalCommand::execute() {

}

SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}


/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
/*
  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
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
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

Command::Command(const char *cmd_line) {
    command = string(cmd_line);
    num_arguments = _parseCommandLine(cmd_line, arguments);
}

void ChangeDirCommand::execute() {
    char* path = this->arguments[1];
    if(this->num_arguments>2){
        _printError("cd: too many arguments");
    }
    else if(!strcmp(path, "-") && this->last_PWD.empty()){
        _printError("cd: OLDPWD not set");
    }
    else{
        string curr_PWD = getcwd(nullptr, COMMAND_ARGS_MAX_LENGTH);
        int res = 0;
        if(!strcmp(path, "-")){
            //Return to last path
            res = chdir(last_PWD.c_str());
        }
        else{
            //Change to new path
            res = chdir(path);
        }
        if(res == -1){
            //Error...
            perror("smash error: cd failed");
            return;
        }
        else{
            //Succeed so save last path
            last_PWD = curr_PWD;
        }
    }
}

void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid() << endl;
}

void GetCurrDirCommand::execute() {
    cout << getcwd(nullptr, COMMAND_ARGS_MAX_LENGTH) << endl;
}

void JobsList::addJob(string cmd, bool isStopped, pid_t pid) {
    removeFinishedJobs();
    max_id++;
    JobStat status = isStopped ? Stopped : Running;
    JobEntry newEntry = JobEntry(cmd, max_id, pid, time(nullptr), status);
    jobs.push_back(newEntry);
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    //Sort vector elements according to job ID
    sort(jobs.begin(),jobs.end());

    for(JobEntry currJob : jobs){
        //Calculate running time
        time_t time_running = time(nullptr) - currJob.getStartTime();

        cout << "[" << currJob.getJobId() << "] " << currJob.getCmd() << " : " << currJob.getJobPid() << " ";
        cout << time_running << " secs";
        if(currJob.getJobStatus() == Stopped){
            cout << " (stopped)";
        }
        cout << endl;
    }
}

void JobsList::killAllJobs() {
    removeFinishedJobs();
    for(JobEntry currJob : jobs){
        pid_t currPid = currJob.getJobPid();
        if(kill(currPid, SIGKILL) == -1){
            perror("smash error: kill failed");
        }
        else{
            cout << currPid <<": " << currJob.getCmd() <<endl;
        }
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for(JobEntry currJob : jobs){
        if(currJob.getJobId() == jobId){
            //Found job
            return &currJob;
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    int i=0;
    for(JobEntry currJob : jobs){
        if(currJob.getJobId() == jobId){
            //Found job to remove
            jobs.erase(jobs.begin()+i);
            break;
        }
        i++;
    }
    if(jobs.empty()){
        max_id = 0;
    }
    else{
        max_id = getLastJob()->getJobId();
    }

}

JobsList::JobEntry *JobsList::getLastJob() {
    JobEntry* lastJob = nullptr;
    int last_id = 0;
    for(JobEntry currJob : jobs){
        if(currJob.getJobId() > last_id){
            last_id = currJob.getJobId();
            lastJob = &currJob;
        }
    }
    return lastJob;
}

JobsList::JobEntry *JobsList::getLastStoppedJob() {
    JobEntry* lastJob = nullptr;
    int last_id = 0;
    for(JobEntry currJob : jobs){
        if(currJob.getJobId() > last_id && currJob.getJobStatus() == Stopped){
            last_id = currJob.getJobId();
            lastJob = &currJob;
        }
    }
    return lastJob;
}

void JobsList::removeFinishedJobs() {
    int res =  0;
    pid_t pid;

    auto it = jobs.begin();
    while(it != jobs.end()){
        pid = it->getJobPid();
        res = waitpid(pid, nullptr, WHOHANG);

        if(res == -1){
            perror("smash error: waitpid fai;ed");
            continue;
        }
        else if(res == pid){
            it = jobs.erase(it);
        }
        else{
            it++;
        }
        if(jobs.empty()){
            max_id = 0;
        }
        else{
            max_id = getLastJob()->getJobId();
        }
    }
}

void JobsCommand::execute() {
    jobs_list->printJobsList();
}

bool checkNumber(string s){
    for(i=0 ; i<s.length() ; i++){
        if(!isdigit(s[i])){
            return false;
        }
    }
    return true;
}

void KillCommand::execute() {
    jobs_list->removeFinishedJobs();

    //Right number of arguments
    if(num_arguments != 3){
        _printError("kill: invalid arguments");
        return;
    }
    string signum = arguments[1];
    string job_id = arguments[2];
    if(arguments[1][0] != '-' || !checkNumber(signum.substr(1)) || !checkNumber(job_id)){
        _printError("kill: invalid arguments");
    }


}
