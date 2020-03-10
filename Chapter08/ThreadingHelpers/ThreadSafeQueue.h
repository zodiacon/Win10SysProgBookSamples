#pragma once

#include "CriticalSection.h"
#include "AutoCriticalSection.h"
#include <queue>

template<typename T>
class ThreadSafeQueue {
public:
	ThreadSafeQueue(const ThreadSafeQueue&) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

	void Push(const T& value);
	T Pop();		// wait if necessary
	bool IsEmpty() const;
	bool TryPop(T& value);

private:
	std::queue<T> _queue;
	CriticalSection _lock;
	CONDITION_VARIABLE _cv;
};

template<typename T>
inline void ThreadSafeQueue<T>::Push(const T& value) {
	{
		AutoCriticalSection locker(_lock);
		_queue.push(value);
	}
	::WakeConditionVariable(&_cv);
}

template<typename T>
inline T ThreadSafeQueue<T>::Pop() {
	AutoCriticalSection locker(_lock);
	while (_queue.empty())
		::SleepConditionVariableCS(&_cv, &_lock, INFINITE);
	auto value = _queue.front();
	_queue.pop();
	return value;
}

template<typename T>
inline bool ThreadSafeQueue<T>::IsEmpty() const {
	return _queue.empty();
}

template<typename T>
inline bool ThreadSafeQueue<T>::TryPop(T& value) {
	AutoCriticalSection locker(_lock);
	if (!_queue.empty()) {
		value = _queue.front();
		_queue.pop();
		return true;
	}
	return false;
}
