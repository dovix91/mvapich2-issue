#ifndef CONFIGUTIL_H_
#define CONFIGUTIL_H_

#include <cstddef>

#define KB *1024
#define MB *(1024 KB)
#define GB *(1024ull MB)

namespace csd { namespace util {

namespace sys {

/*!
 * Disables CPU Mapping and uses system's cpu mapping
 */
// #define CPU_MAPPING_OFF

/*!
 * Uses a spread CPU Mapping. Consecutive csdcpus belong to different numa 
 * regions
 */
//#define CPU_MAPPING_SPREAD

/*!
 * Uses a packed CPU Mapping. Consecutive csdcpus belong to the same numa 
 * regions
 */
#define CPU_MAPPING_PACKED

/*!
 * \brief The maximum amount of memory we allow a numa_alloc_*() call to
 * request.
 * 
 * Picking a value that is higher than the amount of RAM at a single NUMA node
 * may lead to kernel lock-ups at runtime.
 */
static const size_t MAX_NUMA_MALLOC = 8192ull MB;

}

namespace queue {

/*!
 * \brief Size of shared-memory buffers in bytes.
 * 
 * Shared-memory buffers are the basis of shared-memory queues.
 */
static const size_t SHM_BUFFER_SIZE = 16 MB;

}

} }

#endif /*CONFIGUTIL_H_*/
