// src/jobs.cpp
#include "jobs.h"
#include "shell.h" 

#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

struct Job { pid_t pgid; string cmdline; bool stopped; bool background; int id;};
static vector<Job> jobs;
static int next_job_id = 1;

static int find_job_index_by_pgid(pid_t pgid) {
    for (size_t i = 0; i < jobs.size(); ++i) {
        if (jobs[i].pgid == pgid) return (int)i;
    }
    return -1;
}

static void sigchld_handler(int) {
    int saved = errno;
    int status;
    pid_t pid;


    // Use WNOHANG | WUNTRACED | WCONTINUED so we learn about stops/continues too.
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        pid_t pg = pid;
        pid_t g = getpgid(pid);
        if (g > 0) pg = g;

        int idx = find_job_index_by_pgid(pg);
        if (idx < 0) {
            continue;
        }

        Job &j = jobs[idx];

        if (WIFSTOPPED(status)) {
            j.stopped = true;
        } else if (WIFCONTINUED(status)) {
            j.stopped = false;
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            j.pgid = -1;
        }
    }

    errno = saved;
}

void install_sigchld_handler(){
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, nullptr);
}

void register_job(pid_t pgid, const string &cmdline, bool background) {
    Job j; j.pgid = pgid; j.cmdline = cmdline; j.stopped = false; j.background = background; j.id = next_job_id++;
    jobs.push_back(j);
}

void list_jobs() {
    for (auto &j : jobs) {
        if (j.pgid == -1) continue;
        cout << "[" << j.id << "] " 
             << (j.stopped ? "Stopped" : (j.background ? "Running" : "Foreground"))
             << " pgid=" << j.pgid << " " << j.cmdline << "\n";
    }
}

void reap_finished_jobs() {
    jobs.erase(remove_if(jobs.begin(), jobs.end(), [](const Job &j){ return j.pgid == -1; }), jobs.end());
}

int get_job_id_by_pgid(pid_t pgid){
    for (auto &j : jobs) if (j.pgid == pgid) return j.id;
    return -1;
}

void bring_job(int id, bool foreground){
    for (auto it = jobs.begin(); it != jobs.end(); ++it) {
        if (it->id == id) {
            Job &j = *it;
            if (j.pgid == -1) { cerr<<"job finished\n"; return; }

            // continue the entire process group
            if (kill(-j.pgid, SIGCONT) < 0) perror("kill(SIGCONT)");
            j.stopped = false;

            if (foreground) {
                // Give terminal to job group
                if (tcsetpgrp(STDIN_FILENO, j.pgid) < 0) perror("tcsetpgrp (to job)");

                
                int status = 0;
                pid_t w;
                do {
                    w = waitpid(-j.pgid, &status, WUNTRACED);
                    if (w == -1) {
                        if (errno == ECHILD) break;
                        perror("waitpid");
                        break;
                    }
                } while (w > 0 && !WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));

                
                if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) perror("tcsetpgrp (restore to shell)");
            }
            return;
        }
    }
    cerr<<"Job not found\n";
}
