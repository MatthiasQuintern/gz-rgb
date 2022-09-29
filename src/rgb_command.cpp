#include "rgb_command.hpp"
#include "OpenRGB/Color.hpp"
#include "OpenRGB/DeviceInfo.hpp"

#include <gz-util/util/string.hpp>
#include <gz-util/util/string_conversion.hpp>
#include <gz-util/exceptions.hpp>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <map>

// COLOR
void toTwoDigitHex(std::string& appendTo, const uint8_t color) {
    std::stringstream ss;
    ss << std::hex << color;
    // make sure it has two digits
    std::string temp = ss.str();
    /* std::cout << "hex:" << color << " - " << temp << "\n"; */
    if (temp.size() == 1) { appendTo += "0" + temp; }
    else { appendTo += temp; }
}

std::string toString(const orgb::Color& c) {
    std::string s = "#";
    toTwoDigitHex(s, c.r);
    toTwoDigitHex(s, c.g);
    toTwoDigitHex(s, c.b);
    return s;
}

template<> orgb::Color fromString<orgb::Color>(const std::string& s) {
    orgb::Color color;
    color.fromString(s);
    return color;
}


// DEVICE TYPE
struct DeviceTypeConversion {
    static gz::util::unordered_string_map<orgb::DeviceType> deviceTypeName2deviceType;
    static orgb::DeviceType getDeviceType(const std::string_view& s) {
        if (deviceTypeName2deviceType.empty()) {
            // if not initalized, initalize map
            for (int i = 0; i < static_cast<int>(orgb::DeviceType::Unknown); i++) {
                deviceTypeName2deviceType.insert({orgb::enumString(static_cast<orgb::DeviceType>(i)), static_cast<orgb::DeviceType>(i)});
            }
        }
        if (deviceTypeName2deviceType.contains(s)) {
            return deviceTypeName2deviceType.find(s)->second;
        }
        else {
            return orgb::DeviceType::Unknown;
        }
    }
};
gz::util::unordered_string_map<orgb::DeviceType> DeviceTypeConversion::deviceTypeName2deviceType;

std::string toString(const orgb::DeviceType& t) {
    return orgb::enumString(t);
}

template<std::same_as<orgb::DeviceType> T>
orgb::DeviceType fromString(const std::string& s) {
    return DeviceTypeConversion::getDeviceType(s);
}


// RGB SETTING
namespace rgb {
    std::string RGBSetting::toString() const {
        std::string s;
        for (auto it = targetDevices.begin(); it != targetDevices.end(); it++) {
            s += ::toString(*it);
            s += ",";
        }
        s.erase(s.size() - 1);
        s += "|";
        s += ::toString(transition) + "|";
        s += ::toString(mode) + "|";
        s += ::toString(color);
    return s;



    }
} // namespace gz


template<> rgb::RGBSetting fromString<rgb::RGBSetting>(const std::string& s) {
    rgb::RGBSetting rgb;
    std::vector<std::string_view> args = gz::util::splitStringInVector<std::string_view>(std::string_view(s), "|");

    if (args.size() != 4) {
        throw gz::InvalidArgument("String has not enough arguments to construct RGBSetting", "fromString<RGBSetting>");
    }

    std::vector<std::string_view> deviceTypes = gz::util::splitStringInVector<std::string_view>(args[0], ",");
    for (auto it = deviceTypes.begin(); it != deviceTypes.end(); it++) {
        rgb.targetDevices.insert(DeviceTypeConversion::getDeviceType(*it));
    }

    rgb.transition = fromString<rgb::RGBTransition>(args[1]);
    rgb.mode = fromString<rgb::RGBMode>(args[2]);
    rgb.color = fromString<orgb::Color>(std::string(args[3]));

    return rgb;
}

// ENUM - STRING CONVERSION BEGIN
// do not write your own code between these comment blocks - it will be overwritten when you run gen_enum_str.py again
//
// RGBMode
//
// Holds maps used by fromString and toString for conversion of RGBMode values
struct EnumStringConversion_RGBMode {
    static gz::util::unordered_string_map<rgb::RGBMode> name2type;
    static std::map<rgb::RGBMode, std::string> type2name;
};  // generated by gen_enum_str

gz::util::unordered_string_map<rgb::RGBMode> EnumStringConversion_RGBMode::name2type {
	{ "RAINBOW", rgb::RGBMode::RAINBOW },
	{ "STATIC", rgb::RGBMode::STATIC },
	{ "CLEAR", rgb::RGBMode::CLEAR },
};  // generated by gen_enum_str

std::map<rgb::RGBMode, std::string> EnumStringConversion_RGBMode::type2name {
	{ rgb::RGBMode::RAINBOW, "RAINBOW" },
	{ rgb::RGBMode::STATIC, "STATIC" },
	{ rgb::RGBMode::CLEAR, "CLEAR" },
};  // generated by gen_enum_str

std::string toString(const rgb::RGBMode& v) {
	if (EnumStringConversion_RGBMode::type2name.contains(v)) {
		return EnumStringConversion_RGBMode::type2name.at(v);
	}
	else {
		throw gz::InvalidArgument("InvalidArgument: '" + std::to_string(static_cast<int>(v)) + "'", "toString(RGBMode)");
	}
}  // generated by gen_enum_str

