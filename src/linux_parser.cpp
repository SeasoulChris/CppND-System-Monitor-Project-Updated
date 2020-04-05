#include "linux_parser.h"

#include <dirent.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

using std::stof;
using std::string;
using std::to_string;
using std::vector;

vector<string> string_to_vector(const std::string input_string) {
  vector<string> string_vector;
  std::istringstream ss(input_string);
  std::string data;
  while (ss >> data) {
    string_vector.emplace_back(data);
  }
  return string_vector;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  string os, kernel;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
        //std::cout << "................" << pid << "\n";
      }
    }
  }
  closedir(directory);
  return pids;
}

// Done: Read and return the system memory utilization
// (MemTotal - MemFree)/MemTotal
float LinuxParser::MemoryUtilization() {
  std::ifstream filestream(kProcDirectory + kMeminfoFilename);
  string key;
  int value;
  string line;
  float MemTotal, MemFree = 0.0;
  if (filestream) {
    while (std::getline(filestream, line)) {
      std::istringstream ss(line);
      ss >> key >> value;
      if (key == "MemTotal:") MemTotal = value;
      if (key == "MemFree:") {
        MemFree = value;
        break;
      }
    }
  }
  return (MemTotal - MemFree) / MemTotal;
}

// Done: Read and return the system uptime
long LinuxParser::UpTime() {
  //std::cout << "up time used" << "\n";
  std::ifstream filestream(kProcDirectory + kUptimeFilename);
  long uptime, idletime = 0;
  if (filestream) {
    string line;
    std::getline(filestream, line);
    std::istringstream ss(line);
    ss >> uptime >> idletime;
  }
  return uptime;
}

// Done: Read and return the number of jiffies for the system
long LinuxParser::Jiffies() {
  return LinuxParser::ActiveJiffies() + LinuxParser::IdleJiffies();
}

// Done: Read and return the number of active jiffies for a PID
// REMOVE: [[maybe_unused]] once you define the function
// utime(14)、stime(15)、cutime(16)、cstime(17) starttime(22)
long LinuxParser::ActiveJiffies(int pid) {
  std::ifstream filestream(kProcDirectory + std::to_string(pid) +
                           kStatFilename);
  long cpu_usage;
  if (filestream) {
    string line;
    std::getline(filestream, line);
    std::vector<string> string_tmp = string_to_vector(line);
    float utime = stol(string_tmp[13]);
    float stime = stol(string_tmp[14]);
    float cutime = stol(string_tmp[15]);
    float cstime = stol(string_tmp[16]);
    float starttime = stol(string_tmp[21]);

    long uptime = LinuxParser::UpTime(pid);
    float hertz = sysconf(_SC_CLK_TCK);
    float total_time = utime + stime + cutime + cstime;
    float seconds = uptime - (starttime / hertz);
    cpu_usage =  100*((total_time / hertz) / seconds);
    //std::cout << "pid: " << pid <<" " <<"uptime: " << uptime <<" " << "starttime hertz: " << starttime/hertz << " "<< "cpu_usage: " << cpu_usage << "\n";

  }

  return cpu_usage;
}

// Done: Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() {
  vector<string> cpu_stats_string = LinuxParser::CpuUtilization();
  long user, nice, system, idle, iowait, irq, softirq, steal = 0;
  user = stol(cpu_stats_string[LinuxParser::CPUStates::kUser_]);
  nice = stol(cpu_stats_string[LinuxParser::CPUStates::kNice_]);
  system = stol(cpu_stats_string[LinuxParser::CPUStates::kSystem_]);
  idle = stol(cpu_stats_string[LinuxParser::CPUStates::kIdle_]);
  iowait = stol(cpu_stats_string[LinuxParser::CPUStates::kIOwait_]);
  irq = stol(cpu_stats_string[LinuxParser::CPUStates::kIRQ_]);
  softirq = stol(cpu_stats_string[LinuxParser::CPUStates::kSoftIRQ_]);
  steal = stol(cpu_stats_string[LinuxParser::CPUStates::kSteal_]);

  return idle + iowait + user + nice + system + irq + softirq + steal;
}

