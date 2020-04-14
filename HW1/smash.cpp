#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fstream>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    struct sigaction act;
    act.sa_handler = alarmHandler;
    act.sa_flags = SA_RESTART;
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    if(sigaction(SIGALRM , &act, nullptr) == -1) {
        perror("smash error: failed to set ctrl-C handler");
    }


    //TODO: setup sig alarm handler

   //ifstream file("/home/adir/Downloads/טכניון/Operating-Systems-234123/HW1/test_input1.txt");

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        std::cout << smash.getPrompt() << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        //std::getline(file, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}
