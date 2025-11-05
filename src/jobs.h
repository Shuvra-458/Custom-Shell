#ifndef JOBS_H
#define JOBS_H

#include <string>
#include <vector>

void install_sigchld_handler();
void register_job(pid_t pgid, const std::string &cmdline, bool background);
void list_jobs();
void reap_finished_jobs();
void bring_job(int id, bool foreground);
int get_job_id_by_pgid(pid_t pgid);

#endif //JOBS_H