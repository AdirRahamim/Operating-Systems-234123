#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"


using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    pid_t fg_pid = smash.getJobsList()->getFgPid();
    if(fg_pid != -1){
        if(kill(fg_pid, SIGSTOP) == -1){
            perror("smash error: kill failed");
        }
        else{
            if(!smash.getJobsList()->getBgToFg()){
                smash.getJobsList()->addJob(smash.getJobsList()->getFgCmd(),fg_pid, true);
            }
            else{
                smash.getJobsList()->getJobById(smash.getJobsList()->getFgId())->setJobStatus(false);
            }
            smash.getJobsList()->setFg("", -1, -1);
            smash.getJobsList()->setBgToFg(false);
            cout << "smash: process " << fg_pid << " was stopped" << endl;
        }
    }
}

void ctrlZHandlerPipe(int sig_num){
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    pid_t fg_pid = smash.getJobsList()->getFgPid();
    if(fg_pid != -1){
        if(killpg(fg_pid, SIGSTOP) == -1){
            perror("smash error: kill failed");
        }
        else{
            if(!smash.getJobsList()->getBgToFg()){
                smash.getJobsList()->addJob(smash.getJobsList()->getFgCmd(),fg_pid, true);
            }
            else{
                smash.getJobsList()->getJobById(smash.getJobsList()->getFgId())->setJobStatus(false);
            }
            smash.getJobsList()->setFg("", -1, -1);
            smash.getJobsList()->setBgToFg(false);
            cout << "smash: process " << fg_pid << " was stopped" << endl;
        }
    }
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
    cout << "smash: got ctrl-C" << endl;
    SmallShell& smash = SmallShell::getInstance();
    pid_t fg_pid = smash.getJobsList()->getFgPid();
    if(fg_pid != -1){
        if(kill(fg_pid, SIGKILL) == -1){
            perror("smash error: kill failed");
        }
        else{
            smash.getJobsList()->setFg("", -1, -1);
            cout << "smash: process " << fg_pid << " was killed" << endl;
        }
    }
}

void ctrlCHandlerPipe(int sig_num){
    cout << "smash: got ctrl-C" << endl;
    SmallShell& smash = SmallShell::getInstance();
    pid_t fg_pid = smash.getJobsList()->getFgPid();
    if(fg_pid != -1){
        if(killpg(fg_pid, SIGKILL) == -1){
            perror("smash error: kill failed");
        }
        else{
            smash.getJobsList()->setFg("", -1, -1);
            cout << "smash: process " << fg_pid << " was killed" << endl;
        }
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
  SmallShell& smash = SmallShell::getInstance();
  smash.getJobsList()->removeFinishedJobs();
  smash.getTimeoutList()->update();
  if(!smash.getTimeoutList()->checkAreFinished()){

  }
  else{
      cout << "smash: got an alarm" <<endl;
      smash.getTimeoutList()->removeTimeoutJobs();
      smash.getTimeoutList()->callNext();
  }
}

