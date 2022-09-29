#include "main.hpp"

#include "OpenRGB/Exceptions.hpp"
#include "rgb_command.hpp"

#include <filesystem>
#include <fstream>
#include <csignal>
#include <gz-util/file_io.hpp>
#include <gz-util/settings_manager.hpp>
#include <gz-util/util/string.hpp>
#include <unordered_map>


using std::this_thread::sleep_for;
namespace fs = std::filesystem;

gz::Log rgblog(rgb::logfile, rgb::showLog, rgb::storeLog, "gz-rgb", gz::Color::MAGENTA);

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
    ProcessWatcher::ProcessWatcher(const std::vector<std::pair<std::string, std::string>>& settings) {
        size_t i = 0;
        for (auto it = settings.begin(); it != settings.end(); it++) {
            process2index[it->first] = i;
            i++;
        }
    }


    std::unordered_map<std::string, int>::const_iterator ProcessWatcher::processRunning() {
        fs::path proc("/proc");
        fs::path status("status");
        std::string name;
        name.reserve(16);
        std::unordered_map<std::string, int>::const_iterator it = process2index.end();
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
            if (name.empty()) {
                checkedPIDs.insert(pid);
                continue;
            }

            name.erase(0, 6);
            if (process2index.contains(name)) {
                /* rgblog("processRunning: Found process", name); */
                // check priorities
                if (processIndex < 0 or (process2index[name] > processIndex)) {
                    processIndex = process2index[name];
                    it = process2index.find(name);
                }
            }
            else {
                checkedPIDs.insert(pid);
            }
        }
        /* rgblog("processRunning: Returning", processIndex, process2SettingVec[processIndex].first); */
        return it;
    }


    // 
    // FILE WATCHER
    //
    FileWatcher::FileWatcher() : cmdDir(FILE_COMMAND_DIR) {
        if (!fs::is_directory(cmdDir)) {
            fs::create_directory(cmdDir);
            fs::permissions(cmdDir, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all);
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
    void App::rgbControllerThreadFunction(gz::Queue<RGBCommand>* q, std::atomic<int>* returnCode) {
        *returnCode = -1;
        unsigned int tries = 1;
        RGBController controller;
        while (tries <= MAX_TRY_TO_CONNCET) {
            try {
                controller.init(targetDeviceTypes);
                break;
            }
            catch (orgb::Exception& e) {
                if (tries == MAX_TRY_TO_CONNCET) {
                    rgblog.error("Could not connect to OpenRGB server. Try [", tries, "/", MAX_TRY_TO_CONNCET, "], giving up.");
                    *returnCode = 1;
                    return;
                }
                else {
                    rgblog.error("Could not connect to OpenRGB server. Is it running? Retrying in " + std::to_string(retryConnectDelay.count()) + "s. [", tries, "/", MAX_TRY_TO_CONNCET, "]");
                    sleep_for(retryConnectDelay);
                }
            }
            tries++;
        }
        bool running = true;
        while (running) {
            if (q->hasElement()) {
                /* auto vec = q->getInternalBuffer(); */
                /* for (size_t i = 0; i < vec.size(); i++) { */
                /*     std::cout << i << " - " << to_string(vec[i].setting.color) << '\n'; */
                /* } */

                RGBCommand command = q->getCopy();
                switch (command.type) {
                    case RGBCommandType::CHANGE_SETTING:
                        controller.changeSetting(command.setting);
                        break;
                    case RGBCommandType::RESUME_FROM_HIBERNATE:
                        controller.reSetSettings();
                        break;
                    case RGBCommandType::SLEEP:
                        /* rgblog("controllerThread sleeping for", rgbSleepCmdDuration.count()); */
                        sleep_for(rgbSleepCmdDuration);
                        break;
                    case RGBCommandType::QUIT:
                        running = false;
                        break;
                }
            }
            controller.update();
            sleep_for(rgbUpdateDuration);
        }
        *returnCode = 0;
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


    App::App(gz::SettingsManagerCreateInfo& smCI) : settings(smCI), q(4, 10), rgbControllerThread(rgbControllerThreadFunction, &q, &rgbControllerThreadReturnCode) {
        rgblog("Started gz-rgb");
        /* rgblog("Settings:", settings); */
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
        q.emplace_back(RGBCommand{ RGBCommandType::CHANGE_SETTING, settings.getOr<RGBSetting>("clearSetting", clearSetting) });

        auto settingsVector = gz::readKeyValueFile<std::vector<std::pair<std::string, std::string>>>(CONFIG_FILE);
        rgb::ProcessWatcher processWatcher(settingsVector);

        auto currentProcessNameIt = processWatcher.end();
        auto processNameIt = processWatcher.end();
        bool watchProcesses = false;

        rgb::FileWatcher fileWatcher;
        int cmdIndex = -1;

        bool checkTime = true;

        // for hibernation check
        std::chrono::time_point<std::chrono::system_clock> timeAtLastExecution = std::chrono::system_clock::now();
        
        bool running = true;
        while (running) {
            if (watchProcesses) {
                processNameIt = processWatcher.processRunning();
                if (processNameIt != currentProcessNameIt) {
                    if (processNameIt != processWatcher.end()) {
                        rgblog.clog(gz::Color::YELLOW, "Process Watcher", gz::Color::RESET, "Found new running process:", processNameIt->first);
                        RGBCommand command;
                        try {
                            command = { RGBCommandType::CHANGE_SETTING, settings.get<RGBSetting>(processNameIt->first) };
                        }
                        catch(gz::InvalidArgument& e) {
                            rgblog.error("Could not find setting for process: '" + processNameIt->first + "'. Using idleSetting.");
                            command = { RGBCommandType::CHANGE_SETTING, settings.getOr<RGBSetting>("idleSetting", idleSetting) };
                        }
                        catch(gz::InvalidType& e) {
                            rgblog.error("An error occured while trying to get setting for process: '" + processNameIt->first + "'. Using idleSetting. Error:", e.what());
                            command = { RGBCommandType::CHANGE_SETTING, settings.getOr<RGBSetting>("idleSetting", idleSetting) };
                        }

                        q.push_back(command);
                        currentProcessNameIt = processNameIt;
                    }
                    else {
                        rgblog.clog(gz::Color::YELLOW, "Process Watcher", gz::Color::RESET, "No wanted process found: Resetting color.");
                        RGBCommand command { RGBCommandType::CHANGE_SETTING, settings.getOr<RGBSetting>("idleSetting", idleSetting) };
                        q.push_back(command);
                        currentProcessNameIt = processWatcher.end();
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
                    currentProcessNameIt = processWatcher.end();
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
                    q.emplace_back(RGBCommand { RGBCommandType::SLEEP });
                    std::this_thread::sleep_for(waitForTimeWindow);
                }
            }

            // check if system was suspended or hibernated
            if (checkForHibernate) {
                auto now = std::chrono::system_clock::now();
                if (now - timeAtLastExecution > hibernateTimeThreshold) {
                    rgblog("Resume from hibernation detected.");
                    q.emplace_back(RGBCommand { RGBCommandType::RESUME_FROM_HIBERNATE });
                }
                timeAtLastExecution = std::move(now);
            }

            // check if the rgbControllerThread exited
            if (rgbControllerThreadReturnCode >= 0) {
                rgblog.error("rgbControllerThread has exited with code", rgbControllerThreadReturnCode, "- Exiting.");
                exit(1);
            }

            std::this_thread::sleep_for(manageRGBDuration);
        }  // while

        exit(0);
    }


    void App::exit(int exitcode) {
        q.emplace_back(RGBCommand{ RGBCommandType::CHANGE_SETTING, clearSetting });
        q.emplace_back(RGBCommand{ RGBCommandType::QUIT, idleSetting });
        rgbControllerThread.join();
        std::exit(exitcode);
    }
}


int main(int argc, char* argv[]) {
    /* rgb::waitForStart(); */
    gz::SettingsManagerCreateInfo smCI{};
    smCI.initialValues = {
        // rgb stuff
        { "clearSetting", gz::toString(rgb::clearSetting) },
        { "idleSetting", gz::toString(rgb::idleSetting) },
        /* { "FILE_COMMAND_DIR", rgb::FILE_COMMAND_DIR }, */
    };
    smCI.filepath = rgb::CONFIG_FILE;
    smCI.readFileOnCreation = true;

    rgb::App app(smCI);
    app.run();

    /* std::chrono::duration minute { 1min }; */
    /* for (int i = 0; i < 23; i++) { */
    /*     auto minutes = minute * 60 * i; */
    /*     std::cout << i << " - " << rgb::timeInWindow(minutes) << '\n'; */
    /* } */
    return 2;
}
