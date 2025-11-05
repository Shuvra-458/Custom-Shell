#include "parser.h"
#include "jobs.h"

#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

extern pid_t shell_pgid;

using namespace std;

static vector<string> split_tokens(const string &s) {
    vector<string> tokens;
    string cur;
    bool in_single = false, in_double = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '\'' && !in_double) { in_single = !in_single; continue; }
        if (c == '"' && !in_single) { in_double = !in_double; continue; }
        if (!in_single && !in_double && isspace((unsigned char)c)) {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

vector<Command> parse_pipeline(const string &line) {
    vector<string> tokens = split_tokens(line);
    vector<Command> pipeline;
    vector<string> segment;

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == "|") {
            if (!segment.empty()) {
                Command c;
                for (size_t j = 0; j < segment.size(); ++j) {
                    if (segment[j] == "<" && j+1 < segment.size()) c.infile = segment[++j];
                    else if (segment[j] == ">" && j+1 < segment.size()) { c.outfile = segment[++j]; c.append = false; }
                    else if (segment[j] == ">>" && j+1 < segment.size()) { c.outfile = segment[++j]; c.append = true; }
                    else c.argv.push_back(segment[j]);
                }
                pipeline.push_back(move(c));
                segment.clear();
            }
        } else segment.push_back(tokens[i]);
    }

    if (!segment.empty()) {
        Command c;
        for (size_t j = 0; j < segment.size(); ++j) {
            if (segment[j] == "<" && j+1 < segment.size()) c.infile = segment[++j];
            else if (segment[j] == ">" && j+1 < segment.size()) { c.outfile = segment[++j]; c.append = false; }
            else if (segment[j] == ">>" && j+1 < segment.size()) { c.outfile = segment[++j]; c.append = true; }
            else c.argv.push_back(segment[j]);
        }

        if (!c.argv.empty()) {
            string &last = c.argv.back();
            if (last == "&") { c.background = true; c.argv.pop_back(); }
            else if (!last.empty() && last.back() == '&') { last.pop_back(); c.background = true; if (last.empty()) c.argv.pop_back(); }
        }

        pipeline.push_back(move(c));
    }

    return pipeline;
}

bool is_builtin(const vector<string>&argv) {
    if (argv.empty()) return false;
    string s = argv[0];
    return (s=="cd"||s=="exit"||s=="jobs"||s=="fg"||s=="bg");
}

int do_builtin(vector<string> argv){
    if (argv.empty()) return 0;
    string cmd = argv[0];

    if (cmd=="cd") {
        string dir = (argv.size()>1)? argv[1] : getenv("HOME");
        if (chdir(dir.c_str())!=0) perror("cd");
        return 1;
    }
    else if (cmd=="exit"){
        exit(0);
    }
    else if (cmd=="jobs"){
        list_jobs();
        return 1;
    }
    else if (cmd=="fg"||cmd=="bg"){
        if (argv.size()<2) {cerr<<"Usage: "<<cmd<<" <jobid>\n"; return 1; }
        int id = stoi(argv[1]);
        bring_job(id, cmd=="fg");
        return 1;
    }
    return 0;
}

void exec_pipeline(vector<Command> &pipeline, const string &rawline) {
    size_t n = pipeline.size();
    vector<int> pipefds(2 * (n>1 ? n-1 : 0));
    for (size_t i=0; i+1<n; i++) if (pipe(pipefds.data()+2*i)<0) { perror("pipe"); return; }

    pid_t pgid = 0;
    for (size_t i=0; i<n; i++){
        int in_fd = -1, out_fd = -1;
        if (!pipeline[i].infile.empty()) in_fd = open(pipeline[i].infile.c_str(), O_RDONLY);
        if (!pipeline[i].outfile.empty()) {
            int flags = O_WRONLY | O_CREAT | (pipeline[i].append? O_APPEND : O_TRUNC);
            out_fd = open(pipeline[i].outfile.c_str(), flags, 0644);
        }

        pid_t pid = fork();
        if (pid < 0) {perror("fork"); break;}
        if (pid==0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            if (pgid==0) pgid = getpid();
            setpgid(0, pgid);

            if (i>0) dup2(pipefds[2*(i-1)], STDIN_FILENO);
            else if (in_fd!=-1) dup2(in_fd, STDIN_FILENO);

            if (i+1<n) dup2(pipefds[2*i+1], STDOUT_FILENO);
            else if (out_fd!=-1) dup2(out_fd, STDOUT_FILENO);

            for (int fd: pipefds) if (fd>2) close(fd);
            if (in_fd>2) close(in_fd);
            if (out_fd>2) close(out_fd);

            vector<char*> cargv;
            for (auto &a : pipeline[i].argv) cargv.push_back(const_cast<char*>(a.c_str()));
            cargv.push_back(nullptr);

            execvp(cargv[0], cargv.data());
            perror("execvp");
            _exit(127);
        } else {
            if (pgid==0) pgid = pid;
            setpgid(pid, pgid);
        }
    }

    for (int fd : pipefds) if (fd>2) close(fd);

    bool background = pipeline.back().background;
    register_job(pgid, rawline, background);

    if (!background) {
        // Give terminal to the job's process group
        if (tcsetpgrp(STDIN_FILENO, pgid) < 0) {
            perror("tcsetpgrp (to child pgid)");
        }

        // Wait for the foreground process group to either exit or stop.
        // Use a loop so we wait for the whole process group, not just one child.
        int status = 0;
        pid_t w;
        do {
            w = waitpid(-pgid, &status, WUNTRACED);
            if (w == -1) {
                if (errno == ECHILD) break;
                // other error
                break;
            }
        } while (w > 0 && !WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));

        // Restore terminal to the shell's process group
        if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
            perror("tcsetpgrp (restore to shell)");
        }

        // If job was stopped, mark it as stopped (jobs SIGCHLD handler also updates this)
        // Reap/cleanup finished jobs
    } else {
        cout << "[" << get_job_id_by_pgid(pgid) << "] " << pgid << "\n";
    }

    reap_finished_jobs();

}
