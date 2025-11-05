#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

struct Command {
    std::vector<std::string> argv;
    std::string infile;
    std::string outfile;
    bool append = false;
    bool background = false;
};

std::vector<Command> parse_pipeline(const std::string &line);
bool is_builtin(const std::vector<std::string>&argv);
int do_builtin(std::vector<std::string> argv);
void exec_pipeline(std::vector<Command> &pipeline, const std::string &rawline);

#endif // PARSER_H