#ifndef SYNCEDQUEUE_H_
#define SYNCEDQUEUE_H_

#include "../ConfigUtil.h"

#include "../sys/ConditionVariable.h"

#include <boost/noncopyable.hpp>
#include <boost/integer_traits.hpp>

#include <queue>

#include <ctime>
#include <cstdlib>
#include <cassert>

#include <stdint.h>
#include <sys/time.h>

namespace csd { namespace util { namespace queue {

/*!
 * \brief Default functor for the size of an element of the synced queue.
 * 
 * Always returns 1. 
 */
struct OneSize {
	
	/*!
	 * \brief Gets the size of an element of the queue.
	 * 
	 * \tparam T	The type of the object (irrelevant).
	 * \return 		Always 1.
	 */
	template <typename T>
	inline size_t operator()(const T &) const throw() { return 1; }
	
};

/*!
 * \brief Another simple functor for the size of an element of the synced
 * queue.
 * 
 * Always returns 0, allowing for an unbound number of elements in the queue. 
 */
struct ZeroSize {
	
	/*!
	 * \brief Gets the size of an element of the queue.
	 * 
	 * \tparam T	The type of the object (irrelevant).
	 * \return 		Always 0.
	 */
	template <typename T>
	inline size_t operator()(const T &) const throw() { return 0; }
	
};

/*!
 * \brief Default functor for the destruction of an element of the synced
 * queue.
 * 
 * Does not do anything. 
 */
struct NullDestruct {
	
