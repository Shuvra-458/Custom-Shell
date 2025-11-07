#include "shell.h"
#include "parser.h"
#include "jobs.h"

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <termios.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <limits.h>
#include <dirent.h>
#include <sstream>


using namespace std;

static int shell_terminal;
pid_t shell_pgid;

static void init_shell()
{
    shell_terminal = STDIN_FILENO;

    
    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0)
        perror("setpgid");

   
    tcsetpgrp(shell_terminal, shell_pgid);

    // Install handlers
    install_sigchld_handler();
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}

static string make_prompt() {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    const char* user = getenv("USER");
    if (!user) user = "user";

    return "\033[1;32m" + string(user) + "@myshell\033[0m:\033[1;34m" + string(cwd) + "\033[0m $";
}

// Check if a command exists in PATH
static bool cmd_exists(const string& cmd) {
    const char* path = getenv("PATH");
    if (!path) return false;

    string p = path;
    stringstream ss(p);
    string dir;
    while (getline(ss, dir, ':')) {
        string full = dir + "/" + cmd;
        if (access(full.c_str(), X_OK) == 0)
            return true;
    }
    return false;
}

static char* completion_generator(const char* text, int state) {
    static vector<string> matches;
    static size_t index;

    if (state == 0) {
        matches.clear();
        index = 0;

        // Add commands from PATH
        const char* path = getenv("PATH");
        if (path) {
            string p = path;
            stringstream ss(p);
            string dir;
            while (getline(ss, dir, ':')) {
                DIR* dp = opendir(dir.c_str());
                if (!dp) continue;
                struct dirent* ent;
                while ((ent = readdir(dp)) != nullptr) {
                    if (string(ent->d_name).rfind(text, 0) == 0) {
                        matches.push_back(ent->d_name);
                    }
                }
                closedir(dp);
            }
        }

        // Add files in current directory
        DIR* dp = opendir(".");
        if (dp) {
            struct dirent* ent;
            while ((ent = readdir(dp)) != nullptr) {
                if (string(ent->d_name).rfind(text, 0) == 0) {
                    matches.push_back(ent->d_name);
                }
            }
            closedir(dp);
        }
    }

    if (index < matches.size())
        return strdup(matches[index++].c_str());
    return nullptr;
}

static char** my_completion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, completion_generator);
}



void run_shell()
{
    init_shell();
    string line;
    rl_attempted_completion_function = my_completion;
    while (true) {

        string prompt = make_prompt();

        char *input = readline(prompt.c_str());
        if (!input) { 
            cout << "\n"; 
            break; 
        }

        line = input;
        free(input);

        if (line.empty()) continue;
        add_history(line.c_str());

        vector<Command> pipeline = parse_pipeline(line);
        if (pipeline.empty()) continue;

        // builtin shortcut
        if (pipeline.size() == 1 && is_builtin(pipeline[0].argv)) {
            do_builtin(pipeline[0].argv);
            continue;
        }

        exec_pipeline(pipeline, line);
    }
}
