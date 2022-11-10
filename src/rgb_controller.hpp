#pragma once 

#include "OpenRGB/DeviceInfo.hpp"
#include "rgb_command.hpp"

#include "OpenRGB/Client.hpp"

#include <gz-util/log.hpp>

#include <unordered_map>
#include <map>
#include <set>

#include <gz-util/string/conversion.hpp>

// TODO remove
/* static_assert(gz::ConstructibleFromString<rgb::RGBSetting>, "error"); */
/* static_assert(gz::ConvertibleToString<orgb::DeviceType>, "error"); */
/* static_assert(gz::util::ConvertibleToStringGlobal<rgb::RGBSetting>, "error"); */
/* static_assert(gz::util::ConvertibleToStringGlobal<orgb::DeviceType>, "error"); */
/* static_assert(gz::ConvertibleToString<rgb::RGBSetting>, "error"); */
/* static_assert(gz::ConvertibleToString<orgb::DeviceType>, "error"); */
/* static_assert(gz::StringConvertible<rgb::RGBSetting>, "error"); */


extern gz::Log rgblog;

namespace rgb {

    const std::string host = "127.0.0.1";
    const uint16_t port = 6742;
    const std::string clientName = "gzrgb";

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
            RGBController() : client(clientName) {};
            /**
             * @brief Initialize the controller.
             * @details
             *  Connects to OpenRGB server and sets the device modes
             */
            void init(const std::set<orgb::DeviceType>& targetDevices);
            /**
             * Change a rgb setting. Fade and rainbow devices will be added to fadeMode or rainbowMode containers
             * and will be processed during update()
             */
            void changeSetting(const RGBSetting& setting);
            void update();

            void setColor(orgb::Device& device, orgb::Color color);
            void setColor(orgb::Device& device, std::vector<orgb::Color> colors);

            /**
             * @brief Re-set all colors for all devices.
             * @details
             *  Calls setColor for all devices and colors stored in deviceColors.
             *  Useful when another program or hibernation changed to colors
             */
            void reSetSettings();

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
