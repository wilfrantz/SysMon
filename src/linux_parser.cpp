#include "linux_parser.h"

#include <dirent.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using std::stof;
using std::string;
using std::to_string;
using std::vector;

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
  string os, kernel, version;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
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
      }
    }
  }
  closedir(directory);
  return pids;
}

// Read and return the system memory utilization
// NOTE: Information about memory utilization exists in the `/proc/meminfo`
float LinuxParser::MemoryUtilization() {
  float data(0.0f);
  float memFree(0.0f);
  float memTotal(0.0f);

  std::string line, key;
  std::unordered_map<std::string, float> dataMap;
  std::ifstream meminfofile(kProcDirectory + kMeminfoFilename);

  if (meminfofile.is_open()) {
    while (meminfofile >> key >> data) {
      dataMap[key] = data;
    }
  }

  if (dataMap.find("MemTotal:") != dataMap.end())
    memTotal = dataMap["MemTotal:"];

  if (dataMap.find("MemFree:") != dataMap.end()) memFree = dataMap["MemFree:"];

  return (memTotal - memFree);
}

// Read and return the system uptime
// NOTE: Information about system up time exists in the /proc/uptime file.
// This file contains two numbers (values in seconds):
//    1- the uptime of the system (including time spent in suspend) and
//    2- the amount of time spent in the idle process.
long LinuxParser::UpTime() {
  std::string line, uptimeStr;

  std::ifstream uptimeFile(kProcDirectory + kUptimeFilename);

  if (uptimeFile.is_open()) {
    while (std::getline(uptimeFile, line)) {
      std::istringstream linestream(line);
      linestream >> uptimeStr;
    }
  }

  return stol(uptimeStr);
}

// Read and return the number of jiffies for the system
// NOTE: “jiffies” holds the number of ticks that have occurred since the system
// booted.
// https://www.linkedin.com/pulse/linux-kernel-system-timer-jiffies-mohamed-yasser/
long LinuxParser::Jiffies() {
  std::string line, key, data;
  std::ifstream jifFile(kProcDirectory + kTimerListFilename);

  if (jifFile.is_open()) {
    while (std::getline(jifFile, line)) {
      std::istringstream lineStream(line);
      lineStream >> key >> data;

      if (key == "jiffies") {
        return std::stol(data);
      }
    }
  }
  return 0.0;
}

// Read and return the number of active jiffies for a PID
long LinuxParser::ActiveJiffies(int pid) {
  std::string line, data;
  std::vector<string> activeJiffies;
  std::ifstream jifFile(kProcDirectory + to_string(pid) + kStatFilename);

  if (jifFile.is_open()) {
    while (std::getline(jifFile, line)) {
      std::istringstream lineStream(line);
      lineStream >> data;
      activeJiffies.push_back(data);
    }
  }

  if (activeJiffies.size() > 14)
    return stol(activeJiffies[13] + activeJiffies[14]);
  else
    return 0.0;
}

// Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() {
  std::vector<std::string> cpu = CpuUtilization();

  return std::stol(cpu[CPUStates::kUser_]) + std::stol(cpu[CPUStates::kNice_]) +
         std::stol(cpu[CPUStates::kSystem_]) +
         std::stol(cpu[CPUStates::kIRQ_]) +
         std::stol(cpu[CPUStates::kSoftIRQ_]) +
         std::stol(cpu[CPUStates::kSteal_]);
}

// Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() {
  std::vector<std::string> cpu = CpuUtilization();

  return std::stol(cpu[CPUStates::kIdle_]) +
         std::stol(cpu[CPUStates::kIOwait_]);
}

// Read and return CPU utilization
vector<string> LinuxParser::CpuUtilization() {
  std::string line, key, data;
  std::vector<std::string> cpuCore;
  std::ifstream cpuFD(kProcDirectory + kStatFilename);

  if (cpuFD.is_open()) {
    while (std::getline(cpuFD, line)) {
      std::istringstream lineStream(line);
      lineStream >> key;
      if (key == "cpu") {
        for (int i = 0; i < CPUStates::kGuestNice_; i++) {
          lineStream >> data;
          cpuCore.push_back(data);
        }
      }
    }
  }

  return {};
}

// Read and return the total number of processes
int LinuxParser::TotalProcesses() {
  std::string line, key, data;
  std::ifstream streamFD(kProcDirectory + kMeminfoFilename);

  if (streamFD.is_open()) {
    while (std::getline(streamFD, line)) {
      std::istringstream lineStream(line);
      lineStream >> key >> data;
      if (key == "processes") return std::stoi(data);
    }
  }

  return -1;
}

// Read and return the number of running processes
int LinuxParser::RunningProcesses() {
  std::string line, key, data;
  std::ifstream streamFD(kProcDirectory + kMeminfoFilename);

  if (streamFD.is_open()) {
    while (std::getline(streamFD, line)) {
      std::istringstream lineStream(line);
      lineStream >> key >> data;
      if (key == "procs_running") return std::stoi(data);
    }
  }
  return (-1);
}

// Read and return the command associated with a process
string LinuxParser::Command(int pid) {
  std::string line, cmd;
  std::ifstream commandFD(kProcDirectory + std::to_string(pid) +
                          kCmdlineFilename);

  if (commandFD.is_open()) {
    while (std::getline(commandFD, line)) {
      return line;
    }
  }

  return string();
}

// Read and return the memory used by a process
string LinuxParser::Ram(int pid) {
  std::string line, key, data;
  std::ifstream ramFD(kProcDirectory + std::to_string(pid) + kStatusFilename);

  if (ramFD.is_open()) {
    while (std::getline(ramFD, line)) {
      std::istringstream lineStream(line);
      lineStream >> key >> data;

      if (key == "Mems_allowed:") return data;
    }
  }

  return string();
}

// Read and return the user ID associated with a process
string LinuxParser::Uid(int pid) {
  std::string line, key, data;
  std::ifstream uidFD(kProcDirectory + std::to_string(pid) + kStatusFilename);

  if (uidFD.is_open()) {
    while (std::getline(uidFD, line)) {
      std::istringstream lineSteam(line);
      lineSteam >> key >> data;
      if (key == "Uid:") return data;
    }
  }

  return string();
}

// Read and return the user associated with a process
string LinuxParser::User(int pid) {
  std::string userID = Uid(pid);

  // Get the username from /etc/passwd
  std::string line, username, x, uid;
  std::ifstream userFD(kPasswordPath);

  if (userFD.is_open()) {
    while (std::getline(userFD, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream lineStream(line);
      lineStream >> username >> x >> uid;

      if (uid == userID) return username;
    }
  }

  return string();
}

// Read and return the uptime of a process
long LinuxParser::UpTime(int pid) {
  std::string line, key, data;
  std::ifstream uptimeFD(kProcDirectory + std::to_string(pid) + kStatFilename);
  if (uptimeFD.is_open()) {
    std::getline(uptimeFD, line);
    return stol(line) / sysconf(_SC_CLK_TCK);
  }

  return 0;
}
