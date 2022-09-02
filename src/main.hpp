#include "queue.hpp"
#include "rgb_controller.hpp"
#include "log.hpp"

#include <chrono>
#include <filesystem>
#include <functional>
#include <thread>
#include <unordered_map>
#include <set>

using namespace std::chrono_literals;

namespace rgb {
    //
    // SETTINGS
    //
    /// The device types affected by this program
    const std::set<orgb::DeviceType> targetDeviceTypes { 
        orgb::DeviceType::Motherboard, 
        orgb::DeviceType::DRAM, 
        orgb::DeviceType::Mouse,
        /* orgb::DeviceType::Keyboard */
    };

    /// when to start and stop the rgb lighting. these must be in utc-0
    const std::chrono::duration startAt {18h + 30min};
    const std::chrono::duration stopAt {8h + 25min};

    /// rgb settings for when no targetet process is running
    const RGBSetting defaultSetting { targetDeviceTypes, INSTANT, RGBMode::STATIC, orgb::Color(128, 128, 128) };
    /// rgb settings for each process. priority ~ index
    const std::vector<std::pair<std::string, RGBSetting>> processSettingVec {
        { "firefox",    { targetDeviceTypes, FADE,      RGBMode::STATIC,   orgb::Color(255, 128, 0) } },
        { "vim",        { targetDeviceTypes, FADE,      RGBMode::STATIC,   orgb::Color(40, 200, 40) } },
        { "catfish",    { targetDeviceTypes, INSTANT,   RGBMode::RAINBOW,  orgb::Color::Black }},
        { "strawberry", { targetDeviceTypes, FADE,      RGBMode::STATIC,   orgb::Color(170, 20, 170) } },
        { "mpv",        { targetDeviceTypes, FADE,      RGBMode::STATIC,   orgb::Color(0, 0, 255) } },
        { "steam",      { targetDeviceTypes, INSTANT,   RGBMode::RAINBOW,  orgb::Color::Black } }
    };

    /// External commands by placing files in FILE_COMMAND_DIR
    const std::vector<std::pair<std::string, RGBSetting>> externalCommandSettingVec {
        { "colorHex",           { targetDeviceTypes, FADE,      RGBMode::STATIC,   orgb::Color::Black } },
        { "process_watching",   defaultSetting }, 
        { "quit",               defaultSetting },
        { "rainbow",            { targetDeviceTypes, INSTANT,   RGBMode::RAINBOW,   orgb::Color::Black } },
        { "clear",              { targetDeviceTypes, INSTANT,   RGBMode::CLEAR,     orgb::Color::Black } },
    };
    const std::string FILE_COMMAND_DIR = "/tmp/gzrgb";

    // Energy
    const auto rgbUpdateDuration = 33ms;  // ca 30 updates per second
    const auto waitForStartDuration = 10s;
    const auto manageRGBDuration = 3s;

    const std::string logfile = "gzrgb.log";




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
             * @brief Generate process2priority and process2Setting from process2SettingVec
             */
            ProcessWatcher();
            /**
             * @returns iterator to process-name - setting pair with the highest priority that is running or process2Setting.end()
             */ 
            int processRunning();
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
}
