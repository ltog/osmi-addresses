#ifndef MEMHELPER_HPP_
#define MEMHELPER_HPP_

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

// from: http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process

class MemHelper {

public:
	MemHelper() {

	}

	void start() {
		if (active == true) {
			std::cout << "ERROR: MemHelper was already started." << std::endl;
		}
		active = true;
		mem_used = getVmRSS();
	}

	void stop() {
		if (active == false) {
			std::cout << "ERROR: MemHelper was not yet started." << std::endl;
		}
		active = false;
		int difference = getVmRSS() - mem_used;

		std::cout << "Difference (VmRSS): " << difference << " KB ; Current value (VmRSS) = " << getVmRSS() << " KB"<< std::endl;
	}

	void print_max() {
		std::cout << "Peak virtual memory usage (VmPeak) = " << getVmPeak() << " kB" << std::endl;
		std::cout << "Peak physical memory usage (VmHWM) = " << getVmHWM()  << " kB" << std::endl;
	}

private:
	bool active = false;
	int mem_used = 0;

    int parseLine(char* line){
        int i = strlen(line);
        while (*line < '0' || *line > '9') line++;
        line[i-3] = '\0';
        i = atoi(line);
        return i;
    }


    int getVmSize(){ //Note: this value is in KB!
        FILE* file = fopen("/proc/self/status", "r");
        int result = -1;
        char line[128];


        while (fgets(line, 128, file) != NULL){
            if (strncmp(line, "VmSize:", 7) == 0){
                result = parseLine(line);
                break;
            }
        }
        fclose(file);
        return result;
    }

    int getVmPeak(){
        FILE* file = fopen("/proc/self/status", "r");
        int result = -1;
        char line[128];


        while (fgets(line, 128, file) != NULL){
            if (strncmp(line, "VmPeak:", 7) == 0){
                result = parseLine(line);
                break;
            }
        }
        fclose(file);
        return result;
    }

    int getVmRSS(){
        FILE* file = fopen("/proc/self/status", "r");
        int result = -1;
        char line[128];


        while (fgets(line, 128, file) != NULL){
            if (strncmp(line, "VmRSS:", 6) == 0){
                result = parseLine(line);
                break;
            }
        }
        fclose(file);
        return result;
    }

    int getVmHWM(){
        FILE* file = fopen("/proc/self/status", "r");
        int result = -1;
        char line[128];


        while (fgets(line, 128, file) != NULL){
            if (strncmp(line, "VmHWM:", 6) == 0){
                result = parseLine(line);
                break;
            }
        }
        fclose(file);
        return result;
    }

};



#endif /* MEMHELPER_HPP_ */
