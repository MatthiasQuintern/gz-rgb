#pragma once

#ifdef GZ_UTIL_STRING_CONCEPTS
    static_assert(false, "gz-util/string/conversion.hpp must not be included before rgb_command.hpp!");
#endif

#include "OpenRGB/Client.hpp"
#include "OpenRGB/DeviceInfo.hpp"

#include <string>
#include <set>
#include <type_traits>

namespace rgb {
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
        public:
        std::string toString() const;
    };

    enum RGBCommandType {
        CHANGE_SETTING, RESUME_FROM_HIBERNATE, SLEEP, QUIT
    };
    struct RGBCommand {
        RGBCommandType type;
        RGBSetting setting;
    };
} // namespace rgb


// COLOR
/**
 * @brief Turn color to string -> #rrggbb
 */
std::string toString(const orgb::Color& c);

/**
 * @brief Create color from string
 * @details
 *  Calls orgb::Color::fromString()
 */
template<std::same_as<orgb::Color> T>
orgb::Color fromString(const std::string& s);


// DEVICE TYPE
std::string toString(const orgb::DeviceType& t);

template<std::same_as<orgb::DeviceType> T>
orgb::DeviceType fromString(const std::string& s);


// RGB SETTINGS
template<std::same_as<rgb::RGBSetting> T>
rgb::RGBSetting fromString(const std::string& s);

/**
 * @todo move to global namespace
 */
namespace gz {
inline std::string toString(const rgb::RGBSetting& s) {
    return s.toString();
}
}



// ENUM - STRING CONVERSION BEGIN
// do not write your own code between these comment blocks - it will be overwritten when you run gen_enum_str.py again
//
// RGBMode
//
/**
 * @brief Convert @ref rgb::RGBMode "an enumeration value" to std::string
 * @details
 *  This function was generated by gen_enum_str.py\n
 *  Throws gz::InvalidArgument if v is invalid.
 * @throws gz::InvalidArgument if v is invalid.
 */
std::string toString(const rgb::RGBMode& v);
template<std::same_as<rgb::RGBMode> T>
rgb::RGBMode fromString(const std::string& s);
template<std::same_as<rgb::RGBMode> T>
rgb::RGBMode fromString(const std::string_view& sv);
/**
 * @brief Convert a std::string to @ref rgb::RGBMode "an enumeration value"
 * @details
 *  This function was generated by gen_enum_str.py\n
 *  Throws gz::InvalidArgument if s is invalid.
 * @throws gz::InvalidArgument if s is invalid.
 * @param v one of: RAINBOW, STATIC, CLEAR,
 */
template<> rgb::RGBMode fromString<rgb::RGBMode>(const std::string& s);
/// @brief Convert a std::string_view to @ref {self.get_name()} "an enumeration value"
template<> rgb::RGBMode fromString<rgb::RGBMode>(const std::string_view& sv);

//
// RGBTransition
//
/**
 * @brief Convert @ref rgb::RGBTransition "an enumeration value" to std::string
 * @details
 *  This function was generated by gen_enum_str.py\n
 *  Throws gz::InvalidArgument if v is invalid.
 * @throws gz::InvalidArgument if v is invalid.
 */
std::string toString(const rgb::RGBTransition& v);
template<std::same_as<rgb::RGBTransition> T>
rgb::RGBTransition fromString(const std::string& s);
template<std::same_as<rgb::RGBTransition> T>
rgb::RGBTransition fromString(const std::string_view& sv);
/**
 * @brief Convert a std::string to @ref rgb::RGBTransition "an enumeration value"
 * @details
 *  This function was generated by gen_enum_str.py\n
 *  Throws gz::InvalidArgument if s is invalid.
 * @throws gz::InvalidArgument if s is invalid.
 * @param v one of: FADE, INSTANT,
 */
template<> rgb::RGBTransition fromString<rgb::RGBTransition>(const std::string& s);
/// @brief Convert a std::string_view to @ref {self.get_name()} "an enumeration value"
template<> rgb::RGBTransition fromString<rgb::RGBTransition>(const std::string_view& sv);

//
// RGBCommandType
//
/**
 * @brief Convert @ref rgb::RGBCommandType "an enumeration value" to std::string
 * @details
 *  This function was generated by gen_enum_str.py\n
 *  Throws gz::InvalidArgument if v is invalid.
 * @throws gz::InvalidArgument if v is invalid.
 */
std::string toString(const rgb::RGBCommandType& v);
template<std::same_as<rgb::RGBCommandType> T>
rgb::RGBCommandType fromString(const std::string& s);
template<std::same_as<rgb::RGBCommandType> T>
rgb::RGBCommandType fromString(const std::string_view& sv);
/**
 * @brief Convert a std::string to @ref rgb::RGBCommandType "an enumeration value"
 * @details
 *  This function was generated by gen_enum_str.py\n
 *  Throws gz::InvalidArgument if s is invalid.
 * @throws gz::InvalidArgument if s is invalid.
 * @param v one of: CHANGE_SETTING, RESUME_FROM_HIBERNATE, SLEEP, QUIT,
 */
template<> rgb::RGBCommandType fromString<rgb::RGBCommandType>(const std::string& s);
/// @brief Convert a std::string_view to @ref {self.get_name()} "an enumeration value"
template<> rgb::RGBCommandType fromString<rgb::RGBCommandType>(const std::string_view& sv);


// ENUM - STRING CONVERSION END
