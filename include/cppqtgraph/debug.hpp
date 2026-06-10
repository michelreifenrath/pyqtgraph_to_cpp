// Source note: translated/adapted from PyQtGraph pyqtgraph/debug.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#pragma once

#include "cppqtgraph/exceptionHandling.hpp"

#include <chrono>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <vector>

namespace cppqtgraph::debug {

[[nodiscard]] inline std::string backtrace(int skip = 0)
{
    std::ostringstream stream;
    stream << "native backtrace unavailable in portable P2.07 helper";
    if (skip > 0) {
        stream << " (skip=" << skip << ')';
    }
    return stream.str();
}

[[nodiscard]] inline std::string formatException(const std::exception& exception, int skip = 0)
{
    std::ostringstream stream;
    stream << typeid(exception).name() << ": " << exception.what() << '\n';
    stream << "  --- exception caught here ---\n";
    stream << backtrace(skip + 1);
    return stream.str();
}

[[nodiscard]] inline std::string formatException(std::exception_ptr exception, int skip = 0)
{
    if (!exception) {
        return "<no current exception>\n" + backtrace(skip + 1);
    }
    try {
        std::rethrow_exception(exception);
    } catch (const std::exception& caught) {
        return formatException(caught, skip + 1);
    } catch (...) {
        return std::string{"unknown non-std exception\n"} + backtrace(skip + 1);
    }
}

inline void printException(const std::exception& exception, std::ostream& stream = std::cerr)
{
    stream << formatException(exception, 1) << '\n';
}

inline void printExc(std::string_view message, std::exception_ptr exception = std::current_exception(), std::ostream& stream = std::cerr)
{
    if (!message.empty()) {
        stream << message << '\n';
    }
    stream << formatException(exception, 1) << '\n';
}

// C++ callable equivalent of debug.warnOnException: returns true if the callable
// completed, false if an exception was caught and reported.
template <typename Fn>
bool warnOnException(Fn&& fn, std::string_view message = {}, std::ostream& stream = std::cerr)
{
    try {
        fn();
        return true;
    } catch (...) {
        printExc(message, std::current_exception(), stream);
        return false;
    }
}

class Profiler final {
public:
    explicit Profiler(std::string name = {}, bool enabled = false, std::ostream* stream = nullptr)
        : name_(std::move(name))
        , enabled_(enabled)
        , stream_(stream)
        , start_(std::chrono::steady_clock::now())
    {
        if (enabled_ && stream_ != nullptr) {
            *stream_ << "Profiler " << name_ << " start\n";
        }
    }

    ~Profiler()
    {
        if (enabled_ && stream_ != nullptr) {
            *stream_ << "Profiler " << name_ << " done " << elapsedSeconds() << "s\n";
        }
    }

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] double elapsedSeconds() const noexcept
    {
        using Seconds = std::chrono::duration<double>;
        return std::chrono::duration_cast<Seconds>(std::chrono::steady_clock::now() - start_).count();
    }
    void mark(std::string_view label)
    {
        if (enabled_ && stream_ != nullptr) {
            *stream_ << "Profiler " << name_ << ' ' << label << ' ' << elapsedSeconds() << "s\n";
        }
    }

private:
    std::string name_;
    bool enabled_{false};
    std::ostream* stream_{nullptr};
    std::chrono::steady_clock::time_point start_;
};

// Python interpreter/GC/sys tracing helpers from debug.py that are intentionally
// not native C++ behavior in this port slice.
[[nodiscard]] inline const std::vector<std::string>& unsupportedPythonHelpers()
{
    static const std::vector<std::string> helpers{
        "Tracer",
        "listObjs",
        "findRefPath",
        "allObj",
        "GarbageWatcher",
        "ObjTracker",
        "ThreadTrace",
        "ThreadColor",
        "enableFaulthandler",
    };
    return helpers;
}

} // namespace cppqtgraph::debug
