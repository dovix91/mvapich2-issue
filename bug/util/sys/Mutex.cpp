#include "Mutex.h"

#include <cerrno>
#include <cassert>

namespace csd { namespace util { namespace sys {

Mutex::Mutex(const bool shared) {
	if (shared) {
		pthread_mutexattr_t attr;
		int status = pthread_mutexattr_init(&attr);
		assert(!status);
		status = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		assert(!status);
		status = pthread_mutex_init(&this->m_mutex, &attr);
		assert(!status);
		status = pthread_mutexattr_destroy(&attr);
		assert(!status);
	} else {
		const int status = pthread_mutex_init(&this->m_mutex, 0);
		assert(!status);
	}
}

Mutex::~Mutex() {
	const int status = pthread_mutex_destroy(&this->m_mutex);
	assert(!status);
}

void Mutex::lock() {
	const int status = pthread_mutex_lock(&this->m_mutex);
	assert(!status);
}

void Mutex::unlock() {
	const int status = pthread_mutex_unlock(&this->m_mutex);
	assert(!status);
}

} } }
