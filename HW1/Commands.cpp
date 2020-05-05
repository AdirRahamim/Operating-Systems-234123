#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <climits>
#include "Commands.h"
#include "signals.h"

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

int _parseCommandLine(const char* cmd_line, vector<string>& args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args.push_back(s);
        i++;
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
  if(str == ""){
      return;
  }
  // find last character other than spaces
  int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == (int)string::npos) {
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
    cerr << "smash error: " << err << endl;
}

void CopyCommand::execute() {
    jobs_list->removeFinishedJobs();
    if(num_arguments < 3){
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

    vector<string> new_arguments;
    _parseCommandLine(new_command.c_str(), new_arguments);

    char arg1[PATH_MAX];
    char arg2[PATH_MAX];
    char* res1 = realpath(new_arguments[1].c_str(), arg1);
    char* res2 = realpath(new_arguments[2].c_str(), arg2);


    int read_file = open(new_arguments[1].c_str(), O_RDONLY);
    if(read_file == -1){
        perror("smash error: open failed");
        return;
    }

    if(new_arguments[1] == new_arguments[2] || !strcmp(arg1, arg2)){
        if(close(read_file) == -1){
            perror("smash error: close failed");
        }
        cout << "smash: " << new_arguments[1] << " was copied to " << new_arguments[2] << endl;
        return;
    }

    int write_file = open(new_arguments[2].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(write_file == -1){
        perror("smash error: open failed");
        if(close(read_file) == -1){
            perror("smash error: close failed");
        }
        return;
    }

    //Create buffer to read data
    vector<char> buffer(4096);

    int pid = fork();

    if( pid < 0){
        perror("smash error: fork failed");
        return;
    }
    else if(pid == 0) {
        //Child process
        setpgrp();

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
        cout << "smash: " << new_arguments[1] << " was copied to " << new_arguments[2] << endl;
        exit(0);
    }
    else{
        if(is_bg){
            jobs_list->addJob(command,pid, false);
            return; //No need to wait...
        }
        else{
            jobs_list->setFg(command, pid, -1);
            int res = waitpid(pid, nullptr, WUNTRACED);
            if(res == -1){
                perror("smash error: waitpid failed");
                return;
            }
            jobs_list->setFg("", -1, -1);
        }
    }
}

bool isExternal(string cmd_line){
    char cmd[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd, cmd_line.c_str());
    _removeBackgroundSign(cmd);
    vector<string> arguments;
    int num_args = _parseCommandLine(cmd, arguments);
    if(num_args == 0){
        return false;
    }
    string first = string(arguments[0]);
    string cmd_s = _trim(string(cmd_line));

    if( first == "chprompt"){
        return false;
    }
    else if(first == "showpid"){
        return false;
    }
    else if(first == "pwd"){
        return false;
    }
    else if(first == "cd"){
        return false;
    }
    else if(first == "jobs"){
        return false;
    }
    else if(first == "kill"){
        return false;
    }
    else if(first == "fg"){
        return false;
    }
    else if(first == "bg"){
        return false;
    }
    else if(first == "quit"){
        return false;
    }
    else return !(first == "cp");
    return false;
}

void TimeoutCommand::execute() {
    jobs_list->removeFinishedJobs();
    if(num_arguments < 3){
        _printError("timeout: invalid arguments");
        return;
    }
    else if(!checkNumber(arguments[1])){
        _printError("timeout: invalid arguments");
        return;
    }

    int timeout_time = atoi(arguments[1].c_str());
    if(timeout_time <= 0){
        _printError("timeout: invalid arguments");
        return;
    }

    string new_command;

    for(int i = 2 ; i<num_arguments ; i++){
        new_command += " " + string(arguments[i]);
    }
    new_command = _trim(new_command);

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
    auto _command = temp_smash.CreateCommand(new_command.c_str());

    bool ext = isExternal(new_command);
    if(ext){
        int pid = fork();

        if( pid < 0){
            perror("smash error: fork failed");
            return;
        }
        else if(pid == 0){
            //Child
            setpgrp();
            execl(BASH_PATH, BASH_PATH, "-c", new_command.c_str(), nullptr);
            exit(0);
        }
        else{
            //Parent
            if(is_bg){
                int job_id = jobs_list->addJob(command,pid, false);
                timeout_list->InsertAndCall(command, job_id, timeout_time, pid);
                return; //No need to wait...
            }
            else{
                jobs_list->setFg(command, pid, -1);
                timeout_list->InsertAndCall(command, -1, timeout_time, pid);
                int res = waitpid(pid, nullptr, WUNTRACED);
                if(res == -1){
                    perror("smash error: waitpid failed");
                    return;
                }

                jobs_list->setFg("", -1 , -1);
            }

        }
    }

    else{
        //Built in command
        //First do the command, no need to wait as it built in...
        _command->execute();
        //Add a "Fake" command so still there will be alarm
        timeout_list->InsertAndCall(command, -1, timeout_time, -1);
    }
    /*
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
            int job_id = jobs_list->addJob(command,pid, false);
            timeout_list->InsertAndCall(command, job_id, timeout_time, pid);
            return; //No need to wait...
        }
        else{
            jobs_list->setFg(command, pid, -1);
            timeout_list->InsertAndCall(command, -1, timeout_time, pid);
            int res = waitpid(pid, nullptr, WUNTRACED);
            if(res == -1){
                perror("smash error: waitpid failed");
                return;
            }

            jobs_list->setFg("", -1 , -1);
        }

    }
*/
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
    if(is_bg){
        cmd = cmd + " &";
    }
    string file_path = new_command.substr(redirect+1+(int)isAppend);
    file_path = _trim(file_path);
    vector<string> arguments;
    _parseCommandLine(file_path.c_str(), arguments);
    if(num_arguments == 0 ){
        _printError("redirection: invalid arguments");
        return;
    }
    file_path = arguments[0];
    int file;
    if(isAppend){
        file = open(file_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
        if(file == -1){
            perror("smash error: open failed");
            return;
        }
    }
    else {
        file = open(file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (file == -1) {
            perror("smash error: open failed");
            return;
        }
    }
    SmallShell& temp_smash = SmallShell::getInstance();
    auto _command = temp_smash.CreateCommand(cmd.c_str());

    int stdoutCopy = dup(STDOUT_FILENO);

    if(dup2(file,STDOUT_FILENO) == -1){
        perror("smash error: dup2 failed");
        return;
    }
    _command->execute();

    if(dup2(stdoutCopy, STDOUT_FILENO) == -1){
        perror("smash error: dup2 failed");
        return;
    }
    if(close(file) == -1){
        perror("smash error: close failed");
        return;
    }
}

void PipeCommand::execute() {
    jobs_list->removeFinishedJobs();
    /*
    if(signal(SIGTSTP , ctrlZHandlerPipe)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
        return;
    }
    if(signal(SIGINT , ctrlCHandlerPipe)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
        return;
    }
     */
    bool is_bg = false;
    string new_command = command;
    if (_isBackgroundComamnd(command.c_str())) {
        //Ends with &
        is_bg = true;

        char temp[COMMAND_ARGS_MAX_LENGTH];
        strcpy(temp, command.c_str());
        _removeBackgroundSign(temp);
        new_command = string(temp);
    }

    //Produce first and second command
    int pipeIndex = new_command.find('|');
    bool isStderrPipe = new_command[pipeIndex + 1] == '&';
    string cmd1 = new_command.substr(0, pipeIndex);
    string cmd2 = new_command.substr(pipeIndex + 1 + (int) isStderrPipe);

    SmallShell &temp_smash = SmallShell::getInstance();
    auto command1 = temp_smash.CreateCommand(cmd1.c_str(), true);
    auto command2 = temp_smash.CreateCommand(cmd2.c_str(), true);

    pid_t pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
        return;
    } else if (pid == 0) {
        //Child
        setpgid(0,0);
        //pid_t gid = getpgid(0);
        //Create pipe
        int fd[2];
        if (pipe(fd) == -1) {
            perror("smash error: pipe failed");
            return;
        }

        pid_t pid_cmd1 = fork();
        if (pid_cmd1 < 0) {
            perror("smash error: fork failed");
            return;
        } else if (pid_cmd1 == 0) {

            //Child command 1
            if (isStderrPipe) {
                if (dup2(fd[1], STDERR_FILENO) == -1) {
                    perror("smash error: dup2 failed");
                    return;
                }
            } else {
                if (dup2(fd[1], STDOUT_FILENO) == -1) {
                    perror("smash error: dup2 failed");
                    return;
                }
            }
            close(fd[0]);
            close(fd[1]);
            if(isExternal(cmd1)){
                execl(BASH_PATH, BASH_PATH, "-c", cmd1.c_str(), nullptr);
            }
            else{
                command1->execute();
            }
            exit(0);
        } else {

            //Parent command 1

            pid_t pid_cmd2 = fork();

            if (pid_cmd2 < 0) {
                perror("smash error: fork failed");
                return;
            } else if (pid_cmd2 == 0) {
                if (dup2(fd[0], STDIN_FILENO) == -1) {
                    perror("smash error: dup2 failed");
                    return;
                }
                close(fd[1]);
                close(fd[0]);
                if(isExternal(cmd2)){
                    execl(BASH_PATH, BASH_PATH, "-c", cmd2.c_str(), nullptr);
                }
                else{
                    command2->execute();
                }
                exit(0);
            }
            else{
                close(fd[0]);
                close(fd[1]);

                int status1, status2;
                int res1 = waitpid(pid_cmd1, &status1, WUNTRACED);
                int res2 = waitpid(pid_cmd2, &status2, WUNTRACED);

                if (res1 == -1 || res2 == -1) {
                    perror("smash error: waitpid failed");
                    return;

                }
                exit(0);
            }
        }

    } else {
        //Parent
        if (is_bg) {
            jobs_list->addJob(command, pid, false);
            return; //No need to wait...
        } else {
            jobs_list->setFg(command, pid, -1);
            int res = waitpid(pid, nullptr, WUNTRACED);

            if (res == -1) {
                perror("smash error: waitpid failed");
                return;
            }
            jobs_list->setFg("", -1, -1);
        }
        /*
        if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
            perror("smash error: failed to set ctrl-Z handler");
        }
        if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
            perror("smash error: failed to set ctrl-C handler");
        }
         */
    }

}

void ExternalCommand::execute() {
    jobs_list->removeFinishedJobs();
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
        if(!isPipe){
            setpgrp();
        }
        execl(BASH_PATH, BASH_PATH, "-c", new_command.c_str(), nullptr);
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
            jobs_list->setFg(command, pid, -1);
            int status=0;
            int res = waitpid(pid, &status, WUNTRACED);
            if(res == -1){
                perror("smash error: waitpid failed");
                return;
            }
            jobs_list->setFg("",-1, -1);
        }
    }
}

SmallShell::SmallShell() {
// TODO: add your implementation
    last_pwd = "";
    prompt = "smash";
    jobs_list = new JobsList();
    timeout_list = new TimeoutList(jobs_list);
}

SmallShell::~SmallShell() {
// TODO: add your implementation
    delete jobs_list;
    delete timeout_list;

}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/

Command * SmallShell::CreateCommand(const char* cmd_line, bool isPipe) {
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
    vector<string> arguments;
    int num_args = _parseCommandLine(cmd, arguments);
    if(num_args == 0){
        return nullptr;
    }
    string first = string(arguments[0]);
    string cmd_s = _trim(string(cmd_line));

    if(cmd_s.find('|') != string::npos){
        return new PipeCommand(cmd_line, jobs_list);
    }
    else if(cmd_s.find('>') != string::npos){
        return new RedirectionCommand(cmd_line, jobs_list);
    }
    else if( first == "chprompt"){
        return new ChpromptCommand(cmd, prompt);
    }
    else if(first == "showpid"){
        return new ShowPidCommand(cmd_line, pid);
    }
    else if(first == "pwd"){
        return new GetCurrDirCommand(cmd_line);
    }
    else if(first == "cd"){
        return new ChangeDirCommand(cmd_line, last_pwd);
    }
    else if(first == "jobs"){
        return new JobsCommand(cmd_line, jobs_list);
    }
    else if(first == "kill"){
        return new KillCommand(cmd_line, jobs_list);
    }
    else if(first == "fg"){
        return new ForegroundCommand(cmd_line, jobs_list);
    }
    else if(first == "bg"){
        return new BackgroundCommand(cmd_line, jobs_list);
    }
    else if(first == "quit"){
        return new QuitCommand(cmd_line, jobs_list);
    }
    else if(first == "cp"){
        return new CopyCommand(cmd_line, jobs_list);
    }
    else if(first == "timeout"){
        return new TimeoutCommand(cmd_line, jobs_list, timeout_list);
    }
    else{
        return new ExternalCommand(cmd_line, jobs_list, isPipe);
    }
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
    string path = this->arguments[1];
    if(this->num_arguments>2){
        _printError("cd: too many arguments");
        return;
    }
    if(path == "-" && this->last_PWD.empty()){
        _printError("cd: OLDPWD not set");
        return;
    }
    char* temp = get_current_dir_name();
    if(temp == nullptr){
        perror("smash error: get_current_dir_name failed");
        return;
    }
    string curr_PWD = string(temp);
    free(temp);
    int res = 0;
    if(path == "-"){
        //Return to last path
        res = chdir(last_PWD.c_str());
    }
    else{
        //Change to new path
        res = chdir(path.c_str());
    }
    if(res == -1){
        //Error...
        perror("smash error: chdir failed");
        return;
    }
    else{
        //Succeed so save last path
        last_PWD = curr_PWD;
    }
}

void ShowPidCommand::execute() {
    cout << "smash pid is " << pid << endl;
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

        if(res == pid){
            it = jobs.erase(it);
        }
        else{
            it++;
        }
        /*
        res = waitpid(pid, nullptr, WNOHANG);

        int err = errno;
        if(res == -1 && err != ECHILD){
            perror("smash error: waitpid failed");
            it++;
            continue;
        }
        else if(res == pid){
            it = jobs.erase(it);
        }
        else if(res == -1 && err == ECHILD){
            it = jobs.erase(it);
        }
        else{
            it++;
        }
         */
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
    int job_id = job->getJobId();
    setFg(job->getCmd(), job_pid, job->getJobId());
    job->setJobStatus(true);
    resetTime(jobId);
    setBgToFg(true);
    bool isPipe = false;
    if(job->getCmd().find('|') != string::npos){
        isPipe = true;
    }
    int res = 0;
    if(isPipe){
        res = killpg(job_pid, SIGCONT);
    }
    else{
        res = kill(job_pid, SIGCONT);
    }
    if(res == -1){
        perror("smash error: kill failed");
    }
    else{
        int status;
        int res = waitpid(job_pid, &status, WUNTRACED);

        if(res == -1){
            perror("smash error: waitpid failed");
        }
    }
    if(job->getJobStatus() != Stopped){
        removeJobById(job_id);
    }
    setFg("", -1, -1);
    setBgToFg(false);
}

void JobsList::setFg(string cmd, pid_t pid, int jobId) {
    fg_command = cmd;
    fg_pid = pid;
    fgId = jobId;
}

string JobsList::getFgCmd() {
    return fg_command;
}

pid_t JobsList::getFgPid() {
    return fg_pid;
}

void JobsList::bgJob(int jobId) {
    JobEntry* job = getJobById(jobId);
    cout << job->getCmd() << " : " << job->getJobPid() << endl;
    bool isPipe = false;
    if(job->getCmd().find('|') != string::npos){
        isPipe = true;
    }
    int res = 0;
    if(isPipe){
        res = killpg(job->getJobPid(), SIGCONT);
    }
    else{
        res = kill(job->getJobPid(), SIGCONT);
    }
    if(res == -1){
        perror("smash error: kill failed");
    }
    job->setJobStatus(true);

}

void JobsList::resetTime(int jobId) {
    JobEntry* job = getJobById(jobId);
    job->resetTime();
}

void JobsList::setBgToFg(bool update) {
    isBgToFg = update;
}

bool JobsList::getBgToFg() {
    return isBgToFg;
}

void JobsList::setStop(int jobId) {
    JobEntry* job = getJobById(jobId);
    job->setJobStatus(false);
}

int JobsList::getFgId() {
    return fgId;
}

JobsList::JobEntry *JobsList::getJobByPid(pid_t job_pid) {
    auto it = jobs.begin();
    while(it != jobs.end()){
        if(job_pid == it->getJobPid()){
            return &(*it);
        }
        else{
            it++;
        }
    }
    return nullptr;
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
    if(!checkNumber(signum) || !checkNumber(job_id)){
        _printError("kill: invalid arguments");
        return;
    }

    //Arrive here -> syntax is valid!
    int signal_num = atoi(arguments[1].c_str());
    /*
    if(signal_num>=0 || signal_num < -31){
        _printError("kill: invalid arguments");
        return;
    }*/
    int id_num = atoi(arguments[2].c_str());
    //Check if job id exits
    if(jobs_list->getJobById(id_num) == nullptr){
        _printError("kill: job-id " + job_id + " does not exist");
        return;
    }

    //All valid -> send signal

    pid_t pid = jobs_list->getJobById(id_num)->getJobPid();
    signal_num = (-1) * signal_num;

    string job_cmd = jobs_list->getJobById(id_num)->getCmd();
    bool isPipe = false;
    if(job_cmd.find('|') != string::npos){
        isPipe = true;
    }
    if(isPipe){
        if(killpg(pid, signal_num) == -1){
            //Error...
            perror("smash error: kill failed");
            return;
        }
    }
    else{
        if(kill(pid, signal_num) == -1){
            //Error...
            perror("smash error: kill failed");
            return;
        }
    }
    cout << "signal number " << signal_num << " was sent to pid " << pid << endl;
}

void ForegroundCommand::execute() {
    jobs_list->removeFinishedJobs();
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
        int job_id = atoi(arguments[1].c_str());
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
        _printError("bg: invalid arguments");
        return;
    }

    if(num_arguments == 2){
        //If job-id exits:

        if(!checkNumber(arguments[1])){
            _printError("fg: invalid arguments");
            return;
        }
        int job_id = atoi(arguments[1].c_str());
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
        jobs_list->bgJob(job_id);
    }
    else{
        //Job id was not given:

        if(jobs_list->getLastStoppedJob() == nullptr){
            _printError("bg: there is no stopped jobs to resume");
        }
        else {
            int jobId = jobs_list->getLastStoppedJob()->getJobId()  ;
            jobs_list->bgJob(jobId);
        }
    }
}

void QuitCommand::execute() {
    jobs_list->removeFinishedJobs();
    if(num_arguments > 1){
        for(int i=1 ; i<num_arguments; i++){
            if(string(arguments[i]) == "kill"){
                cout << "smash: sending SIGKILL signal to " << jobs_list->getJobsSize() << " jobs:" <<endl;
                jobs_list->killAllJobs();
                break;
            }
        }
    }
    exit(0);
}

void ChpromptCommand::execute() {
    if(num_arguments == 1){
        prompt = "smash";
    }
    else{
        prompt = arguments[1];
    }
}