	/*!
	 * \brief Destroys an element of the queue.
	 * 
	 * Does not do anything to the object.
	 * 
	 * \tparam T	The type of the object.
	 */
	template <typename T>
	inline void operator()(T &) const throw() {}
	
};

/*!
 * \brief A synchronized multiple-reader, multiple-writer queue.
 * 
 * \tparam T			Buffer element type.
 * \tparam SizeFunc		Functor that returns the size of an element to be
 * 						deducted from the queue's capacity. The default is a
 * 						functor that always returns 1.
 * \tparam DestructFunc	Functor that is called on all elements that have not 
 *						been got when the queue is destroyed. The default is to
 *						do nothing.
 */
template <
	typename T,
	typename SizeFunc = OneSize,
	typename DestructFunc = NullDestruct
> class SyncedQueue: private boost::noncopyable {
	
public:
	
	/*!
	 * \brief The type of the elements stored in the queue.
	 */
	typedef T Element;
	
	/*!
	 * \brief Type of the functor that returns the size of an element.
	 */
	typedef SizeFunc Size;
	
	/*!
	 * \brief Type of the functor that is called to destruct an element.
	 */
	typedef DestructFunc Destruct;
	
	/*!
	 * \brief Infinite timeout constant.
	 * 
	 * \sa	put(T&, unsigned long&, size_t),
	 * 		bulkPutBegin(size_t, size_t&, unsigned long),
	 * 		get(T&, unsigned long), and
	 * 		flush(T*, unsigned int, unsigned int&, unsigned long)
	 */
	static const unsigned long TIMEOUT_NONE =
		boost::integer_traits<unsigned long>::const_max;
	
	/*!
	 * \brief Passing this constant instructs the queue to call the
	 * size functor to determine the size of an element.
	 * 
	 * \sa put()
	 */
	static const size_t SIZE_USE_FUNC =
		boost::integer_traits<size_t>::const_max;
	
	/*!
	 * \brief Constructor.
	 * 
	 * \param capacity			The size of the queue.
	 * \param getSize			The size functor.
	 * \param destruct			The destruct functor.
	 */
	explicit SyncedQueue(
		size_t capacity,
		const SizeFunc &getSize = SizeFunc(),
		const DestructFunc &destruct = DestructFunc()
	);
	
	/*!
	 * \brief Destructor.
	 */ 
	~SyncedQueue();
	
	/*!
	 * \brief Puts a single T into the queue.
	 * 
	 * \param t 				The T to put into the queue.
	 * \param timeoutMillis 	Timeout in milliseconds.
	 * \param size 				Size of the T.
	 * 
	 * \return 					true iff a timeout occurred before the 
	 * 							message could be put into the queue.
	 */
	bool put(
		const T &t,
		unsigned long timeoutMillis = TIMEOUT_NONE,
		size_t size = SIZE_USE_FUNC
	);
	
	/*!
	 * \brief Puts a single T into the queue.
	 * 
	 * \param t 				The T to put into the queue.
	 * \param timeout 			Absolute timeout.
	 * \param size				Size of the T.
	 * 
	 * \return					true iff a timeout occurred before the message
	 * 							could be put into the queue.
	 */
	bool put(
		const T &t,
		const timespec &timeout,
		size_t size = SIZE_USE_FUNC
	);
	
	/*!
	 * \brief Begin bulk put.
	 * 
	 * Attempts to acquire an exclusive lock on a non-full queue.
	 * 
	 * \param spaceRequested	Requested space.
	 * \param[out] spaceFree	Free space.
	 * \param timeoutMillis		Timeout in milliseconds.
	 * 
	 * \return 					true iff a timeout occurred before the queue
	 * 							became non-full.
	 * 
	 * \sa bulkPut() and bulkPutEnd()
	 */ 
	bool bulkPutBegin(
		size_t spaceRequested,
		size_t &spaceFree,
		unsigned long timeoutMillis = TIMEOUT_NONE
	);
	
	/*!
	 * \brief Begin bulk put.
	 * 
	 * Attempts to acquire an exclusive lock on the queue.
	 * 
	 * \param spaceRequested	Requested space.
	 * \param[out] spaceFree	Free space.
	 * \param timeout			Absolute timeout.
	 * 
	 * \return 					True iff a timeout occurred before the queue
	 * 							became non-full.
	 * 
	 * \sa bulkPut() and bulkPutEnd()
	 */ 
	bool bulkPutBegin(
		size_t spaceRequested,
		size_t &spaceFree,
		const timespec &timeout
	);
	
	/*!
	 * \brief Bulk put.
	 * 
	 * Call bulkPutBegin() first, then perform an arbitrary number of calls to
	 * bulkPut(), finally call bulkPutEnd().
	 * 
	 * \param ts				Array of elements to put.
	 * \param noOfTs			Number of elements in the array.
	 * \param[out] noOfTsPut	noOfTsPut < noOfTs indicates queue is full.
	 * \param tSizes			Array of size of each element. Null pointer 
	 * 							means SizeFunc is used.
	 * 
	 * \sa bulkPutBegin() and bulkPutEnd()
	 */ 
	void bulkPut(
		const T *ts,
		unsigned int noOfTs,
		unsigned int &noOfTsPut,
		size_t *tSizes = 0
	);
	
	/*!
	 * \brief End bulk put.
	 * 
	 * Releases the exclusive lock on the queue.
	 * 
	 * \sa bulkPutBegin() and bulkPut()
	 */ 
	void bulkPutEnd();
	
	/*!
	 * \brief Gets a single T from the queue.
	 * 
	 * Blocks until there is something in the queue or a timeout occurs.
	 *  
	 * \param[out] t			The T got from the queue.
	 * \param[in] timeoutMillis	Timeout in milliseconds.
	 * 
	 * \return					true iff a timeout occured before the message
	 * 							could be put into the queue.
	 */
	bool get(
		T &t,
		unsigned long timeoutMillis = TIMEOUT_NONE
	);
	
	/*!
	 * \brief Gets a single T from the queue.
	 * 
	 * Blocks until there is something in the queue or a timeout occurs.
	 *  
	 * \param[out] t			The T got from the queue.
	 * \param[in] timeout		Absolute timeout.
	 * 
	 * \return					true iff a timeout occured before the message
	 * 							could be put into the queue.
	 */
	bool get(
		T &t,
		const timespec &timeout
	);
	
	/*!
	 * \brief Flushes multiple Ts from the queue.
	 * 
	 * Blocks until there is at least one message in the queue or a timeout
	 * occurs.
	 * 
	 * \param[out] ts			Array of Ts.
	 * \param[in] maxNoOfTs		Length of the output array.
	 * \param[out] noOfTsGot	The number of Ts written out.
	 * \param[in] timeoutMillis	Timeout in milliseconds.
	 * 
	 * \return					true iff a timeout occured before a single T
	 * 							could be retrieved from the queue.
	 */
	bool flush(
		T *ts,
		unsigned int maxNoOfTs,
		unsigned int &noOfTsGot,
		unsigned long timeoutMillis = TIMEOUT_NONE
	);
	
	/*!
	 * \brief Flushes multiple Ts from the queue.
	 * 
	 * Blocks until there is at least one message in the queue or a timeout
	 * occurs.
	 * 
	 * \param[out] ts			Array of Ts.
	 * \param[in] maxNoOfTs		Length of the output array.
	 * \param[out] noOfTsGot	The number of Ts written out.
	 * \param[in] timeout		Absolute timeout.
	 * 
	 * \return					true iff a timeout occured before a single T
	 * 							could be retrieved from the queue.
	 */
	bool flush(
		T *ts,
		unsigned int maxNoOfTs,
		unsigned int &noOfTsGot,
		const timespec &timeout
	);
	
	/*!
	 * \brief Returns the capacity of the queue.
	 * 
	 * \return The capacity of the queue.
	 */
	inline size_t getCapacity() const throw();

	/*!
	 * \brief Checks whether the queue is empty (but beware of
	 * asynchronicity!)
	 *
	 * \return					true iff the queue is empty. 
	 */
	inline bool isEmpty() const throw();
	
	/*!
	 * \brief Checks whether the queue is full (but beware of
	 * asynchronicity!)
	 * 
	 * \return					true iff the queue is empty. 
	 */ 
	inline bool isFull() const throw();
	
private:
	
	/*!
	 * \brief The signals that m_cond accepts.
	 */
	enum Signal {
		SYNCEDQUEUE_FILLED, /*!< The queue is full. */
		SYNCEDQUEUE_FLUSHED /*!< The queue has been flushed. */
	};
	
	/*!
	 * \brief Pair of an element and its size.
	 */
	struct ElemSizePair {
		inline ElemSizePair(const T &t, const size_t size)
		: m_t(t), m_size(size) {
		}
		T m_t;
		size_t m_size;
	};
	
	/*!
	 * \brief The container of all the elements of the queue.
	 */
	typedef std::queue<ElemSizePair> TQueue;
	
	/*!
	 * \brief Converts a relative to an absolute timeout.
	 * 
	 * \param[in] relativeMillis	The relative timeout in milliseconds.
	 * \param[out] absolute			The absolute timeout.
	 */
	static inline void absoluteTimeout(
		unsigned long relativeMillis,
		timespec &absolute
	) throw();
	
	const size_t m_capacity;
	size_t m_free;
	const SizeFunc m_getSize;
	const DestructFunc m_destruct;
	mutable sys::ConditionVariable m_cond;
	TQueue m_espQueue;
	
};

template <typename T, typename SizeFunc, typename DestructFunc>
SyncedQueue<T, SizeFunc, DestructFunc>::SyncedQueue(
	const size_t capacity,
	const SizeFunc &getSize,
	const DestructFunc &destruct
) :	m_capacity(capacity),
	m_free(capacity),
	m_getSize(getSize),
	m_destruct(destruct),
	m_cond(2) {
}

template <typename T, typename SizeFunc, typename DestructFunc>
SyncedQueue<T, SizeFunc, DestructFunc>::~SyncedQueue() {
	while (!this->m_espQueue.empty()) {
		ElemSizePair esp(this->m_espQueue.front());
		this->m_destruct(esp.m_t);
		this->m_espQueue.pop();
	}
}

template <typename T, typename SizeFunc, typename DestructFunc>
bool SyncedQueue<T, SizeFunc, DestructFunc>::put(
	const T &t,
	const unsigned long timeoutMillis,
	const size_t size
) {
	
	if (timeoutMillis != TIMEOUT_NONE) {
		
		// relative timeout specified, calculate absolute timeout
		timespec timeout;
		absoluteTimeout(timeoutMillis, timeout);
		
		return this->put(t, timeout, size);
		
	} else {
		
		// no timeout specified
		const timespec * const timeout = 0;
		return this->put(t, *timeout, size);
		
	}
	
}

template <typename T, typename SizeFunc, typename DestructFunc>
bool SyncedQueue<T, SizeFunc, DestructFunc>::put(
	const T &t,
	const timespec &timeout,
	size_t size
) {
	
	// get size of t
	if (size == SIZE_USE_FUNC)
		size = this->m_getSize(t);
	assert(size <= this->m_capacity);
	
	if (&timeout) {
		
		// timeout specified
	
		// wait for space to become free
		this->m_cond.lock();
		while (this->m_free < size) {
			
			// timed wait
			if (this->m_cond.timedWait(&timeout, SYNCEDQUEUE_FLUSHED)) {
				// timed out
				this->m_cond.unlock();
				return true;
			}
			
		}
		
	} else {
		
		// no timeout specified
		
		// wait for space to become free
		this->m_cond.lock();
		while (this->m_free < size)
			this->m_cond.wait(SYNCEDQUEUE_FLUSHED);
		
	}
	
	// space is free, enqueue T
	this->m_espQueue.push(ElemSizePair(t, size));
	this->m_free -= size;
	
	// tell the consumer threads
	this->m_cond.broadcast(SYNCEDQUEUE_FILLED);
	
	this->m_cond.unlock();
	
	return false;
	
}

template <typename T, typename SizeFunc, typename DestructFunc>
bool SyncedQueue<T, SizeFunc, DestructFunc>::bulkPutBegin(
	const size_t spaceRequested,
	size_t &spaceFree,
	const unsigned long timeoutMillis
) {
	
	if (timeoutMillis != TIMEOUT_NONE) {
			
		// relative timeout specified, calculate absolute timeout
		timespec timeout;
		absoluteTimeout(timeoutMillis, timeout);
		
		return this->bulkPutBegin(spaceRequested, spaceFree, timeout);
		
	} else {
		
		// no timeout specified
		const timespec * const timeout = 0;
		return this->bulkPutBegin(spaceRequested, spaceFree, *timeout);
		
	}
	
}

template <typename T, typename SizeFunc, typename DestructFunc>
bool SyncedQueue<T, SizeFunc, DestructFunc>::bulkPutBegin(
	const size_t spaceRequested,
	size_t &spaceFree,
	const timespec &timeout
) {
	
	// sanity check
	assert(spaceRequested <= this->m_capacity);
	
	if (&timeout) {
			
		// timeout specified
	
		// wait for space to become free
		this->m_cond.lock();
		while (this->m_free < spaceRequested) {
			
			// timed wait
			if (this->m_cond.timedWait(&timeout, SYNCEDQUEUE_FLUSHED)) {
				// timed out
				this->m_cond.unlock();
				return true;
			}
			
		}
		
	} else {
		
		// no timeout specified
		
		// wait for space to become free
		this->m_cond.lock();
		while (this->m_free < spaceRequested)
			this->m_cond.wait(SYNCEDQUEUE_FLUSHED);
		
	}
	
	// at this point we hold an exclusive lock and there is enough space left
	// in the queue
	spaceFree = this->m_free;
	assert(spaceFree >= spaceRequested);
	
	return false;
	
}

template <typename T, typename SizeFunc, typename DestructFunc>
void SyncedQueue<T, SizeFunc, DestructFunc>::bulkPut(
	const T * const ts,
	const unsigned int noOfTs,
	unsigned int &noOfTsPut,
	size_t * const tSizes
) {
	
	// sanity check
	if (noOfTs)
		assert(ts);
	
	for (unsigned int i = 0; i < noOfTs; ++i) {
		
		// get size of element i
		const size_t size = !tSizes || tSizes[i] == SIZE_USE_FUNC
			? this->m_getSize(ts[i])	// no size supplied; use function
			: tSizes[i];				// use client-supplied size
		
		if (size <= this->m_free) {
			// space free; put element in the queue
			this->m_espQueue.push(ElemSizePair(ts[i], size));
			this->m_free -= size;
		} else {
			// out of space; fail
			noOfTsPut = i;
			return;
		}
		
	}
	
	// all elements put into the queue; success
	noOfTsPut = noOfTs;
	
}

template <typename T, typename SizeFunc, typename DestructFunc>
void SyncedQueue<T, SizeFunc, DestructFunc>::bulkPutEnd() {
	
	// signal consumers and release lock
	this->m_cond.broadcast(SYNCEDQUEUE_FILLED);
	this->m_cond.unlock();
	
}

template <typename T, typename SizeFunc, typename DestructFunc>
bool SyncedQueue<T, SizeFunc, DestructFunc>::get(
	T &t,
	const unsigned long timeoutMillis
) {
	
	if (timeoutMillis != TIMEOUT_NONE) {
		
		// relative timeout specified, calculate absolute timeout
		timespec timeout;
		absoluteTimeout(timeoutMillis, timeout);
		
		return this->get(t, timeout);
		
	} else {
		
		// no timeout specified
		const timespec * const timeout = 0;
		return this->get(t, *timeout);
		
	}
	
}

template <typename T, typename SizeFunc, typename DestructFunc>
bool SyncedQueue<T, SizeFunc, DestructFunc>::get(
	T &t,
	const timespec &timeout
) {
	
	if (&timeout) {
		
		this->m_cond.lock();
	
		// wait for something in the queue
		while (this->m_espQueue.empty()) {
			
			if (this->m_cond.timedWait(&timeout, SYNCEDQUEUE_FILLED)) {
				// timed out
				this->m_cond.unlock();
				return true;
			}
			
		}
		
	} else {
		
		// no timeout
		this->m_cond.lock();
			
		// wait for something in the queue
		while (this->m_espQueue.empty())
			this->m_cond.wait(SYNCEDQUEUE_FILLED);
		
	}
	
	// something in the queue, get it
	const ElemSizePair &esp = this->m_espQueue.front();
	t = esp.m_t;
	this->m_free += esp.m_size;
	assert(this->m_free <= this->m_capacity);
	this->m_espQueue.pop();
	
	// tell the producer threads
	this->m_cond.broadcast(SYNCEDQUEUE_FLUSHED);
	
	this->m_cond.unlock();
	
	return false;
	
}

template <typename T, typename SizeFunc, typename DestructFunc>
bool SyncedQueue<T, SizeFunc, DestructFunc>::flush(
	T *ts,
	const unsigned int maxNoOfTs,
	unsigned int &noOfTsGot,
	const unsigned long timeoutMillis
) {
	
	if (timeoutMillis != TIMEOUT_NONE) {
		
		// relative timeout specified, calculate absolute timeout
		timespec timeout;
		absoluteTimeout(timeoutMillis, timeout);
		
		return this->flush(ts, maxNoOfTs, noOfTsGot, timeout);
		
	} else {
		
		// no timeout specified
		const timespec * const timeout = 0;
		return this->flush(ts, maxNoOfTs, noOfTsGot, *timeout);
		
	}
	
}

template <typename T, typename SizeFunc, typename DestructFunc>
bool SyncedQueue<T, SizeFunc, DestructFunc>::flush(
	T *ts,
	const unsigned int maxNoOfTs,
	unsigned int &noOfTsGot,
	const timespec &timeout
) {
	
	if (&timeout) {
		
		this->m_cond.lock();
		
		// wait for something in the queue
		while (this->m_espQueue.empty()) {
			
			if (this->m_cond.timedWait(&timeout, SYNCEDQUEUE_FILLED)) {
				// timed out
				this->m_cond.unlock();
				noOfTsGot = 0;
				return true;
			}
			
		}
		
	} else {
		
		// no timeout
		this->m_cond.lock();
			
		// wait for something in the queue
		while (this->m_espQueue.empty())
			this->m_cond.wait(SYNCEDQUEUE_FILLED);
		
	}
	
	// flush the queue
	unsigned int i = 0;
	while (i < maxNoOfTs && !this->m_espQueue.empty()) {
		const ElemSizePair &esp = this->m_espQueue.front(); 
		ts[i] = esp.m_t;
		this->m_free += esp.m_size;
		this->m_espQueue.pop();
		++i;
	}
	noOfTsGot = i;
	assert(this->m_free <= this->m_capacity);
	
	// tell the producer threads
	this->m_cond.broadcast(SYNCEDQUEUE_FLUSHED);
	
	this->m_cond.unlock();
	
	return false;
	
}

template <typename T, typename SizeFunc, typename DestructFunc>
inline void SyncedQueue<T, SizeFunc, DestructFunc>::absoluteTimeout(
	const unsigned long relativeMillis,
	timespec &absolute
) throw() {
	
	if (relativeMillis < 1000) {
		
		timeval now;
		gettimeofday(&now, NULL);
		const uint64_t nsec = ((relativeMillis * 1000) + now.tv_usec) * 1000;  
		absolute.tv_nsec = nsec % 1000000000;
		absolute.tv_sec = (nsec / 1000000000) + now.tv_sec;
		
	} else {
		
		const time_t now = time(0);
		absolute.tv_sec = now + relativeMillis / 1000 + 
			(((relativeMillis % 1000) < 500) ? 1 : 0);
		absolute.tv_nsec = (relativeMillis % 1000) * 1000000;
		
	}
}

template <typename T, typename SizeFunc, typename DestructFunc>
inline size_t SyncedQueue<T, SizeFunc, DestructFunc>::getCapacity()
const throw() {
	return this->m_capacity;
}

template <typename T, typename SizeFunc, typename DestructFunc>
inline bool SyncedQueue<T, SizeFunc, DestructFunc>::isEmpty() const throw() {
	this->m_cond.lock();
	const bool e = this->m_espQueue.empty();
	this->m_cond.unlock();
	return e;
}

template <typename T, typename SizeFunc, typename DestructFunc>
inline bool SyncedQueue<T, SizeFunc, DestructFunc>::isFull() const throw() {
	this->m_cond.lock();
	const bool f = !this->m_free;
	this->m_cond.unlock();
	return f;
}

} } }

#endif /* SYNCEDQUEUE_H_ */
