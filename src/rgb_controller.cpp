#include "rgb_controller.hpp"

#include <algorithm>
#include <cmath>

namespace rgb {
    bool isSameColor(const orgb::Color& color1, const orgb::Color& color2) {
        return color1.r == color2.r and color1.g == color2.g and color1.b == color2.b;
    }
    void stepToTargetNumber(uint8_t& i, const uint8_t targetNumber) {
        if (i < targetNumber) { 
            i += std::min(FADE_STEP_SIZE, targetNumber - i);
        }
        else if (i > targetNumber) { 
            i -= std::min(FADE_STEP_SIZE, i - targetNumber);
        }
    }
    void stepToTargetColor(orgb::Color& color, const orgb::Color& targetColor) {
        stepToTargetNumber(color.r, targetColor.r);
        stepToTargetNumber(color.g, targetColor.g);
        stepToTargetNumber(color.b, targetColor.b);
    }


    void simpleRainbowStep(orgb::Color& color, int i) {
        color.r = static_cast<uint8_t>(127 * std::sin((i * 2.0f / RAINBOW_STEP_COUNT + RED_PHASE)   * std::numbers::pi) + 128);
        color.g = static_cast<uint8_t>(127 * std::sin((i * 2.0f / RAINBOW_STEP_COUNT + GREEN_PHASE) * std::numbers::pi) + 128);
        color.b = static_cast<uint8_t>(127 * std::sin((i * 2.0f / RAINBOW_STEP_COUNT + BLUE_PHASE)  * std::numbers::pi) + 128);
    }


//
// RGBController
//
    void RGBController::init(const std::set<orgb::DeviceType>& targetDevices) {
        client.connectX(host, port);
        getDevices(targetDevices);
        setModes();
    }


    void RGBController::getDevices(const std::set<orgb::DeviceType>& targetDevices) {
        deviceList = client.requestDeviceListX();
        for (auto it = deviceList.begin(); it != deviceList.end(); it++) {
            rgblog.clog({ gz::Color::BLUE, gz::Color::RESET }, "Found device", orgb::enumString(it->type), it->vendor, it->name, "Zones:", it->zones.size(), "Leds:", it->leds.size(), "Colors:", it->colors.size());
            if (targetDevices.contains(it->type)) {
                devices.push_back(&(*it));
                deviceColors[&(*it)] = it->colors;
            }
        }
    }


    void RGBController::setModes() {
        for (auto it = devices.begin(); it != devices.end(); it++) {
            const orgb::Mode* mode = (*it)->findMode("Direct");
            if (mode == nullptr) {
                mode = (*it)->findMode("Static");
            }
            if (mode == nullptr) {
                rgblog.warning("Device" , (*it)->name, "does not have static or direct mode and will not be used");
                it = devices.erase(it);
                continue;
            }

            try {
                client.changeModeX(*(*it), *mode);
                /* log.warning("Would now change mode for device", (*it)->name); */
                rgblog("Changed mode for device", (*it)->name);
            } 
            catch (orgb::Exception& e) {
                rgblog.error("Device", (*it)->name, "Error during changeMode, removing device.", e.errorMessage());
                it = devices.erase(it);
                continue;
            }
        }
    }


    void RGBController::setColor(orgb::Device& device, orgb::Color color) {
        try {
            client.setDeviceColorX(device, color);
            deviceColors[&device].assign(deviceColors[&device].size(), color);
            /* log.warning("setColor: would now set deviceColor to", color.r, color.g, color.b); */
        } 
        catch (orgb::Exception& e) {
            rgblog.error("Device", device.name, "Error during setDeviceColor, skipping device.", e.errorMessage());
        }
    }


    void RGBController::setColor(orgb::Device& device, std::vector<orgb::Color> colors) {
        try {
            client.setDeviceLEDColorsX(device, colors);
            deviceColors[&device] = colors;
            /* log.warning("setColor: would now set deviceColor to", color.r, color.g, color.b); */
        } 
        catch (orgb::Exception& e) {
            rgblog.error("Device", device.name, "Error during setDeviceColor, skipping device.", e.errorMessage());
        }
    }


    void RGBController::changeSetting(const RGBSetting& setting) {
        /* rgblog("changeSetting", to_string(setting.color)); */
        switch(setting.mode) {
            case RAINBOW:
                for (orgb::Device* device : devices) {
                    if (setting.targetDevices.contains(device->type)) {
                        rgblog("Setting device", device->name, "to rainbow mode");
                        fadeMode.erase(device);
                        rainbowMode.insert(device);
                    }
                }
                break;
            case STATIC:
                if (setting.transition == INSTANT) {
                    for (orgb::Device* device : devices) {
                        if (setting.targetDevices.contains(device->type)) {
                            rgblog("Setting device", device->name, "to static mode without transition.");
                            rainbowMode.erase(device);
                            fadeMode.erase(device);
                            setColor(*device, setting.color);
                        }
                    }
                }
                else {
                    for (orgb::Device* device : devices) {
                        if (setting.targetDevices.contains(device->type)) {
                            rgblog("Setting device", device->name, "to static mode with transition");
                            rainbowMode.erase(device);
                            fadeMode[device] = setting.color;
                        }
                    }
                }
                break;
            case CLEAR:
                    for (orgb::Device* device : devices) {
                        if (setting.targetDevices.contains(device->type)) {
                            rgblog("Setting device", device->name, "to clear mode");
                            rainbowMode.erase(device);
                            fadeMode.erase(device);
                            setColor(*device, orgb::Color::Black);
                        }
                    }
                break;
        }
    }


    void RGBController::update() {
        // fade colors
        if (fadeMode.size() > 0) {
            for (const auto& [device, color] : fadeMode) {
                if (!isSameColor(deviceColors[device][0], color)) {
                    orgb::Color newColor = deviceColors[device][0];
                    stepToTargetColor(newColor, color);
                    setColor(*(device), newColor);
                } 
                else {
                    /* rgblog("Finished fading for device", device->name); */
                    fadeFinished.push_back(device);
                }
            }
            if (fadeFinished.size() > 0) {
                for (auto device : fadeFinished) {
                    fadeMode.erase(device);
                }
                fadeFinished.clear();
            }
        }
        if (rainbowMode.size() > 0) {
            orgb::Color newColor;
            simpleRainbowStep(newColor, rainbowStep);
            for (auto it = rainbowMode.begin(); it != rainbowMode.end(); it++) {
                std::rotate(deviceColors[*it].begin(), --deviceColors[*it].end(), deviceColors[*it].end());
                deviceColors[*it][0] = newColor;
                setColor(**it, deviceColors[*it]);
            }

            if (++rainbowStep > RAINBOW_STEP_COUNT) { rainbowStep = 0; }
        }
    }


    void RGBController::reSetSettings() {
        for (auto& [device, colors] : deviceColors) {
            setColor(*device, colors);
        }
    }



}

