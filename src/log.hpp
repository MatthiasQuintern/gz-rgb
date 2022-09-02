#pragma once

#define LOG_MULTITHREAD

#include <iostream>
#include <string>
#include <concepts>
#include <ranges>
#include <unordered_map>

#ifdef LOG_MULTITHREAD 
#include <mutex>
#endif

namespace gz {
    inline const char* boolToString(bool b) {
        return b ? "true" : "false";
    }

    const int logLength = 100;

    constexpr unsigned int TIMESTAMP_CHAR_COUNT = 22;
    constexpr unsigned int POSTPREFIX_CHAR_COUNT = 2;

    //
    // CONCEPTS
    //
    /// is std::string or convertible to std::string
    template<typename T> 
    concept Stringy = std::same_as<T, std::string> || std::convertible_to<T, std::string_view>;

    /// has .to_string() member
    template<typename T>
    concept HasToString = !Stringy<T> && requires(T t) { { t.to_string() }-> Stringy; };
    
    /// works with std::to_string(), except bool
    template<typename T>
    concept WorksToString = !std::same_as<T, bool> && !Stringy<T> && !HasToString<T> && requires(T t) { { std::to_string(t) } -> Stringy; };

    /// string-like, has .to_string() member, works with std::to_string() or bool
    template<typename T>
    concept PrintableNoPtr = Stringy<T> || HasToString<T> || WorksToString<T> || std::same_as<T, bool>;

    template<typename T>
    concept Printable = PrintableNoPtr<T> || requires(T t) { { *(t.get()) } ->  PrintableNoPtr; };

    /// Type having printable .x and .y members
    template<typename T>
    concept Vector2Printable = !Printable<T> && 
        requires(T t) { { t.x } -> Printable; { t.y } -> Printable; };

    /// Pair having printable elements
    template<typename T>
    concept PairPrintable = !Vector2Printable<T> && !Printable<T> && 
        requires(T p) { { p.first } -> Printable; } &&  (requires(T p){ { p.second } -> Printable; } ||  requires(T p){ { p.second } -> Vector2Printable; }); 

    /// Container having printable elements
    template<typename T>
    concept ContainerPrintable = !Printable<T> && !Vector2Printable<T> &&  !PairPrintable<T> &&
        std::ranges::forward_range<T> && (Printable<std::ranges::range_reference_t<T>> || Vector2Printable<std::ranges::range_reference_t<T>>); 

    /// Container having printable pairs
    template<typename T>
    concept MapPrintable = !Printable<T> && !Vector2Printable<T> && !ContainerPrintable<T> &&
        std::ranges::forward_range<T> && PairPrintable<std::ranges::range_reference_t<T>>;

    template<typename T>
    concept LogableNotPointer = Printable<T> || Vector2Printable<T> || PairPrintable<T> || ContainerPrintable<T> || MapPrintable<T>;

    template<typename T> 
    concept LogableSmartPointer = requires(T t) { { *(t.get()) } ->  LogableNotPointer; };


    //
    // COLORS
    //
    /// Colors to be used in Log::clog
    enum Color {
        RESET, BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, BBLACK, BRED, BGREEN, BYELLOW, BBLUE, BMAGENTA, BCYAN, BWHITE
    };
    extern const char* COLORS[];


    //
    // LOG
    //
    /**
     * @brief Define types that can be logged with Log
     * @details
     *  As of now you can log type T with instance t:
     *  -# Any string-like type
     *  -# Any type that works with std::to_string()
     *  -# Any type that has a to_string() member that returns a string
     *  -# Any type with t.x and t.y, provided t.x and t.y satisfy one of 1-3
     *  -# Any type with t.first, t.second provided t.first satisfies one of 1-3 and t.second satisfies 1-4
     *  -# Any type that has a forward_iterator which references any one of 1-5
     *
     *  1-6 include for example:
     *  - int, float, bool...
     *  - std::vector<std::string>, std::list<unsigned int>
     *  - std::map<A, vec2<float>> if A.to_string() returns a string
     *  - ...
     */
    template<typename T>
    concept Logable = LogableNotPointer<T> || LogableSmartPointer<T>;

