#include "main.hpp"

#include <filesystem>
#include <fstream>

using std::this_thread::sleep_for;
namespace fs = std::filesystem;

gz::Log rgblog(rgb::logfile, rgb::showLog, rgb::storeLog, "gzrgb", gz::Color::MAGENTA);

namespace rgb {
    // 
    // TIME
    //
    bool timeInWindow() {
        auto now = std::chrono::system_clock::now();
        auto days = std::chrono::time_point_cast<std::chrono::days>(now);
        auto dayTimeInMinutes = std::chrono::duration_cast<std::chrono::minutes>(now - days);
        auto untilStart = rgb::startAt - dayTimeInMinutes;
        auto untilStop = rgb::stopAt - dayTimeInMinutes;

        // 0---start----------stop------------24
        if (startAt <= stopAt) {
            return untilStart.count() < 0 and untilStop.count() > 0;
        }
        // 0---stop------------start---------24
        else {
            return untilStart.count() < 0 or untilStop.count() > 0;
        }
    }


    void waitForStart() {
        bool waitForStart = true;
        while (waitForStart) {
            if (timeInWindow()) {
                waitForStart = false;
            } else {
                sleep_for(waitForStartDuration);
            }
        }
    }


    //
    // PROCESS WATCHER
    //
    ProcessWatcher::ProcessWatcher() {
        for (size_t i = 0; i < processSettingVec.size(); i++) {
            process2index[processSettingVec[i].first] = i;
        }
    }


    int ProcessWatcher::processRunning() {
        fs::path proc("/proc");
        fs::path status("status");
        std::string name;
        name.reserve(16);
        int pid;
        int processIndex = -1;
        for (const auto& entry : fs::directory_iterator(proc)) {
            if (!fs::is_directory(entry)) { continue; }
            try {
                pid = std::stoi(entry.path().filename().c_str());
            } 
            catch (std::invalid_argument& e) { continue; }
            if (checkedPIDs.contains(pid)) { continue; }
            if (!fs::is_regular_file(entry.path() / status)) {
                checkedPIDs.insert(pid);
                continue;
            }
            std::ifstream cmdlineFile(entry.path() / status);
            getline(cmdlineFile, name);
            /* std::cout << "pid " << pid << " - name " << name << '\n'; */
            if (name.empty()) {
                checkedPIDs.insert(pid);
                continue;
            }

            name.erase(0, 6);
            if (process2index.contains(name)) {
                /* rgblog("processRunning: Found process", name); */
                if (processIndex < 0 or (process2index[name] > processIndex)) {
                    processIndex = process2index[name];
                }
            }
            else {
                checkedPIDs.insert(pid);
            }
        }
        /* rgblog("processRunning: Returning", processIndex, process2SettingVec[processIndex].first); */
        return processIndex;
    }


    // 
    // FILE WATCHER
    //
    FileWatcher::FileWatcher() : cmdDir(FILE_COMMAND_DIR) {
        if (!fs::is_directory(cmdDir)) {
            fs::create_directory(cmdDir);
        }
    }


    int FileWatcher::fileCommandReceived() {
        int cmdIndex = -1;
        for (const auto& entry : fs::directory_iterator(cmdDir)) {
            if (fs::is_regular_file(entry)) {
                rgblog("FileWatcher::fileCommandReceived:", entry.path().filename().c_str());
                rgblog(colorHex.compare(0, colorHex.size(), entry.path().filename().c_str()));
                if (entry.path().filename().string().compare(0, colorHex.size(), colorHex) == 0) {
                    rgblog(entry.path().filename().string().substr(colorHex.size()));
                    color.fromString(entry.path().filename().string().substr(colorHex.size()));
                    cmdIndex = 0;
                }
                else {
                    for (size_t i = 0; i < externalCommandSettingVec.size(); i++) {
                        if (entry.path().filename().c_str() == externalCommandSettingVec[i].first) {
                            cmdIndex = i;
                        }
                    }
                }
                fs::remove(entry);
            }
        }
        return cmdIndex;
    }


