#ifndef CONDITIONVARIABLE_H_
#define CONDITIONVARIABLE_H_

#include "../ConfigUtil.h"

#include <boost/noncopyable.hpp>

#include <cstdlib>

#include <pthread.h>

namespace csd { namespace util { namespace sys {

/*!
 * A POSIX-style condition variable and associated mutex.
 */
class ConditionVariable: private boost::noncopyable {
	
public:
	
	/*!
	 * \brief Constructor.
	 * 
	 * \param noOfSignals	The number of different signals.
	 * \param conds			Optional pointer to pre-allocated space for
	 * 						condition variables. Need one POSIX condition
	 * 						variable per signal. If pointer is suppied, the
	 * 						condition variable can be shared between processes
	 * 						via shared memory.
	 */
	explicit ConditionVariable(
		unsigned int noOfSignals = 1,
		pthread_cond_t *conds = 0
	);
	
	/*!
	 * \brief Destructor.
	 */
	~ConditionVariable();
	
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
	
	/*!
	 * \brief Wait until a signal is received.
	 * 
	 * \param signalNo				The number of the signal to wait for.
	 * 
	 * \sa timedWait()
	 */
	void wait(
		unsigned int signalNo = 0
	);
	 
	/*!
	 * \brief Wait until a signal is received or a timeout occurs.
	 * 
	 * The mutex must be locked when calling this method.
	 * 
	 * \param absTime				Timeout as an absolute point in time.
	 * \param signalNo				The number of the signal to wait for.
	 * \return						true iff a timeout has occurred.
	 * 
	 * \sa wait(), lock(), and unlock()
	 */
	bool timedWait(
		const timespec *absTime, 
		unsigned int signalNo = 0 
	);
	
	/*!
	 * \brief Wait until a signal is received or a timeout occurs.
	 * 
	 * The mutex must be locked when calling this method.
	 * 
	 * \param nanoSeconds			Relative timeout in nanoseconds.
	 * \param signalNo				The number of the signal to wait for.
	 * \return						true iff a timeout has occurred.
	 * 
	 * \sa wait(), lock(), and unlock()
	 */
	bool timedWait(
		unsigned long long nanoSeconds,
		unsigned int signalNo = 0
	);
	
	/*
	 * \brief Send a signal.
	 * 
	 * Wakes at most one thread waiting on the given signal.
	 * 
	 * \param signalNo				The number of the signal to send.
	 * 
	 * \sa broadcast()
	 */
	void signal(
		unsigned int signalNo = 0
	);
	
	/*
	 * \brief Broadcast a signal.
	 * 
	 * Wakes all threads waiting on the given signal.
	 * 
	 * \param signalNo				The number of the signal to send.
	 * 
	 * \sa signal()
	 */
	void broadcast(
		unsigned int signalNo = 0
	);
	
private:
	
	inline pthread_cond_t *getConds() throw();
	
	const unsigned int m_noOfSignals;
	const bool m_shared;
	const ssize_t m_condsOff;
	pthread_mutex_t m_mutex;
	
};

} } }

#endif /* CONDITIONVARIABLE_H_ */
