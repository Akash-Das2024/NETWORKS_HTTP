#include <bits/stdc++.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <glob.h>
using namespace std;
using ll = long long;

void shell();
vector<string> split_pipes(string &);
vector<string> split_line(string &);
int other_cmds(vector<string> &, int, int);
int execute(vector<string> &);
vector<string> glob(const string &);
void delep(string file_name);

int main()
{
    signal(SIGTSTP, SIG_IGN); // ignore ctrl-c and ctrl-z
    signal(SIGINT, SIG_IGN);
    shell();
    return 0;
}

void shell()
{
    string line;
    vector<string> args;
    vector<string> pipes;
    bool state = true;
    char *buf = new char[1024];
    while (state)
    {
        getcwd(buf, 1024);
        cout << "\033[0;32m";
        cout << buf;
        cout << "\033[0m";
        cout << "& ";
        signal(SIGTSTP, SIG_IGN);
        signal(SIGINT, SIG_IGN);
        getline(cin, line);
        pipes = split_pipes(line);
        if (pipes.size() == 1)
        {
            args = split_line(line);
            state = execute(args);
        }
        else if (pipes.size() > 1)
        {
            int in = 0, pipe_des[2];
            for (int i = 0; i < pipes.size() - 1; i++)
            {
                pipe(pipe_des);
                args = split_line(pipes[i]);
                state = other_cmds(args, in, pipe_des[1]);
                close(pipe_des[1]);
                in = pipe_des[0];
            }
            args = split_line(pipes.back());
            state = other_cmds(args, in, STDOUT_FILENO);
            dup(STDIN_FILENO);
            dup(STDOUT_FILENO);
        }
        fflush(stdin);
        fflush(stdout);
        line.clear();
        args.clear();
    }
}

vector<string> split_pipes(string &line)
{
    int start = 0, end = 0;
    vector<string> cmds;
    string tmp;
    for (int i = 0; i < line.length(); i++)
    {
        if (line[i] != '|')
        {
            tmp += line[i];
        }
        else
        {
            cmds.push_back(tmp);
            tmp = "";
        }
    }
    if (tmp != "")
        cmds.push_back(tmp);
    return cmds;
}

vector<string> split_line(string &line)
{
    vector<string> cmds;
    string tmp;
    line.append(" ");
    for (int i = 0; i < line.length(); i++)
    {
        if (line[i] != ' ')
        {
            tmp += line[i];
        }
        else
        {
            if (tmp.length() != 0)
            {
                // cout << "1" << endl;
                // cout << tmp << endl;
                vector<string> A;
                if (tmp.find('*') != string ::npos || tmp.find('?') != string ::npos)
                {
                    // cout << "2" << endl;
                    A = glob(tmp);
                    for (auto it : A)
                        cmds.push_back(it);
                }
                else
                {
                    // cout << "3" << endl;
                    cmds.push_back(tmp);
                }
            }
            tmp = "";
        }
    }
    if (tmp != "")
        cmds.push_back(tmp);
    return cmds;
}

int other_cmds(vector<string> &args, int in, int out)
{
    int child_status;
    pid_t pid1 = 0, wpid;

    pid1 = fork();
    if (pid1 == 0)
    {
        // Child Process
        // cout << "a\n";
        signal(SIGTSTP, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        int cnt = 0;
        if (in != 0)
        {
            dup2(in, STDIN_FILENO);
            close(in);
        }
        if (out != 1)
        {
            dup2(out, STDOUT_FILENO);
            close(out);
        }

        for (int i = 0; i < args.size(); i++)
        {
            if (args[i] == "&")
                cnt++;
            else if (args[i] == ">")
            {
                mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
                close(STDOUT_FILENO);
                dup(open(args[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, mode));
                cnt += 2;
            }
            else if (args[i] == "<")
            {
                close(STDIN_FILENO);
                dup(open(args[i + 1].c_str(), O_RDONLY));
                cnt += 2;
            }
        }

        char **arg_s = new char *[args.size() + 1];
        int i = 0;
        for (i = 0; i < args.size() - cnt; i++)
        {
            arg_s[i] = strdup(args[i].c_str());
        }
        arg_s[i] = NULL;
        if (execvp(arg_s[0], arg_s) == -1)
            perror("Invalid Command");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent Process
        // cout << "b\n";
        if (args.back() != "&")
        {
            do
            {
                wpid = waitpid(pid1, &child_status, WUNTRACED);
            } while (!WIFEXITED(child_status) && !WIFSIGNALED(child_status) && !WIFSTOPPED(child_status));
            if (WIFSTOPPED(child_status))
            {
                cout << "\n";
                kill(pid1, SIGCONT);
            }
            else if (WIFSIGNALED(child_status))
            {
                cout << "\n";
            }
        }
    }
    return 1;
}

void delep(string file_name)
{
    vector<int> processes;
    char temp[5];
    char buffer[1024];
    strcpy(buffer, "lsof ");
    strcat(buffer, file_name.c_str());
    FILE *f = popen(buffer, "r");
    if (f == NULL)
        return;
    while (fgets(buffer, 1024, f) != NULL)
    {
        int pid;
        if (sscanf(buffer, "%*s %d", &pid) == 1)
        {
            processes.push_back(pid);
        }
    }
    pclose(f);
    if (processes.size() == 0)
    {
        cout << "The " << file_name << " is not locked\n";
    }
    else
    {
        cout << "The file " << file_name << " is locked by following processes: \n";
        for (int k = 0; k < processes.size(); k++)
        {
            cout << processes[k] << endl;
        }
        cout << "Do you want to kill all of these processes (yes/no)\n";
        cin >> temp;
        if (temp[0] == 'Y' | temp[0] == 'y')
        {
            for (int k = 0; k < processes.size(); k++)
            {
                kill(processes[k], SIGKILL);
            }
            remove(file_name.c_str());
            cout << "All process are killed and the file " << file_name << " is successfully deleted" << endl;   
        }
    }
    getchar();

}

int execute(vector<string> &args)
{
    if (args[0] == "")
        return 1;
    else
    {
        if (args[0] == "cd")
        {
            if (args.size() == 1)
            {
                chdir(getenv("HOME"));
            }
            else if (args.size() > 2)
            {
                perror("cd: too many arguments");
            }
            else
            {
                if (chdir(args[1].c_str()) != 0)
                {
                    perror("Invalid argument");
                }
            }
        }
        else if (args[0] == "pwd")
        {
            char *buf = new char[1024];
            getcwd(buf, 1024);
            cout << buf << endl;
        }
        else if (args[0] == "exit")
            return 0;
        else if (args[0] == "delep")
        {
            delep(args[1]);
        }
        else
            return other_cmds(args, STDIN_FILENO, STDOUT_FILENO);
    }
    return 1;
}

vector<string> glob(const string &pattern)
{
    // glob struct
    glob_t glob_op;
    memset(&glob_op, 0, sizeof(glob_op));

    // doing the glob operation
    int ret = glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_op);
    if (ret != 0)
    {
        globfree(&glob_op);
        stringstream ss;
        ss << "glob() failed with return value, " << ret << endl;
        throw runtime_error(ss.str());
    }

    // collect all the filenames into a vector<string>
    vector<string> filenames;
    for (size_t i = 0; i < glob_op.gl_pathc; ++i)
    {
        filenames.push_back(string(glob_op.gl_pathv[i]));
    }

    // cleanup
    globfree(&glob_op);

    // done
    return filenames;
}