    // 
    // RGB THREAD
    //
    void rgbControllerThreadFunction(gz::util::Queue<RGBCommand>* q) {
        RGBController controller(targetDeviceTypes);
        bool running = true;
        while (running) {
            if (q->hasElement()) {
                auto vec = q->getInternalBuffer();
                for (size_t i = 0; i < vec.size(); i++) {
                    std::cout << i << " - " << to_string(vec[i].setting.color) << '\n';
                }

                RGBCommand command = q->getCopy();
                if (command.type == RGBCommandType::QUIT) {
                    running = false;
                }
                else {
                    controller.changeSetting(command.setting);
                }
            }
            controller.update();
            sleep_for(rgbUpdateDuration);
        }
    }


    //
    // CONTROLLER THREAD
    //
    void manageRGB() {
        gz::util::Queue<RGBCommand> q(4, 20);
        std::thread rgbControllerThread(rgbControllerThreadFunction, &q);

        q.push_back(RGBCommand{ RGBCommandType::CHANGE_SETTING, defaultSetting });

        rgb::ProcessWatcher processWatcher;
        int currentProcessIndex = -1;
        int processIndex;
        bool watchProcesses = true;

        rgb::FileWatcher fileWatcher;
        int cmdIndex = -1;

        bool running = true;
        while (running) {
            if (watchProcesses) {
                processIndex = processWatcher.processRunning();
                if (processIndex != currentProcessIndex) {
                    if (processIndex >= 0) {
                        rgblog("Process Watcher: Found new running process:", processSettingVec[processIndex].first, to_string(processSettingVec[processIndex].second.color));
                        RGBCommand command { RGBCommandType::CHANGE_SETTING, processSettingVec[processIndex].second };
                        q.push_back(command);
                        currentProcessIndex = processIndex;
                    }
                    else {
                        rgblog("Process Watcher: No wanted process found: Resetting color.");
                        RGBCommand command { RGBCommandType::CHANGE_SETTING, defaultSetting };
                        q.push_back(command);
                        currentProcessIndex = -1;
                    }
                }
            }

            cmdIndex = fileWatcher.fileCommandReceived();
            if (cmdIndex >= 0) {
                if (cmdIndex == 0) {
                    watchProcesses = false;
                    RGBCommand command { RGBCommandType::CHANGE_SETTING, externalCommandSettingVec[cmdIndex].second };
                    command.setting.color = fileWatcher.getColor();
                    q.push_back(command);
                    rgblog("File Watcher: Setting color from hex.");
                }
                else if (cmdIndex == 1) {
                    rgblog("File Watcher: Starting process watching.");
                    watchProcesses = true;
                    currentProcessIndex = -1;
                }
                else if (cmdIndex == 2) {
                    rgblog("File Watcher: Quit command received");
                    running = false;
                }
                else {
                    rgblog("File Watcher:", externalCommandSettingVec[cmdIndex].first);
                    watchProcesses = false;
                    RGBCommand command { RGBCommandType::CHANGE_SETTING, externalCommandSettingVec[cmdIndex].second };
                    q.push_back(command);
                }
            }
            std::this_thread::sleep_for(manageRGBDuration);
        }  // while

        q.push_back(RGBCommand{ RGBCommandType::QUIT, defaultSetting });
        rgbControllerThread.join();
    }
}


int main(int argc, char* argv[]) {
    rgblog("Starting gzrgb");
    /* rgb::waitForStart(); */

    rgb::manageRGB();

    /* std::chrono::duration minute { 1min }; */
    /* for (int i = 0; i < 23; i++) { */
    /*     auto minutes = minute * 60 * i; */
    /*     std::cout << i << " - " << rgb::timeInWindow(minutes) << '\n'; */
    /* } */
    return 0;
}
