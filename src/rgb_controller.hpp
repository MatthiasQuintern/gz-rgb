#include "log.hpp"

#include "OpenRGB/Client.hpp"

#include <unordered_map>
#include <map>

#include <set>

extern gz::Log rgblog;

namespace rgb {

    const std::string host = "127.0.0.1";
    const uint16_t port = 6742;
    const std::string clientName = "gzrgb";

    enum RGBMode {
        RAINBOW, STATIC, CLEAR
    };
    enum RGBTransition {
        FADE, INSTANT,
    };
    struct RGBSetting {
        std::set<orgb::DeviceType> targetDevices;
        RGBTransition transition;
        RGBMode mode;
        orgb::Color color;
    };

    enum RGBCommandType {
        CHANGE_SETTING, QUIT
    };
    struct RGBCommand {
        RGBCommandType type;
        RGBSetting setting;
    };


    std::string to_string(const orgb::Color& color);

    // fade
    bool isSameColor(const orgb::Color& color1, const orgb::Color& color2);
    const int FADE_STEP_SIZE = 10;
    void stepToTargetColor(orgb::Color& color, const orgb::Color& targetColor);

    // rainbow
    const int RAINBOW_STEP_COUNT = 50;
    const float RED_PHASE = 0;
    const float BLUE_PHASE = 2.0f / 3;
    const float GREEN_PHASE = 4.0f / 3;


    class RGBController {
        public:
            RGBController(const std::set<orgb::DeviceType>& targetDevices);
            /**
             * Change a rgb setting. Fade and rainbow devices will be added to fadeMode or rainbowMode containers
             * and will be processed during update()
             */
            void changeSetting(const RGBSetting& setting);
            void update();

            void setColor(orgb::Device& device, orgb::Color color);
            void setColor(orgb::Device& device, std::vector<orgb::Color> colors);


        private:
            orgb::Client client;
            void getDevices(const std::set<orgb::DeviceType>& targetDevices);
            void setModes();

            // All devices
            orgb::DeviceList deviceList;
            // Target devices, pointers to deviceList elements
            std::vector<orgb::Device*> devices;
            //
            // Storing and updating the device colors here rather than refreshing the device list all the time
            std::map<orgb::Device*, std::vector<orgb::Color>> deviceColors;
            // Active devices
            std::set<orgb::Device*> rainbowMode;
            int rainbowStep;
            std::map<orgb::Device*, orgb::Color> fadeMode;
            std::vector<orgb::Device*> fadeFinished;
    };


}