template<> rgb::RGBMode fromString<rgb::RGBMode>(const std::string_view& sv) {
	if (EnumStringConversion_RGBMode::name2type.contains(sv)) {
		return EnumStringConversion_RGBMode::name2type.find(sv)->second;
	}
	else {
		throw gz::InvalidArgument("InvalidArgument: '" + std::string(sv) + "'", "fromString<RGBMode>");
	}
}  // generated by gen_enum_str

template<> rgb::RGBMode fromString<rgb::RGBMode>(const std::string& s) {
	return fromString<rgb::RGBMode>(std::string_view(s));
}  // generated by gen_enum_str

//
// RGBTransition
//
// Holds maps used by fromString and toString for conversion of RGBTransition values
struct EnumStringConversion_RGBTransition {
    static gz::util::unordered_string_map<rgb::RGBTransition> name2type;
    static std::map<rgb::RGBTransition, std::string> type2name;
};  // generated by gen_enum_str

gz::util::unordered_string_map<rgb::RGBTransition> EnumStringConversion_RGBTransition::name2type {
	{ "FADE", rgb::RGBTransition::FADE },
	{ "INSTANT", rgb::RGBTransition::INSTANT },
};  // generated by gen_enum_str

std::map<rgb::RGBTransition, std::string> EnumStringConversion_RGBTransition::type2name {
	{ rgb::RGBTransition::FADE, "FADE" },
	{ rgb::RGBTransition::INSTANT, "INSTANT" },
};  // generated by gen_enum_str

std::string toString(const rgb::RGBTransition& v) {
	if (EnumStringConversion_RGBTransition::type2name.contains(v)) {
		return EnumStringConversion_RGBTransition::type2name.at(v);
	}
	else {
		throw gz::InvalidArgument("InvalidArgument: '" + std::to_string(static_cast<int>(v)) + "'", "toString(RGBTransition)");
	}
}  // generated by gen_enum_str

template<> rgb::RGBTransition fromString<rgb::RGBTransition>(const std::string_view& sv) {
	if (EnumStringConversion_RGBTransition::name2type.contains(sv)) {
		return EnumStringConversion_RGBTransition::name2type.find(sv)->second;
	}
	else {
		throw gz::InvalidArgument("InvalidArgument: '" + std::string(sv) + "'", "fromString<RGBTransition>");
	}
}  // generated by gen_enum_str

template<> rgb::RGBTransition fromString<rgb::RGBTransition>(const std::string& s) {
	return fromString<rgb::RGBTransition>(std::string_view(s));
}  // generated by gen_enum_str

//
// RGBCommandType
//
// Holds maps used by fromString and toString for conversion of RGBCommandType values
struct EnumStringConversion_RGBCommandType {
    static gz::util::unordered_string_map<rgb::RGBCommandType> name2type;
    static std::map<rgb::RGBCommandType, std::string> type2name;
};  // generated by gen_enum_str

gz::util::unordered_string_map<rgb::RGBCommandType> EnumStringConversion_RGBCommandType::name2type {
	{ "CHANGE_SETTING", rgb::RGBCommandType::CHANGE_SETTING },
	{ "RESUME_FROM_HIBERNATE", rgb::RGBCommandType::RESUME_FROM_HIBERNATE },
	{ "SLEEP", rgb::RGBCommandType::SLEEP },
	{ "QUIT", rgb::RGBCommandType::QUIT },
};  // generated by gen_enum_str

std::map<rgb::RGBCommandType, std::string> EnumStringConversion_RGBCommandType::type2name {
	{ rgb::RGBCommandType::CHANGE_SETTING, "CHANGE_SETTING" },
	{ rgb::RGBCommandType::RESUME_FROM_HIBERNATE, "RESUME_FROM_HIBERNATE" },
	{ rgb::RGBCommandType::SLEEP, "SLEEP" },
	{ rgb::RGBCommandType::QUIT, "QUIT" },
};  // generated by gen_enum_str

std::string toString(const rgb::RGBCommandType& v) {
	if (EnumStringConversion_RGBCommandType::type2name.contains(v)) {
		return EnumStringConversion_RGBCommandType::type2name.at(v);
	}
	else {
		throw gz::InvalidArgument("InvalidArgument: '" + std::to_string(static_cast<int>(v)) + "'", "toString(RGBCommandType)");
	}
}  // generated by gen_enum_str

template<> rgb::RGBCommandType fromString<rgb::RGBCommandType>(const std::string_view& sv) {
	if (EnumStringConversion_RGBCommandType::name2type.contains(sv)) {
		return EnumStringConversion_RGBCommandType::name2type.find(sv)->second;
	}
	else {
		throw gz::InvalidArgument("InvalidArgument: '" + std::string(sv) + "'", "fromString<RGBCommandType>");
	}
}  // generated by gen_enum_str

template<> rgb::RGBCommandType fromString<rgb::RGBCommandType>(const std::string& s) {
	return fromString<rgb::RGBCommandType>(std::string_view(s));
}  // generated by gen_enum_str


// ENUM - STRING CONVERSION END