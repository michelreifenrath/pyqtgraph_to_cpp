// Source note: translated/adapted from PyQtGraph pyqtgraph/exceptionHandling.py
// PyQtGraph ref: pyqtgraph-0.14.0
// Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
// License: MIT; see THIRD_PARTY_NOTICES.md

#pragma once

#include <algorithm>
#include <cstddef>
#include <exception>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <typeinfo>
#include <utility>
#include <vector>

namespace cppqtgraph::exceptionHandling {

struct ExceptionArgs final {
    std::exception_ptr exception;
    std::string typeName;
    std::string message;
    std::thread::id thread{};
};

struct NotificationResult final {
    std::size_t callbackCount{0};
    std::size_t oldCallbackCount{0};
    std::size_t callbackFailureCount{0};
};

using Callback = std::function<void(const ExceptionArgs&)>;
using OldCallback = std::function<void(std::exception_ptr, std::string_view, std::string_view)>;
using CallbackId = std::size_t;
using OldCallbackId = std::size_t;

namespace detail {
template <typename Fn>
struct CallbackRecord final {
    std::size_t id;
    Fn fn;
};

inline std::vector<CallbackRecord<Callback>>& callbacks()
{
    static std::vector<CallbackRecord<Callback>> records;
    return records;
}

inline std::vector<CallbackRecord<OldCallback>>& oldCallbacks()
{
    static std::vector<CallbackRecord<OldCallback>> records;
    return records;
}

inline std::size_t& nextCallbackId()
{
    static std::size_t next = 1;
    return next;
}

inline bool& clearTracebacks()
{
    static bool clear = false;
    return clear;
}
} // namespace detail

inline CallbackId registerCallback(Callback callback)
{
    const auto id = detail::nextCallbackId()++;
    detail::callbacks().push_back({id, std::move(callback)});
    return id;
}

inline void unregisterCallback(CallbackId id)
{
    auto& target = detail::callbacks();
    target.erase(std::remove_if(target.begin(), target.end(), [id](const auto& record) { return record.id == id; }), target.end());
}

// C++ spelling for exceptionHandling.py's deprecated register(fn).  The exact
// Python name is a C++ keyword, so the native API keeps behavior with a suffix.
inline OldCallbackId registerCallbackDeprecated(OldCallback callback)
{
    const auto id = detail::nextCallbackId()++;
    detail::oldCallbacks().push_back({id, std::move(callback)});
    return id;
}

inline void unregisterDeprecated(OldCallbackId id)
{
    auto& target = detail::oldCallbacks();
    target.erase(std::remove_if(target.begin(), target.end(), [id](const auto& record) { return record.id == id; }), target.end());
}

inline void clearCallbacks()
{
    detail::callbacks().clear();
    detail::oldCallbacks().clear();
}

inline void setTracebackClearing(bool clear = true)
{
    detail::clearTracebacks() = clear;
}

[[nodiscard]] inline bool tracebackClearing() noexcept
{
    return detail::clearTracebacks();
}

[[nodiscard]] inline ExceptionArgs makeExceptionArgs(std::exception_ptr exception, std::thread::id thread = {})
{
    ExceptionArgs args;
    args.exception = exception;
    args.thread = thread;
    if (!exception) {
        args.typeName = "<none>";
        return args;
    }
    try {
        std::rethrow_exception(exception);
    } catch (const std::exception& caught) {
        args.typeName = typeid(caught).name();
        args.message = caught.what();
    } catch (...) {
        args.typeName = "<non-std-exception>";
        args.message = "unknown non-std exception";
    }
    return args;
}

inline NotificationResult notifyUnhandledException(std::exception_ptr exception, std::thread::id thread = {})
{
    const ExceptionArgs args = makeExceptionArgs(exception, thread);
    NotificationResult result;
    for (const auto& record : detail::callbacks()) {
        ++result.callbackCount;
        try {
            record.fn(args);
        } catch (...) {
            ++result.callbackFailureCount;
        }
    }
    for (const auto& record : detail::oldCallbacks()) {
        ++result.oldCallbackCount;
        try {
            record.fn(args.exception, args.typeName, args.message);
        } catch (...) {
            ++result.callbackFailureCount;
        }
    }
    return result;
}

class ExceptionHandler final {
public:
    void remove() noexcept { active_ = false; }
    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] NotificationResult handle(std::exception_ptr exception, std::thread::id thread = {}) const
    {
        if (!active_) {
            return {};
        }
        return notifyUnhandledException(exception, thread);
    }
    [[nodiscard]] bool implements(std::string_view interface = {}) const noexcept
    {
        return interface.empty() || interface == "ExceptionHandler";
    }

private:
    bool active_{true};
};

} // namespace cppqtgraph::exceptionHandling