    template<typename T>
    class vec2;  // defined in gz_math.hpp

/**
 * @brief Manages printing messages to stdout and to logfiles.
 * @details
 *  @subsection log_threads Thread safety
 *   Log can use a static mutex for thread safety. To use this feature, you have to #define LOG_MULTITHREAD at the top of log.hpp.
 *   Note that log uses the default std::cout buffer, so you should make sure it is not being used while logging something.
 */
class Log {
    public:
        /**
         * @brief Creates a log object, which can print messages to stdout and/or write them to a log file
         * @details By creating multiple instances with different parameters, logs can be easily turned on/off for different usages.
         *
         * @param logfile: Name of the file in the logs folder
         * @param showLog: Wether to print the messages to stdout
         * @param storeLog: Wether to save the messages to the logfile
         * @param prefix: A prefix that comes between the timestamp and the message. ": " is automatically appended to the prefix
         * @param prefixColor: The color of the prefix
         *
         * @note Colors will only be shown when written to stdout, not in the logfile.
         */
        Log(std::string logfile="log.log", bool showLog=true, bool storeLog=true, std::string&& prefix="", Color prefixColor=RESET);

        ~Log();

        /**
         * @brief Logs a message
         * @details Depending on the settings of the log instance, the message will be printed to stdout and/or written to the logfile.
         *  The current date and time is placed before the message.
         *  The message will look like this:
         *  <time>: <prefix>: <message>
         *  where time will be white, prefix in prefixColor and message white.
         * @param args Any number of arguments that satisfy concept Logable
         *
         */
        template<Logable... Args>
        void log(Args&&... args) {
#ifdef LOG_MULTITHREAD 
            mtx.lock();
#endif
            logArray[iter] = getTime();
            logArray[iter] += prefix;
            vlog(" ", std::forward<Args>(args)...);
            logArray[iter] += "\n";

            std::cout << std::string_view(logArray[iter].c_str(), TIMESTAMP_CHAR_COUNT - 1) <<
                COLORS[prefixColor] << prefix << COLORS[RESET] << 
                std::string_view(logArray[iter].begin() + prefixLength, logArray[iter].end());
            if (++iter >= logLength) {
                iter = 0;
                writeLog();
            }
#ifdef LOG_MULTITHREAD 
            mtx.unlock();
#endif
        }

        /**
         * @brief Logs a message. Overload for convenience, same behavior as log()
         */
        template<Logable... Args>
        void operator() (Args&&... args) {
            log(std::forward<Args>(args)...);
        }

