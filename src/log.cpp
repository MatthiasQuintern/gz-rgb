#include "log.hpp"

#include <filesystem>
#include <fstream>
#include <ctime>

#include <SFML/System/Vector2.hpp>

namespace gz {
    const char* COLORS[] = {
        "\033[0m", // RESET
        "\033[30m", // BLACK
        "\033[31m", // RED
        "\033[32m", // GREEN
        "\033[33m", // YELLOW
        "\033[34m", // BLUE
        "\033[35m", // MAGENTA
        "\033[36m", // CYAN
        "\033[37m", // WHITE
        "\033[1;30m", // BBLACK
        "\033[1;31m", // BRED
        "\033[1;32m", // BGREEN
        "\033[1;33m", // BYELLOW
        "\033[1;34m", // BBLUE
        "\033[1;35m", // BMAGENTA
        "\033[1;36m", // BCYAN
        "\033[1;37m", // BWHITE
    };


#ifdef LOG_MULTITHREAD 
    std::mutex Log::mtx;
#endif
    Log::Log(std::string logfile, bool showLog, bool storeLog, std::string&& prefix_, Color prefixColor)
        : showLog(showLog), storeLog(storeLog), prefixColor(prefixColor), prefix(prefix_ + ": "), prefixLength(prefix.size() + TIMESTAMP_CHAR_COUNT - 1) {
        // store the absolute path to the logfile
        logFile = std::string(std::filesystem::current_path().parent_path()) + "/logs/" + logfile;
        log("Initialising log with settings: logFile: " + logFile + 
                ", showLog - " + boolToString(showLog) + ", storeLog - " + boolToString(storeLog));
    }
    Log::~Log() {
        if (storeLog) { writeLog(); }
    }

    char* Log::getTime() {
        std::time_t t = std::time(0);
        struct std::tm *tmp;
        tmp = std::localtime(&t);
        
        //returs the date and time: yyyy-mm-dd hh:mm:ss:
        std::strftime(time, sizeof(time), "%F %T: ", tmp);
        return time;
    }

    
    void Log::writeLog() {
        std::ofstream file(logFile);
        if (file.is_open()) {
            for (std::string message : logArray) {
                file << message;
            }
            std::string message = getTime();
            message += "Written log to file: " + logFile + "\n";
            file << message;
            std::cout << message;
        }
        else {
            std::cout << COLORS[RED] << "LOG ERROR: " << COLORS[RESET] << "Could not open file '" << logFile << "'." << '\n';
        }
        file.close();
    }
} // namespace gz