// Done: Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() {
  vector<string> cpu_stats_string = LinuxParser::CpuUtilization();
  long idle, iowait;
  idle = stol(cpu_stats_string[LinuxParser::CPUStates::kIdle_]);
  iowait = stol(cpu_stats_string[LinuxParser::CPUStates::kIOwait_]);
  return idle + iowait;
}

// Done: Read and return CPU utilization
vector<string> LinuxParser::CpuUtilization() {
  // string user, nice, system, idle, iowait, irq, softirq, steal, guest,
  // guest_nice ;
  vector<string> CpuUtil;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream) {
    string line;
    std::getline(filestream, line);
    std::stringstream ss(line);
    string cpu, user, nice, system, idle, iowait, irq, softirq, steal, guest,
        guest_nice;
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >>
        steal >> guest >> guest_nice;
    CpuUtil.emplace_back(user);
    CpuUtil.emplace_back(nice);
    CpuUtil.emplace_back(system);
    CpuUtil.emplace_back(idle);
    CpuUtil.emplace_back(iowait);
    CpuUtil.emplace_back(irq);
    CpuUtil.emplace_back(softirq);
    CpuUtil.emplace_back(steal);
    CpuUtil.emplace_back(guest);
    CpuUtil.emplace_back(guest_nice);
  }
  return CpuUtil;
}

// Done: Read and return the total number of processes
int LinuxParser::TotalProcesses() {
  std::ifstream filestream(kProcDirectory + kStatFilename);
  string line, key;
  int value = 0;
  if (filestream) {
    while (std::getline(filestream, line)) {
      std::stringstream ss(line);
      ss >> key >> value;
      if (key == "processes") break;
    }
  }
  return value;
}

// Done: Read and return the number of running processes
int LinuxParser::RunningProcesses() {
  std::ifstream filestream(kProcDirectory + kStatFilename);
  string line, key;
  int value = 0;
  if (filestream) {
    while (std::getline(filestream, line)) {
      std::stringstream ss(line);
      ss >> key >> value;
      if (key == "procs_running") break;
    }
  }
  return value;
}

// Done: Read and return the command associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Command(int pid) {
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + "/cmdline");
  string line;
  if (filestream) {
    getline(filestream, line);
  }
  return line;
}

// Done: Read and return the memory used by a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Ram(int pid) {
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + "/status");
  float value;
  if (filestream) {
    string line, key;

    while (getline(filestream, line)) {
      std::istringstream ss(line);
      ss >> key >> value;
      if (key == "VmSize:") break;
    }
  }
  return std::to_string(value / 1000);
}

// Done: Read and return the user ID associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Uid(int pid) {
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + "/status");
  string line, key, value1, value2, value3, value4;
  if (filestream) {
    while (getline(filestream, line)) {
      std::istringstream ss(line);
      ss >> key >> value1 >> value2 >> value3 >> value4;
      if (key == "Uid:") break;
    }
  }
  return value1;
}

// Done: Read and return the user associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::User(int pid) {
  string uid_target = Uid(pid);
  std::ifstream filestream(kPasswordPath);
  std::string line, user, uid;
  if (filestream) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::replace(line.begin(), line.end(), 'x', ' ');
      std::istringstream ss(line);
      ss >> user >> uid;
      // cout << "user: " << user << "\n";
      // cout << "uid: " << uid << "\n";
      if (uid == uid_target) break;
    }
  }
  return user;
}

// Done: Read and return the uptime of a process
// REMOVE: [[maybe_unused]] once you define the function
long LinuxParser::UpTime(int pid) {
  long value;
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + "/stat");
  if (filestream) {
    string line;
    std::getline(filestream, line);
    std::stringstream ss(line);
    std::vector<string> str_vec = string_to_vector(line);
    value = stol(str_vec[21]);
  }
  //std::cout << "...................." << "pid : "<< pid  << "uptime : "<< value << std::endl;
  return value;
}
