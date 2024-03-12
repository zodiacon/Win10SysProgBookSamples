#include <iostream>
#include <Windows.h>

struct Handle {
    explicit Handle(HANDLE h = nullptr) : _h(h) {}

    ~Handle() { Close(); }

    // delete copy-ctor and copy-assignment
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;

    // allow move (transfer ownership)
    Handle(Handle&& other) : _h(other._h) {
        other._h = nullptr;
    }
    Handle& operator=(Handle&& other) {
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

    HANDLE Get() const {
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


void main()
{
    Handle hMyEvent(::CreateEvent(nullptr, TRUE, FALSE, nullptr));
    if (!hMyEvent) {
        // handle failure
        return;
    }
    ::SetEvent(hMyEvent.Get());

    // move ownership
    Handle hOtherEvent(std::move(hMyEvent));
    ::ResetEvent(hOtherEvent.Get());

}