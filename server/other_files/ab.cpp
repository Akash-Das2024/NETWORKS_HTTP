#include <bits/stdc++.h>
#include <sys/types.h>
#include <dirent.h>
using namespace std;

vector<pair<int, pair<int, double>>> v;

// defining a comparator
bool comp(pair<int, double> p1, pair<int, double> p2)
{
    return (p1.second < p2.second);
}

// Function to count the number of children of a process from all generations
// As explained in heuristic
int count_child(int pid)
{
    char command[50];
    sprintf(command, "pstree -p %d | wc -l", pid);
    int num_children = 0;

    if (system(command) == 0)
    {
        FILE *fp = popen(command, "r");
        if (fp == NULL)
        {
            perror("popen failed");
            exit(1);
        }
        fscanf(fp, "%d", &num_children);
        pclose(fp);
    }
    return num_children;
    //     printf("The number of children of the current process (including all generations) is: %d\n", num_children);
}

// Function to calculate CPU time of a process
pair<int, double> calculate_cpu_time(int pid)
{
    // Calling proc function to get different times
    char file[100];
    sprintf(file, "/proc/%s/stat", (to_string(pid)).c_str());
    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        exit(1);
    }
    char file_1[100];
    sprintf(file_1, "/proc/uptime"); // Storing uptime of the system
    FILE *fp_1 = fopen(file_1, "r");
    if (fp_1 == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    int utime, stime, start_time;
    double uptime;
    fscanf(fp_1, "%lf", &uptime);
    // storing different times from proc call
    fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %d %d %*d %*d %*d %*d %*d %*d %d", &utime, &stime, &start_time);
    int hertz = sysconf(_SC_CLK_TCK);
    // printf("%d, %d, %d, %lf, %d\n", utime, stime, start_time, uptime, hertz);
    double total_time = utime + stime;
    total_time = uptime - (start_time / hertz);
    // printf("total_time %lf\n",total_time);
    fclose(fp);
    fclose(fp_1);

    int cnt_child = count_child(pid);

    v.push_back({pid, {cnt_child, total_time}});
    return {cnt_child, total_time};
}

// Function to get details of a process from the /proc file system
map<string, string> getProcessDetails(int pid)
{
    map<string, string> details;
    string filename = "/proc/" + to_string(pid) + "/status"; // Opening the status file for the process
    ifstream statusFile(filename.c_str());
    if (!statusFile.is_open())
    {
        cout << "Error opening file: " << filename << endl;
        exit(1);
    }
    // Read the details from the status file
    string line;
    while (getline(statusFile, line))
    {
        int pos = line.find(':');
        if (pos != string::npos)
        {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            details[key] = value;
        }
        // printf("......%s\n",(details["cpu_time"]));
    }
    pair<int, double> p;
    p = calculate_cpu_time(pid);
    details["cpu_time"] = to_string(p.second);
    details["num_child"] = to_string(p.first);
    return details;
}

// Function to traverse the process tree
void traverseProcessTree(int pid, int indent, bool suggest)
{
    // Read the details of the process
    map<string, string> details = getProcessDetails(pid);
    // Print the process ID and name
    cout << string(indent, ' ') << "Process ID: " << pid << endl;
    cout << string(indent, ' ') << "Process Name: " << details["Name"] << endl;
    cout << string(indent, ' ') << "Total children(recursively): " << details["num_child"] << endl;
    cout << string(indent, ' ') << "CPU time: " << details["cpu_time"] << endl
         << endl;

    // Recursively traverse the process tree
    int parentPID = atoi(details["PPid"].c_str());
    if (parentPID > 0)
    {
        traverseProcessTree(parentPID, indent + 2, suggest);
    }
}
int main(int argc, char *argv[])
{
    // Check if the correct number of arguments was provided
    if (argc != 2 && argc != 3)
    {
        cout << "Usage: sb [PID] [-suggest]" << endl;
        exit(1);
    }
    // Get the process ID
    int pid = atoi(argv[1]);
    // Check if the suggest flag is set
    bool suggest = false;
    if (argc == 3 && string(argv[2]) == "-suggest")
    {
        suggest = true;
    }
    // Traverse the process tree
    traverseProcessTree(pid, 0, suggest);
    vector<pair<int, double>> vnew;
    if (suggest)
    {
        for (long unsigned int k = 0; k < v.size(); k++)
        {
            vnew.push_back({v[k].first, v[k].second.first - v[k].second.second});
        }
        sort(vnew.begin(), vnew.end(), comp);
        cout << "\n# The suspected TROJAN process id is: " << vnew[vnew.size() - 1].first << endl;
    }
    // for (auto it : vnew)
    // {
    //     cout << it.first << " " << it.second << endl;
    // }
    return 0;
}