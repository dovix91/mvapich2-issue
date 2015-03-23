#ifndef FBXNET_MPI_NETWORKMANAGERMPIBLOCKINGMULTITHREAD_H_
#define FBXNET_MPI_NETWORKMANAGERMPIBLOCKINGMULTITHREAD_H_

#define OMPI_SKIP_MPICXX
#define MPICH_SKIP_MPICXX

#include "../NetworkManager.h"

#include <boost/tr1/unordered_map.hpp>
#include <vector>

#include <mpi.h>

#include "../../util/sys/Mutex.h"
#include "../../util/queue/SyncedQueue.h"

namespace fbx {
namespace net {
namespace mpi {

struct SendRequest {
  SendRequest(Buffer buffer, BufferSize bufSize, std::vector<NodeID> recvNodeIDs,
      ChannelID channelID, bool cleanupBuffer) :
      m_buffer(buffer),
      m_bufferSize(bufSize),
      m_channelID(channelID),
      m_cleanupBuffer(cleanupBuffer) {
    m_recvNodeIDs = recvNodeIDs;
  }

  Buffer m_buffer;
  BufferSize m_bufferSize;
  std::vector<NodeID> m_recvNodeIDs;
  ChannelID m_channelID;
  bool m_cleanupBuffer;
};

class NetworkManagerMPIBlockingMultithread: public NetworkManager {
public:
  NetworkManagerMPIBlockingMultithread();
  bool initialize(int* argc, char*** argv, int numSenders, int numReceivers) override;
  NodeID getId(std::string machine);

  void registerCallback(ChannelID channelID, void* self, RecvCallback callback) override;
  void deregisterCallback(ChannelID channelID) override;

  /*
   * Uses blocking send.
   */
  bool send(Buffer buffer, BufferSize bufSize, std::vector<NodeID>& recvNodeIDs,
      ChannelID channelID, bool cleanupBuffer = true) override;

  /*
   * Uses non-blocking probe and blocking recv.
   */
  bool recv(ChannelID channelID, Buffer* buffer, BufferSize* buffSize,
      NodeID* sendNodeID, ChannelID* recvChannelID) override;

  ~NetworkManagerMPIBlockingMultithread();

private:
  static void* senderLoopStatic(void* cookie);
  static void* receiverLoopStatic(void* cookie);
  void senderLoop();
  void receiverLoop();

  volatile bool m_stopFlag;
  std::vector<pthread_t> m_senders;
  std::vector<pthread_t> m_receivers;

  typedef std::tr1::unordered_map<ChannelID, std::pair<void*, RecvCallback> > RecvCallbackMap;
  RecvCallbackMap m_recvCallbackMap;
  csd::util::sys::Mutex m_recvCallbackMapLock;

  // The incoming send queue (unbound).
  typedef typename csd::util::queue::SyncedQueue<
    SendRequest*, csd::util::queue::ZeroSize> SendQueue;
  SendQueue m_sendQueue;
};

}  // namespace mpi
}  // namespace net
}  // namespace fbx

#endif /* FBXNET_MPI_NETWORKMANAGERMPIBLOCKINGMULTITHREAD_H_ */
