#ifndef MUTEX_H_
#define MUTEX_H_

#include "../ConfigUtil.h"

#include <boost/noncopyable.hpp>

#include <pthread.h>

namespace csd { namespace util { namespace sys {

/*!
 * A POSIX-style mutex.
 */
class Mutex: private boost::noncopyable {
	
public:
	
	/*!
	 * \brief Constructor.
	 * 
	 * \param shared		Whether the mutex can be shared between processes
	 * 						via shared memory.
	 */
	explicit Mutex(bool shared = false);
	
	/*!
	 * \brief Destructor.
	 */
	~Mutex();
	
	/*!
	 * \brief Lock the mutex.
	 * \sa unlock()
	 */
	void lock();
	
	/*!
	 * \brief Unlock the mutex.
	 * \sa lock()
	 */
	void unlock();
	
private:
	
	pthread_mutex_t m_mutex;
	
};

} } }

#endif /* MUTEX_H_ */
