#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

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

// TODO: Add your implementation for classes in Commands.h 

Command::Command(const char* cmd_line) : cmd_line(cmd_line) {}
Command::~Command() {}
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}
ChangePromptCommand::ChangePromptCommand(const char* cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line), smash(smash){}
ShowPidCommand::ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
ChangeDirCommand:: ChangeDirCommand(const char* cmd_line, char** last_pwd): BuiltInCommand(cmd_line), last_pwd(last_pwd){}
SmallShell::SmallShell() : prompt("smash"){
    char* emp = NULL;
    last_pwd = &emp;
}
const char* SmallShell::getPrompt(){
    return this->prompt;
}
void SmallShell::setPrompt(const char* prompt){
    this->prompt = prompt;
}
// 1
void ChangePromptCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int length = _parseCommandLine(cmd_line, args);
    if (length == 1)
        smash->setPrompt("smash");
    else
        smash->setPrompt(args[1]);
}
//2
void ShowPidCommand :: execute(){
    std::cout << "smash pid is " << getpid()<< "\n";  // need to check if that is the proper way.
}

char** SmallShell:: getLastPwd(){
    return last_pwd;
}

//4
void ChangeDirCommand:: execute(){
    char* args[COMMAND_MAX_ARGS];
    int length = _parseCommandLine(cmd_line, args);
    if(length>2){
        std::cerr << "smash error: cd: too many arguments" << endl;
    }
    else if(length == 2){
        char sign[] = "-";
        if(strcmp(args[1], sign) == 0){
            if(*(this->last_pwd) == NULL){
                std::cerr << "smash error: cd: OLDPWD not set \n";
            }
            else{
                char* copy_last_pwd = (char*) malloc(strlen(*last_pwd) + 1);
                strcpy(copy_last_pwd, *last_pwd);
                free(*last_pwd);
                *this->last_pwd = getcwd(NULL, 0);
                if(chdir(copy_last_pwd) == -1){
                    perror("smash error: chdir failed");
                    free(*last_pwd);
                    *this->last_pwd = getcwd(NULL, 0);
                }
                else{
                      std::cout << copy_last_pwd << "\n";
//                    free(*last_pwd);
//                    this->last_pwd = &prev_location;
//                    std::cout << *this->last_pwd << "\n";
                }
                free(copy_last_pwd);
            }
        }
        else{
            free(*last_pwd);
            *this->last_pwd = getcwd(NULL, 0);
            if (chdir(args[1]) == -1){
                perror("smash error: chdir failed");
                free(*last_pwd);
                *this->last_pwd = getcwd(NULL, 0);
            }
            else{
                std::cout << args[1] << "\n";
            }
        }

    }

}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("chprompt") == 0) {
        return new ChangePromptCommand(cmd_line, this);
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("cd") == 0) {
//        char* temp_last_pwd;
//        strcpy(temp_last_pwd, last_pwd);

//        char* temp_last_pwd = last_pwd.c_str()
            return new ChangeDirCommand(cmd_line, getLastPwd());
        }

//        free(temp_last_pwd);

	// For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
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
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)



}