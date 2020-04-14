#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <algorithm>
#include <fcntl.h>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#define BASH_PATH "/bin/bash"

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

bool checkNumber(string s){
    for(unsigned int i = 0 ; i < s.length() ; i++){
        if((i == 0) && (s[i] == '-')) continue;
        if(!isdigit(s[i])){
            return false;
        }
    }
    return true;
}

void _printError(const string err){
    cout << "smash error: " << err << endl;
}

// TODO: Add your implementation for classes in Commands.h

void CopyCommand::execute() {
    if(num_arguments < 3 || (num_arguments == 4 && arguments[3][0] != '&') || num_arguments > 4){
        _printError("cp: invalid arguments");
        return;
    }
    bool is_bg = false;
    string new_command = command;
    if(_isBackgroundComamnd(command.c_str())){
        //Ends with &
        is_bg = true;

        char temp[COMMAND_ARGS_MAX_LENGTH];
        strcpy(temp, command.c_str());
        _removeBackgroundSign(temp);
        new_command = string(temp);
    }

    int read_file = open(arguments[1], O_RDONLY);
    //00int read_file = open("/home/adir/Downloads/טכניון/Operating-Systems-234123/HW1/test_src_1.txt", O_RDONLY);
    if(read_file == -1){
        perror("smash error: open failed");
        return;
    }
    int write_file = open(arguments[2], O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if(write_file == -1){
        perror("smash error: open failed");
        if(close(read_file) == -1){
            perror("smash error: close failed");
        }
        return;
    }

    //Create buffer to read data
    vector<char> buffer(1024);

    int pid = fork();

    if( pid < 0){
        perror("smash error: fork failed");
        return;
    }
    else if(pid == 0) {
        //Child process
        //setpgrp();

        while(true){
            ssize_t read_size = read(read_file, buffer.data(), buffer.size());
            if(read_size == 0){
                //Reach EOF
                break;
            }
            else if(read_size == -1){
                perror("smash error: read failed");
                if(close(read_file) == -1 || close(write_file) == -1){
                    perror("smash error: close failed");
                }
                return;
            }

            ssize_t size_wrrited = 0;
            while(size_wrrited<read_size){
                ssize_t write_res = write(write_file, buffer.data()+ size_wrrited, read_size - size_wrrited);
                if(write_res == -1){
                    perror("smash error: write failed");
                    if(close(read_file) == -1 || close(write_file) == -1){
                        perror("smash error: close failed");
                    }
                    return;
                }
                else{
                    size_wrrited += write_res;
                }
            }
        }
        if(close(read_file) == -1 || close(write_file) == -1){
            perror("smash error: close failed");
        }
        cout << "smash: " << arguments[1] << " was copied to " << arguments[2] << endl;
        exit(0);
    }
    else{
        if(is_bg){
            jobs_list->addJob(command,pid, false);
            return; //No need to wait...
        }
        else{
            jobs_list->setFg(command, pid);
            int res = waitpid(pid, nullptr, WUNTRACED);
            if(res == -1){
                perror("smash error: waitpid failed");
                return;
            }
            jobs_list->setFg("", -1);
        }
    }


}

void TimeoutCommand::execute() {

    if(num_arguments == 1){
        _printError("timeout: invalid arguments");
        return;
    }
    else if(!checkNumber(arguments[1])){
        _printError("timeout: invalid arguments");
        return;
    }

    int timeout_time = atoi(arguments[1]);

    string new_command;

    for(int i = 2 ; i<num_arguments ; i++){
        new_command + " " + string(arguments[i]);
    }
    _trim(new_command);

    bool is_bg = false;
    if(_isBackgroundComamnd(command.c_str())){
        //Ends with &
        is_bg = true;

        char temp[COMMAND_ARGS_MAX_LENGTH];
        strcpy(temp, new_command.c_str());
        _removeBackgroundSign(temp);
        new_command = string(temp);
    }

    SmallShell& temp_smash = SmallShell::getInstance();
    auto _command = temp_smash.CreateCommand(cmd.c_str());

    int pid = fork();

    if( pid < 0){
        perror("smash error: fork failed");
        return;
    }
    else if(pid == 0){
        //Child
        setpgrp();
        _command->execute();
        exit(0);
    }
    else{
        //Parent
        if(is_bg){
            int job_id = jobs_list->addJob(cmd,pid, false);
            temp_smash.addTimeoutJob(command, job_id, timeout_time, pid);
            return; //No need to wait...
        }
        else{
            jobs_list->setFg(command, pid);
            temp_smash.addTimeoutJob(command, -1, timeout_time, pid);
            int res = waitpid(pid, nullptr, WUNTRACED);
            if(res == -1){
                perror("smash error: waitpid failed");
                return;
            }
            if(close(file) == -1){
                perror("smash error: close failed");
                return;
            }
            jobs_list->setFg("", -1);
        }

    }

}

void RedirectionCommand::execute() {
    bool is_bg = false;
    string new_command = command;
    if(_isBackgroundComamnd(command.c_str())){
        //Ends with &
        is_bg = true;

        char temp[COMMAND_ARGS_MAX_LENGTH];
        strcpy(temp, command.c_str());
        _removeBackgroundSign(temp);
        new_command = string(temp);
    }

    //Produce first and second command
    int redirect = new_command.find('>');
    bool isAppend = new_command[redirect+1] == '>';
    string cmd = new_command.substr(0,redirect);
    string file_path = new_command.substr(redirect+1+(int)isAppend);
    int file;
    if(isAppend){
        file = open(file_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0664);
        if(file == -1){
            perror("smash error: open failed");
            return;
        }
    }
    else {
        file = open(file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
        if (file == -1) {
            perror("smash error: open failed");
            return;
        }
    }
    SmallShell& temp_smash = SmallShell::getInstance();
    auto _command = temp_smash.CreateCommand(cmd.c_str());

    int pid = fork();

    if( pid < 0){
        perror("smash error: fork failed");
        return;
    }
    else if(pid == 0){
        //Child process
        setpgrp();
        if(dup2(file,STDOUT_FILENO) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        _command->execute();
        exit(0);
    }
    else{
        if(is_bg){
            jobs_list->addJob(cmd,pid, false);
            return; //No need to wait...
        }
        else{
            jobs_list->setFg(command, pid);
            int res = waitpid(pid, nullptr, WUNTRACED);
            if(res == -1){
                perror("smash error: waitpid failed");
                return;
            }
            if(close(file) == -1){
                perror("smash error: close failed");
                return;
            }
            jobs_list->setFg("", -1);
        }
    }
}

void PipeCommand::execute() {
    bool is_bg = false;
    string new_command = command;
    if(_isBackgroundComamnd(command.c_str())){
        //Ends with &
        is_bg = true;

        char temp[COMMAND_ARGS_MAX_LENGTH];
        strcpy(temp, command.c_str());
        _removeBackgroundSign(temp);
        new_command = string(temp);
    }

    //Produce first and second command
    int pipeIndex = new_command.find('|');
    bool isStderrPipe = new_command[pipeIndex+1] == '&';
    string cmd1 = new_command.substr(0,pipeIndex);
    string cmd2 = new_command.substr(pipeIndex+1+(int)isStderrPipe);

    SmallShell& temp_smash = SmallShell::getInstance();
    auto command1 = temp_smash.CreateCommand(cmd1.c_str());
    auto command2 = temp_smash.CreateCommand(cmd2.c_str());

    //Create pipe
    int fd[2];
    if(pipe(fd) == -1){
        perror("smash error: pipe failed");
        return;
    }

    pid_t pid = fork();
    if( pid < 0){
        perror("smash error: fork failed");
        return;
    }
    else if(pid == 0){
        //Child process
        setpgrp();
        if(isStderrPipe){
            if(dup2(fd[1],STDERR_FILENO) == -1){
                perror("smash error: dup2 failed");
                return;
            }
        }
        else{
            if(dup2(fd[1],STDOUT_FILENO) == -1){
                perror("smash error: dup2 failed");
                return;
            }
        }
        close(fd[0]);
        close(fd[1]);
        command1->execute();
        exit(0);
    }
    else{
        //Parent process
        int pid2 = fork();
        if( pid2 < 0){
            perror("smash error: fork failed");
            return;
        }
        else if(pid2 == 0){
            if(dup2(fd[1],STDIN_FILENO) == -1){
                perror("smash error: dup2 failed");
                return;
            }
            close(fd[0]);
            close(fd[1]);
            command2->execute();
            exit(0);
        }
        else{
            close(fd[0]);
            close(fd[1]);
            if(is_bg){
                jobs_list->addJob(cmd1, pid, false);
                jobs_list->addJob(cmd2,pid2, false);
                return; //No need to wait...
            }
            else{
                jobs_list->setFg(command, pid);
                int res1 = waitpid(pid, nullptr, WUNTRACED);
                int res2 = waitpid(pid2, nullptr, WUNTRACED);
                if(res1 == -1 || res2 == -1){
                    perror("smash error: waitpid failed");
                    return;
                }
                jobs_list->setFg("",-1);
            }
        }
    }
}

void ExternalCommand::execute() {
    bool is_bg = false;
    string new_command = command;
    if(_isBackgroundComamnd(command.c_str())){
        //Ends with &
        is_bg = true;

        char temp[COMMAND_ARGS_MAX_LENGTH];
        strcpy(temp, command.c_str());
        _removeBackgroundSign(temp);
        new_command = string(temp);
    }
    new_command = _trim(new_command);

    pid_t pid = fork();
    if( pid < 0){
        perror("smash error: fork failed");
        return;
    }
    else if( pid == 0){ //Child proccess
        setpgrp();
        execl(BASH_PATH, "bash", "-c", new_command.c_str(), nullptr);
        perror("smash error: execl failed");
        exit(0);
    }
    else{ //Parent
        if(is_bg){
            //Background command -> add to job list
            jobs_list->addJob(command,pid, false);
            return;
        }
        else{
            //Running foreground -> wait to finish
            jobs_list->setFg(command, pid);
            int res = waitpid(pid, nullptr, WUNTRACED);
            if(res == -1){
                perror("smash error: waitpid failed");
                return;
            }
            jobs_list->setFg("",-1);
        }
    }


}

SmallShell::SmallShell() {
// TODO: add your implementation
    last_pwd = "";
    prompt = "smash";
    jobs_list = new JobsList();
}

SmallShell::~SmallShell() {
// TODO: add your implementation
    delete jobs_list;
}

void free_char_array(char* array[], int num_elements){
    for(int i = 0; i<num_elements; i++){
        free(array[i]);
    }
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
    char cmd[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd, cmd_line);
    _removeBackgroundSign(cmd);
    char* arguments[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(cmd, arguments);
    string first = string(arguments[0]);
    string cmd_s = _trim(string(cmd_line));

    if(cmd_s.find('|') != string::npos){
        free_char_array(arguments, num_args);
        return new PipeCommand(cmd_line, jobs_list);
    }
    else if(cmd_s.find('>') != string::npos){
        free_char_array(arguments, num_args);
        return new RedirectionCommand(cmd_line, jobs_list);
    }
    else if( first == "chprompt"){
        free_char_array(arguments, num_args);
        return new ChpromptCommand(cmd_line, prompt);
    }
    else if(first == "showpid"){
        free_char_array(arguments, num_args);
        return new ShowPidCommand(cmd_line);
    }
    else if(first == "pwd"){
        free_char_array(arguments, num_args);
        return new GetCurrDirCommand(cmd_line);
    }
    else if(first == "cd"){
        free_char_array(arguments, num_args);
        return new ChangeDirCommand(cmd_line, last_pwd);
    }
    else if(first == "jobs"){
        free_char_array(arguments, num_args);
        return new JobsCommand(cmd_line, jobs_list);
    }
    else if(first == "kill"){
        free_char_array(arguments, num_args);
        return new KillCommand(cmd_line, jobs_list);
    }
    else if(first == "fg"){
        free_char_array(arguments, num_args);
        return new ForegroundCommand(cmd_line, jobs_list);
    }
    else if(first == "bg"){
        free_char_array(arguments, num_args);
        return new BackgroundCommand(cmd_line, jobs_list);
    }
    else if(first == "quit"){
        free_char_array(arguments, num_args);
        return new QuitCommand(cmd_line, jobs_list);
    }
    else if(first == "cp"){
        free_char_array(arguments, num_args);
        return new CopyCommand(cmd_line, jobs_list);
    }
    else if(first == "timeout"){
        free_char_array(arguments, num_args);
    }
    else{
        free_char_array(arguments, num_args);
        return new ExternalCommand(cmd_line, jobs_list);
    }
    free_char_array(arguments, num_args);
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
  auto* cmd = CreateCommand(cmd_line);
  if(cmd == nullptr){
      return;
  }
  cmd->execute();
  delete cmd;
}

Command::Command(const char *cmd_line) {
    command = string(cmd_line);
    num_arguments = _parseCommandLine(cmd_line, arguments);
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
    command = string(cmd_line);
    char temp[COMMAND_ARGS_MAX_LENGTH];
    strcpy(temp, cmd_line);
    _removeBackgroundSign(temp);
    num_arguments = _parseCommandLine(temp, arguments);
}

void ChangeDirCommand::execute() {
    if(num_arguments == 1){
        return;
    }
    char* path = this->arguments[1];
    if(this->num_arguments>2){
        _printError("cd: too many arguments");
    }
    else if(!strcmp(path, "-") && this->last_PWD.empty()){
        _printError("cd: OLDPWD not set");
    }
    else{
        char* temp = get_current_dir_name();
        if(temp == nullptr){
            perror("smash error: get_current_dir_name failed");
            return;
        }
        string curr_PWD = string(temp);
        free(temp);
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
    char* temp = get_current_dir_name();
    cout << temp << endl;
    free(temp);
}

int JobsList::addJob(string cmd, pid_t pid, bool isStopped) {
    removeFinishedJobs();
    max_id++;
    JobStat status = isStopped ? Stopped : Running;
    JobEntry newEntry = JobEntry(cmd, max_id, pid, time(nullptr), status);
    jobs.push_back(newEntry);
    return max_id;
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
    max_id = 0;
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    vector<JobEntry>::iterator it = jobs.begin();
    while(it != jobs.end()){
        if(jobId == it->getJobId()){
            return &(*it);
        }
        else{
            it++;
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
        res = waitpid(pid, nullptr, WNOHANG);

        if(res == -1){
            perror("smash error: waitpid failed");
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

int JobsList::getJobsSize() {
    return jobs.size();
}

void JobsList::fgJob(int jobId) {
    JobEntry* job = getJobById(jobId);
    cout << job->getCmd() << " : " << job->getJobPid() << endl;

    pid_t job_pid = job->getJobPid();

    setFg(job->getCmd(), job_pid);
    removeJobById(jobId);
    if(kill(job_pid, SIGCONT) == -1){
        perror("smash error: kill failed");
    }
    else{
        int res = waitpid(job_pid, nullptr, WUNTRACED);
        if(res == -1){
            perror("smash error: waitpid failed");
        }
    }
    setFg("", -1);
}

void JobsList::setFg(string cmd, pid_t pid) {
    fg_command = cmd;
    fg_pid = pid;
}

string JobsList::getFgCmd() {
    return fg_command;
}

pid_t JobsList::getFgPid() {
    return fg_pid;
}

void JobsCommand::execute() {
    jobs_list->printJobsList();
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

    //Determine if '-' sign exits and the arguments are numbers
    if(arguments[1][0] != '-' || !checkNumber(signum.substr(1)) || !checkNumber(job_id)){
        _printError("kill: invalid arguments");
        return;
    }

    //Arrive here -> syntax is valid!
    int signal_num = atoi(arguments[1]);
    int id_num = atoi(arguments[2]);
    //Check if job id exits
    if(jobs_list->getJobById(id_num) == nullptr){
        _printError("kill: job-id " + job_id + " does not exist");
    }

    //All valid -> send signal

    pid_t pid = jobs_list->getJobById(id_num)->getJobPid();
    signal_num = (-1) * signal_num;
    if(kill(pid, signal_num) == -1){
        //Error...
        perror("smash error: kill failed");
    }
    else{
        cout << "signal number " << signal_num << " was sent to pid " << pid << endl;
    }
}

void ForegroundCommand::execute() {
    if(num_arguments > 2){
        _printError("fg: invalid arguments");
        return;
    }

    if(num_arguments == 2){
        //If job-id exits:

        if(!checkNumber(arguments[1])){
            _printError("fg: invalid arguments");
            return;
        }
        int job_id = atoi(arguments[1]);
        if(jobs_list->getJobById(job_id) == nullptr){
            //Job dosent exist..
            _printError("fg: job-id " + to_string(job_id) + " does not exist");
            return;
        }
        //All good - fg the job
        jobs_list->fgJob(job_id);
    }
    else{
        //Job id was not given:

        if(jobs_list->getJobsSize() == 0){
            _printError("fg: jobs list is empty");
        }
        else {
            int last_job_id = jobs_list->getLastJob()->getJobId();
            jobs_list->fgJob(last_job_id);
        }
    }
}

void BackgroundCommand::execute() {
    if(num_arguments > 2){
        _printError("fg: invalid arguments");
        return;
    }

    if(num_arguments == 2){
        //If job-id exits:

        if(!checkNumber(arguments[1])){
            _printError("fg: invalid arguments");
            return;
        }
        int job_id = atoi(arguments[1]);
        if(jobs_list->getJobById(job_id) == nullptr){
            //Job dosent exist..
            _printError("bg: job-id " + to_string(job_id) + " does not exist");
            return;
        }

        if(jobs_list->getJobById(job_id)->getJobStatus() == JobsList::Running){
            _printError("bg: job-id " +to_string(job_id) + " is already running in the background");
            return;
        }

        //All good - bg the job
        JobsList::JobEntry* job = jobs_list->getJobById(job_id);
        cout << job->getCmd() << " : " << job->getJobPid() << endl;
        if(kill(job->getJobPid(), SIGCONT) == -1){
            perror("smash error: kill failed");
        }
        else{

            job->setJobStatus(JobsList::Running);
        }
    }
    else{
        //Job id was not given:

        if(jobs_list->getLastStoppedJob() == nullptr){
            _printError("bg: there is no stopped jobs to resume");
        }
        else {
            JobsList::JobEntry* job = jobs_list->getLastStoppedJob();
            cout << job->getCmd() << " : " << job->getJobPid() << endl;
            if(kill(job->getJobPid(), SIGCONT) == -1){
                perror("smash error: kill failed");
            }
            else{
                job->setJobStatus(JobsList::Running);
            }
        }
    }
}

void QuitCommand::execute() {
    if(num_arguments > 1){
        if(string(arguments[1]) == "kill"){
            cout << "smash: sending SIGKILL signal to " << jobs_list->getJobsSize() << " jobs:" <<endl;
            jobs_list->killAllJobs();
        }
    }
    exit(0);
}

void ChpromptCommand::execute() {
    if(arguments[1] == nullptr){
        prompt = "smash";
    }
    else{
        prompt = arguments[1];
    }
}
