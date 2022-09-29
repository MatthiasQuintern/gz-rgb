#pragma once

#include "rgb_command.hpp"
#include "rgb_controller.hpp"

#include <gz-util/container/queue.hpp>
#include <gz-util/settings_manager.hpp>
#include <gz-util/log.hpp>


#include <chrono>
#include <filesystem>
#include <functional>
#include <gz-util/util/string.hpp>
#include <thread>
#include <unordered_map>
#include <set>

using namespace std::chrono_literals;

namespace rgb {
    //
    // SETTINGS
    //
    /// The default device types affected by this program
    const std::set<orgb::DeviceType> targetDeviceTypes { 
        orgb::DeviceType::Motherboard, 
        orgb::DeviceType::DRAM, 
        orgb::DeviceType::Mouse,
        /* orgb::DeviceType::Keyboard */
    };

    // START TIME
    /// when to start and stop the rgb lighting. these must be in utc-0
    const std::chrono::duration startAt {18h + 30min};
    const std::chrono::duration stopAt {8h + 25min};

    /// rgb settings for when no targetet process is running
    const RGBSetting clearSetting { targetDeviceTypes, INSTANT, RGBMode::CLEAR, orgb::Color::Black };
    const RGBSetting idleSetting  { targetDeviceTypes, INSTANT, RGBMode::STATIC, orgb::Color(128, 128, 128) };

    /// rgb settings for each process. priority ~ index
    /* const std::vector<std::pair<std::string, RGBSetting>> processSettingVec { */
    /*     { "firefox",    { targetDeviceTypes, FADE,      RGBMode::STATIC,   orgb::Color(255, 128, 0) } }, */
    /*     { "vim",        { targetDeviceTypes, FADE,      RGBMode::STATIC,   orgb::Color(40, 200, 40) } }, */
    /*     { "catfish",    { targetDeviceTypes, INSTANT,   RGBMode::RAINBOW,  orgb::Color::Black }}, */
    /*     { "strawberry", { targetDeviceTypes, FADE,      RGBMode::STATIC,   orgb::Color(170, 20, 170) } }, */
    /*     { "mpv",        { targetDeviceTypes, FADE,      RGBMode::STATIC,   orgb::Color(0, 0, 255) } }, */
    /*     { "steam",      { targetDeviceTypes, INSTANT,   RGBMode::RAINBOW,  orgb::Color::Black } } */
    /* }; */


    /// External commands by placing files in FILE_COMMAND_DIR
    const std::vector<std::pair<std::string, RGBSetting>> externalCommandSettingVec {
        { "colorHex",           { targetDeviceTypes, FADE,      RGBMode::STATIC,   orgb::Color::Black } },
        { "process_watching",   idleSetting }, 
        { "quit",               idleSetting },
        { "rainbow",            { targetDeviceTypes, INSTANT,   RGBMode::RAINBOW,   orgb::Color::Black } },
        { "clear",              { targetDeviceTypes, INSTANT,   RGBMode::CLEAR,     orgb::Color::Black } },
    };
    const std::string FILE_COMMAND_DIR = "/tmp/gzrgb";
    const std::string CONFIG_FILE = "/etc/gz-rgb.conf";

    // ENERGY CONSUMPTION vs RESPONSIVENESS
    /// How long to sleep while waiting for the time window (main thread)
    const auto waitForTimeWindow = 15s;
    /// How long to sleep while active (main thread)
    const auto manageRGBDuration = 3s;
    /// How long to sleep between updates to rgb lighting (rgb controller thread)
    const auto rgbUpdateDuration = 33ms;  // ca 30 updates per second
    const auto rgbSleepCmdDuration = waitForTimeWindow - manageRGBDuration - rgbUpdateDuration;

    // HIBERNATION
    // resend the last command when coming out of hibernate
    const bool checkForHibernate = true;
    const auto hibernateTimeThreshold = (manageRGBDuration + waitForTimeWindow) * 1.1;

    // LOG
    const std::string logfile = "/var/log/gzrgb.log";
    const bool storeLog = true;
    const bool showLog = true;

    // ERROR
    const unsigned int MAX_TRY_TO_CONNCET = 5;
    const auto retryConnectDelay = 5s;

    //
    // TIME
    //
    bool timeInWindow();
    void waitForStart();

    // 
    // PROCESS WATCHER
    //
    class ProcessWatcher {
        public:
            /**
             * @brief Get the name and priority of the processes to watch for
             * @param settings A map containing process names as keys
             */
            ProcessWatcher(const std::vector<std::pair<std::string, std::string>>& settings);
            /**
             * @returns iterator to process-name - index pair with the highest priority that is running or end()
             */ 
            std::unordered_map<std::string, int>::const_iterator processRunning();
            std::unordered_map<std::string, int>::const_iterator end() const { return process2index.end(); };

        private:
            // index is the priority of the process
            std::unordered_map<std::string, int> process2index;
            // store pids that did not match the name to reduce file reading
            std::set<int> checkedPIDs;
    };


    //
    // FILE WATCHER
    //
    class FileWatcher {
        public:
            FileWatcher();
            int fileCommandReceived();
            orgb::Color getColor() { return color; }
            
            std::filesystem::path cmdDir;
            std::string colorHex = "colorHex";
            orgb::Color color;
    };


    class App {
        public:
            /**
             * @brief Creates the rgbControllerThread.
             * @details
             */
            App(gz::SettingsManagerCreateInfo& smCI);
            ~App();
            App(const App& app) = delete;
            App& operator=(const App&) = delete;
            /**
             * @brief Run gz-rgb
             * @details
             *  It does:
             *  - Time checking: check if the time is between startAt and stopAt, if true enable process watching
             *  - File Watching: check if a command is sent through a created file in FILE_COMMAND_DIR
             *  - Process Watching: check if a wanted process from process2SettingVec is running
             *  - When necessary through one of the above, send RGBCommand through the q to the RGBController thread
             */
            void run();
        private:
            gz::SettingsManager<RGBSetting> settings;
            gz::Queue<RGBCommand> q;
            std::thread rgbControllerThread;

            /// join rgbControllerThread ans exit
            void exit(int exitcode);

            std::atomic<int> rgbControllerThreadReturnCode = 0;

            static App* app;
            /**
             * @brief Send QUIT command and join other thread before exiting
             */
            static void handleSignal(int sig);
            /**
             * @brief Creates a RGBController and waits for commands
             * @param q: The q with commands to send to the controller
             * @param returnCode: A code that is >= 0 when the function exits, and -1 while running 
             */
            static void rgbControllerThreadFunction(gz::Queue<RGBCommand>* q, std::atomic<int>* returnCode);
    };
}
