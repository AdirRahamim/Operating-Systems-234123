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
            smash.getJobsList()->addJob(smash.getJobsList()->getFgCmd(),fg_pid, true);
            smash.getJobsList()->setFg("", -1);
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
            smash.getJobsList()->setFg("", -1);
            cout << "smash: process " << fg_pid << " was killed" << endl;
        }
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
  cout << "smash: got an alarm" <<endl;
  SmallShell& smash = SmallShell::getInstance();
  JobsList* jobs = smash.getJobsList();
    vector<SmallShell::TimeoutJobs>& timeout_jobs = smash.getTimeoutJobs();
  auto it = timeout_jobs.begin();
  while(it != timeout_jobs.end()){
      if(it->getDuration() == (time(nullptr) - it->getStartTime())){
          cout << "smash: " << it->getCmd() << " timed out!" << endl;
          if(it->getPid() != -1){
              //Backgroud job -> remove from jobs list
              jobs->removeJobById(it->getJobId());
          }
          else{
              //Foreground job -> update
              jobs->setFg("", -1);
          }
          if(kill(it->getPid(), SIGKILL) == -1){
              perror("smash error: kill failed");
          }
          it = timeout_jobs.erase(it);
      }
      else{
          it++;
      }
  }
}