        /**
         * @brief Log an error
         * @details Prints the message with a red "Error: " prefix.
         *  The message will look like this:
         *  <time>: <prefix>: Error: <message>
         *  where time will be white, prefix in prefixColor, Error in red and message white.
         * @param args Any number of arguments that satisfy concept Logable
         */
        template<Logable... Args>
        void error(Args&&... args) {
            clog(RED, "Error", WHITE, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a warnign
         * @details Prints the message with a yellow "Warning: " prefix.
         *  The message will look like this:
         *  <time>: <prefix>: Warning: <message>
         *  where time will be white, prefix in prefixColor, Warning in yellow and message white.
         * @param args Any number of arguments that satisfy concept Logable
         */
        template<Logable... Args>
        void warning(Args&&... args) {
            clog(YELLOW, "Warning", WHITE, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a message in a certain color and with a colored type
         * @details
         *  The message will look like this:
         *  <time>: <prefix>: <type>: <message>
         *  where time will be white, prefix in prefixColor, type in typeColor and message in messageColor
         * @param args Any number of arguments that satisfy concept Logable
         */
        template<Logable... Args>
        void clog(Color typeColor, std::string&& type, Color messageColor, Args&&... args) {
#ifdef LOG_MULTITHREAD 
            mtx.lock();
#endif
            logArray[iter] = getTime();
            logArray[iter] += prefix + ": " + type + ": ";
            vlog(" ", std::forward<Args>(args)...);
            logArray[iter] += "\n";

            std::cout << std::string_view(logArray[iter].c_str(), TIMESTAMP_CHAR_COUNT - 1) <<
                COLORS[prefixColor] << prefix << COLORS[typeColor] << 
                std::string_view(logArray[iter].begin() + prefixLength + POSTPREFIX_CHAR_COUNT, logArray[iter].begin() + prefixLength + type.size() + 2 * POSTPREFIX_CHAR_COUNT) <<
                COLORS[messageColor] << std::string_view(logArray[iter].begin() + prefixLength + type.size() + 2 * POSTPREFIX_CHAR_COUNT, logArray[iter].end()) << COLORS[RESET];
            if (++iter >= logLength) {
                iter = 0;
                writeLog();
            }
#ifdef LOG_MULTITHREAD 
            mtx.unlock();
#endif
        }

    private:
        template<Stringy T, Logable... Args>
        void vlog(const char* appendChars, T&& t,  Args&&... args) {
            logArray[iter] += t;
            logArray[iter] += appendChars;
            vlog(" ", std::forward< Args>(args)...);
        }

        template<HasToString T, Logable... Args>
        void vlog(const char* appendChars, T&& t,  Args&&... args) {
            logArray[iter] += t.to_string();
            logArray[iter] += appendChars;
            vlog(" ", std::forward< Args>(args)...);
        }

        template<WorksToString T, Logable... Args>
        void vlog(const char* appendChars, T&& t,  Args&&... args) {
            logArray[iter] += std::to_string(t);
            logArray[iter] += appendChars;
            vlog(" ", std::forward< Args>(args)...);
        }

        template<typename T, Logable... Args>
            requires(std::same_as<T, bool>)
        void vlog(const char* appendChars, T&& b,  Args&&... args) {
            logArray[iter] += boolToString(b);
            logArray[iter] += appendChars;
            vlog(" ", std::forward< Args>(args)...);
        }


        template<Vector2Printable V,  Logable... Args>
        void vlog(const char* appendChars, V&& v,  Args&&... args) {
            logArray[iter] += "(";
            vlog("", v.x);
            logArray[iter] += ", ";
            vlog("", v.y);
            logArray[iter] += ")";
            logArray[iter] += appendChars;
            vlog(" ", std::forward< Args>(args)...);
        }

        template<PairPrintable P,  Logable... Args>
        void vlog(const char* appendChars, P&& p,  Args&&... args) {
            logArray[iter] += "(";
            vlog("", p.first);
            logArray[iter] += ", ";
            vlog("" ,p.second);
            logArray[iter] += ")";
            logArray[iter] += appendChars;
            vlog(" ", std::forward< Args>(args)...);
        }

        template<ContainerPrintable T, Logable... Args>
        void vlog(const char* appendChars, T&& t,  Args&&... args) {;
            logArray[iter] += "[";
            for (auto it = t.begin(); it != t.end(); it++) {
                vlog(", ", *it); 
            }
            logArray[iter] += "]";
            logArray[iter] += appendChars;
            vlog(" ", std::forward< Args>(args)...);
        }

        template<MapPrintable T, Logable... Args>
        void vlog(const char* appendChars, T&& t, Args&&... args) {
            logArray[iter] += "{";
            for (const auto& [k, v] : t) {
                vlog(": ", k);
                vlog(", ", v);
            }
            logArray[iter] += "}";
            logArray[iter] += appendChars;
            vlog(" ", std::forward<Args>(args)...);
        }


        /// Log any logable element that is stored in a pointer
        template<LogableSmartPointer T, Logable... Args>
        void vlog(const char* appendChars, T&& t, Args&&... args) {
            vlog("", *t);
            vlog(" ", std::forward<Args>(args)...);
        }

        void vlog(const char* appendChars) {};

    private:
        std::array<std::string, logLength> logArray;
        size_t iter;
        std::string logFile;
        bool showLog;
        bool storeLog;
        void writeLog();

        Color prefixColor;
        std::string prefix;
        std::string::size_type prefixLength;

        char time[TIMESTAMP_CHAR_COUNT];
        char* getTime();
#ifdef LOG_MULTITHREAD 
        static std::mutex mtx;
#endif
};

extern Log genLog;
extern Log gameLog;
extern Log engLog;
}

/**
 * @file
 * @brief A logger capable of logging lots of different types and containers to stdout and/or a logfile.
 */
