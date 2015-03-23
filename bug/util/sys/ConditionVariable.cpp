#include "ConditionVariable.h"

#include <cerrno>
#include <cassert>

namespace csd { namespace util { namespace sys {

ConditionVariable::ConditionVariable(
	const unsigned int noOfSignals,
	pthread_cond_t * const conds
) :	m_noOfSignals(noOfSignals),
	m_shared(conds),
	m_condsOff(conds
		? reinterpret_cast<char*>(conds) - reinterpret_cast<char*>(this)
		: reinterpret_cast<char*>(new pthread_cond_t[noOfSignals]) -
		  reinterpret_cast<char*>(this)) {
	if (this->m_shared) {
		pthread_condattr_t condAttr;
		int status = pthread_condattr_init(&condAttr);
		assert(!status);
		status = pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
		assert(!status);
		for (unsigned int i = 0; i < noOfSignals; ++i) {
			status = pthread_cond_init(this->getConds() + i, &condAttr);
			assert(!status);
		}
		status = pthread_condattr_destroy(&condAttr);
		assert(!status);
		pthread_mutexattr_t mutexAttr;
		status = pthread_mutexattr_init(&mutexAttr);
		assert(!status);
		status = pthread_mutexattr_setpshared(&mutexAttr,
			PTHREAD_PROCESS_SHARED);
		assert(!status);
		status = pthread_mutex_init(&this->m_mutex, &mutexAttr);
		assert(!status);
		status = pthread_mutexattr_destroy(&mutexAttr);
		assert(!status);
	} else {
		int status;
		for (unsigned int i = 0; i < noOfSignals; ++i) {
			status = pthread_cond_init(this->getConds() + i, 0);
			assert(!status);
		}
		status = pthread_mutex_init(&this->m_mutex, 0);
		assert(!status);
	}
}

ConditionVariable::~ConditionVariable() {
	int status = pthread_mutex_destroy(&this->m_mutex);
	assert(!status);
	for (unsigned int i = 0; i < this->m_noOfSignals; ++i) {
		status = pthread_cond_destroy(this->getConds() + i);
		assert(!status);
	}
	if (!this->m_shared)
		delete[] this->getConds();
}

void ConditionVariable::lock() {
	const int status = pthread_mutex_lock(&this->m_mutex);
	assert(!status);
}

void ConditionVariable::unlock() {
	const int status = pthread_mutex_unlock(&this->m_mutex);
	assert(!status);
}

void ConditionVariable::wait(const unsigned int signalNo) {
	assert(signalNo < this->m_noOfSignals);
	const int status =
		pthread_cond_wait(this->getConds() + signalNo, &this->m_mutex);
	assert(!status);
}

bool ConditionVariable::timedWait(
	const timespec * const absTime,
	const unsigned int signalNo
) {
	assert(signalNo < this->m_noOfSignals);
	const int status = pthread_cond_timedwait(this->getConds() + signalNo,
		&this->m_mutex, absTime);
	if (!status)
		return false;
	else {
		assert(status == ETIMEDOUT);
		return true;
	}
}

bool ConditionVariable::timedWait(
	const unsigned long long nanoSeconds,
	const unsigned int signalNo
) {
	assert(signalNo < this->m_noOfSignals);
	const time_t now = time(0);
	const timespec timeOut = {
		static_cast<time_t>(now + nanoSeconds / 1000000000),
		static_cast<long>(nanoSeconds % 1000000000)
	};
	return this->timedWait(&timeOut, signalNo);
}

void ConditionVariable::signal(const unsigned int signalNo) {
	assert(signalNo < this->m_noOfSignals);
	const int status = pthread_cond_signal(this->getConds() + signalNo);
	assert(!status);
}

void ConditionVariable::broadcast(const unsigned int signalNo) {
	assert(signalNo < this->m_noOfSignals);
	const int status =
		pthread_cond_broadcast(this->getConds() + signalNo);
	assert(!status);
}

inline pthread_cond_t *ConditionVariable::getConds() throw() {
	return reinterpret_cast<pthread_cond_t*>(reinterpret_cast<char*>(this) +
		this->m_condsOff);
}

} } }
