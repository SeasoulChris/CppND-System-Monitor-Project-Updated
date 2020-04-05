#include "processor.h"
#include "process.h"
#include "stdio.h"
#include <unistd.h>
#include "linux_parser.h"
#include <iostream>

// Done: Return the aggregate CPU utilization
float Processor::Utilization() {
    float active_jiffies = LinuxParser::ActiveJiffies();
    float jiffies = LinuxParser::Jiffies();
    //std::cout << "..........................: " << active_jiffies/jiffies << "\n"; 
    return active_jiffies/jiffies;
}