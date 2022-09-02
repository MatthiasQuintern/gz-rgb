#include "main.hpp"

#include <filesystem>
#include <fstream>
#include <csignal>

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
                sleep_for(waitForTimeWindow);
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
    void App::rgbControllerThreadFunction(gz::util::Queue<RGBCommand>* q) {
        RGBController controller(targetDeviceTypes);
        bool running = true;
        while (running) {
            if (q->hasElement()) {
                /* auto vec = q->getInternalBuffer(); */
                /* for (size_t i = 0; i < vec.size(); i++) { */
                /*     std::cout << i << " - " << to_string(vec[i].setting.color) << '\n'; */
                /* } */

                RGBCommand command = q->getCopy();
                if (command.type == RGBCommandType::QUIT) {
                    running = false;
                } 
                else if (command.type == RGBCommandType::SLEEP) {
                    /* rgblog("controllerThread sleeping for", rgbSleepCmdDuration.count()); */
                    sleep_for(rgbSleepCmdDuration);
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
    // App
    //
    App* App::app = nullptr;

    void App::handleSignal(int sig) {
        rgblog.clog(gz::Color::YELLOW, "Signal handler:", gz::Color::RESET, "Received signal", sig);
        if (app != nullptr) {
            app->q.emplace_back(RGBCommand{ RGBCommandType::CHANGE_SETTING, clearSetting });
            rgblog.clog(gz::Color::YELLOW, "Signal handler:", gz::Color::RESET, "Joining thread. This might take up to", std::chrono::duration_cast<std::chrono::seconds>(rgbSleepCmdDuration).count(), "seconds.");
            app->q.emplace_back(RGBCommand{ RGBCommandType::QUIT, idleSetting });
            app->rgbControllerThread.join();
            rgblog.clog(gz::Color::YELLOW, "Signal handler:", gz::Color::RESET, "Thread joined. Exiting");
            std::exit(0);
        } 
        else {
            rgblog.clog(gz::Color::YELLOW, "Signal handler:", gz::Color::RESET, "Error: Could not find running instance of App");
            std::exit(1);
        }
    }


    App::App() : q(4, 10), rgbControllerThread(rgbControllerThreadFunction, &q) {
        rgblog("Started gz-rgb");
        if (app != nullptr) {
            rgblog.error("App::App(): Another instance is already created.");
        } 
        else {
            app = this;
        }
    }
    App::~App() {
        app = nullptr;
    }

    void App::run() {
        std::signal(SIGTERM, &App::handleSignal);
        std::signal(SIGINT, &App::handleSignal);
        q.emplace_back(RGBCommand{ RGBCommandType::CHANGE_SETTING, clearSetting });

        rgb::ProcessWatcher processWatcher;
        int currentProcessIndex = -1;
        int processIndex;
        bool watchProcesses = false;

        rgb::FileWatcher fileWatcher;
        int cmdIndex = -1;

        bool checkTime = true;

        bool running = true;
        while (running) {
            if (watchProcesses) {
                processIndex = processWatcher.processRunning();
                if (processIndex != currentProcessIndex) {
                    if (processIndex >= 0) {
                        rgblog.clog(gz::Color::YELLOW, "Process Watcher", gz::Color::RESET, "Found new running process:", processSettingVec[processIndex].first, to_string(processSettingVec[processIndex].second.color));
                        RGBCommand command { RGBCommandType::CHANGE_SETTING, processSettingVec[processIndex].second };
                        q.push_back(command);
                        currentProcessIndex = processIndex;
                    }
                    else {
                        rgblog.clog(gz::Color::YELLOW, "Process Watcher", gz::Color::RESET, "No wanted process found: Resetting color.");
                        RGBCommand command { RGBCommandType::CHANGE_SETTING, idleSetting };
                        q.push_back(command);
                        currentProcessIndex = -1;
                    }
                }
            }

            // watch files
            cmdIndex = fileWatcher.fileCommandReceived();
            if (cmdIndex >= 0) {
                checkTime = false;
                if (cmdIndex == 0) {
                    rgblog.clog(gz::Color::CYAN, "File Watcher", gz::Color::RESET, "Setting color from hex.");
                    watchProcesses = false;
                    RGBCommand command { RGBCommandType::CHANGE_SETTING, externalCommandSettingVec[cmdIndex].second };
                    command.setting.color = fileWatcher.getColor();
                    q.emplace_back(std::move(command));
                }
                else if (cmdIndex == 1) {
                    rgblog.clog(gz::Color::CYAN, "File Watcher", gz::Color::RESET, "Starting process watching.");
                    watchProcesses = true;
                    currentProcessIndex = -1;
                }
                else if (cmdIndex == 2) {
                    rgblog.clog(gz::Color::CYAN, "File Watcher", gz::Color::RESET, "Quit command received");
                    running = false;
                }
                else {
                    rgblog.clog(gz::Color::CYAN, "File Watcher", gz::Color::RESET, externalCommandSettingVec[cmdIndex].first);
                    watchProcesses = false;
                    RGBCommand command { RGBCommandType::CHANGE_SETTING, externalCommandSettingVec[cmdIndex].second };
                    q.emplace_back(std::move(command));
                }
            }

            // check time
            if (checkTime) {
                if (timeInWindow()) {
                    rgblog("Now in time window - enabling process watching");
                    checkTime = false;
                    watchProcesses = true;
                }
                else {
                    q.emplace_back(RGBCommand {RGBCommandType::SLEEP, idleSetting });
                    std::this_thread::sleep_for(waitForTimeWindow);
                }
            }

            std::this_thread::sleep_for(manageRGBDuration);
        }  // while

        q.emplace_back(RGBCommand{ RGBCommandType::CHANGE_SETTING, clearSetting });
        q.emplace_back(RGBCommand{ RGBCommandType::QUIT, idleSetting });
        rgbControllerThread.join();
    }
}


int main(int argc, char* argv[]) {
    /* rgb::waitForStart(); */

    rgb::App app;
    app.run();

    /* std::chrono::duration minute { 1min }; */
    /* for (int i = 0; i < 23; i++) { */
    /*     auto minutes = minute * 60 * i; */
    /*     std::cout << i << " - " << rgb::timeInWindow(minutes) << '\n'; */
    /* } */
    return 0;
}
