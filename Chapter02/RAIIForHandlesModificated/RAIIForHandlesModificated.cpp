#include <iostream>
#include <Windows.h>

struct Handle {
    explicit Handle(HANDLE h = nullptr) : _h(h) {}

    ~Handle() noexcept { Close(); }

    // delete copy-ctor and copy-assignment
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;

    // allow move (transfer ownership)
    Handle(Handle&& other) noexcept : _h(other._h) {
        other._h = nullptr;
        // *this = std::move(other);
    }
    Handle& operator=(Handle&& other) noexcept {
        if (this != &other) {
            Close();
            _h = other._h;
            other._h = nullptr;
        }
        return *this;
    }

    operator bool() const {
        return _h != nullptr && _h != INVALID_HANDLE_VALUE;
    }

    HANDLE operator()() const {
        return _h;
    }

    void Close() {
        if (_h) {
            ::CloseHandle(_h);
            _h = nullptr;
        }
    }

private:
    HANDLE _h;
};


int main()
{
    Handle hMyEvent(::CreateEvent(nullptr, TRUE, FALSE, nullptr));
    if (!hMyEvent) {
        // handle failure
        return 1;
    }
    ::SetEvent(hMyEvent());

    // move ownership
    Handle hOtherEvent(std::move(hMyEvent));
    ::ResetEvent(hOtherEvent());

    return 0;
}