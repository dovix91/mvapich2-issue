#include "NetworkManagerMPIBlockingMultithread.h"

#include <iostream>

using namespace std;

#define QUEUE_TIMEOUT 100L

namespace fbx {
namespace net {
namespace mpi {

NetworkManagerMPIBlockingMultithread::NetworkManagerMPIBlockingMultithread()
  : m_stopFlag(false),
    m_senders(0),
    m_receivers(0),
    m_sendQueue(0) {
}

bool NetworkManagerMPIBlockingMultithread::initialize(int* argc, char*** argv, int numSenders, int numReceivers) {
  int provided;
  MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    cout << "SharedMPIBlockingMultithread in use." << endl;
  }

  // Version
  int len;
  char mpi_lib_ver[MPI_MAX_LIBRARY_VERSION_STRING];
  MPI_Get_library_version(mpi_lib_ver, &len);
  printf("MPI library version: %s\n", mpi_lib_ver);

  m_stopFlag = false;

  m_senders.resize(numSenders);
  m_receivers.resize(numReceivers);
  for(int i = 0; i < numSenders; i++) {
    pthread_create(&m_senders[i], NULL, NetworkManagerMPIBlockingMultithread::senderLoopStatic, this);
  }
  for(int i = 0; i < numReceivers; i++) {
    pthread_create(&m_receivers[i], NULL, NetworkManagerMPIBlockingMultithread::receiverLoopStatic, this);
  }

  return (provided == MPI_THREAD_MULTIPLE);
}

NodeID NetworkManagerMPIBlockingMultithread::getId(string machine) {
  return stoi(machine);
}

void NetworkManagerMPIBlockingMultithread::registerCallback(ChannelID channelID,
    void* self, RecvCallback callback) {
  m_recvCallbackMapLock.lock();
  m_recvCallbackMap[channelID] = std::make_pair(self, callback);
  m_recvCallbackMapLock.unlock();
}

void NetworkManagerMPIBlockingMultithread::deregisterCallback(ChannelID channelID) {
  m_recvCallbackMapLock.lock();
  m_recvCallbackMap.erase(channelID);
  m_recvCallbackMapLock.unlock();
}

bool NetworkManagerMPIBlockingMultithread::send(Buffer buffer, BufferSize bufSize,
    std::vector<NodeID>& recvNodeIDs, ChannelID channelID, bool cleanupBuffer) {
  SendRequest* request = new SendRequest(buffer, bufSize, recvNodeIDs, channelID, cleanupBuffer);
  return !m_sendQueue.put(request);
}

bool NetworkManagerMPIBlockingMultithread::recv(ChannelID requiredChannelID, Buffer* buffer,
    BufferSize* buffSize, NodeID* sendNodeID, ChannelID* recvChannelID) {
  int res, flag;
  MPI_Message message;
  MPI_Status status;

  // Nonblocking Probe
  res = MPI_Improbe(MPI_ANY_SOURCE, requiredChannelID, MPI_COMM_WORLD, &flag, &message,
      &status);
  if (res != MPI_SUCCESS) {
    cerr << "Error in MPI_Improbe." << endl;
    return false;
  }
  if (!flag) {
    // cerr << "Probe produced no result." << endl;
    return false;
  }

  // Allocate buffer
  res = MPI_Get_count(&status, MPI_BYTE, buffSize);
  if (res != MPI_SUCCESS) {
    cerr << "Error in MPI_Get_count." << endl;
    return false;
  }
  *buffer = (void*) malloc(*buffSize);

  // Blocking Receive
  res = MPI_Mrecv(*buffer, *buffSize, MPI_BYTE, &message, &status);
  if (res != MPI_SUCCESS) {
    cerr << "Error in MPI_Mrecv." << endl;
    return false;
  }
  *sendNodeID = status.MPI_SOURCE;
  *recvChannelID = status.MPI_TAG;

  return true;
}

NetworkManagerMPIBlockingMultithread::~NetworkManagerMPIBlockingMultithread() {
  m_stopFlag = true;

  for (int i = 0; i < m_senders.size(); i++) {
    pthread_join(m_senders[i], NULL);
  }

  for (int i = 0; i < m_receivers.size(); i++) {
    pthread_join(m_receivers[i], NULL);
  }

  m_recvCallbackMap.clear();

  MPI_Finalize();
}

void* NetworkManagerMPIBlockingMultithread::senderLoopStatic(void* cookie) {
  reinterpret_cast<NetworkManagerMPIBlockingMultithread *>(cookie)->senderLoop();
  return NULL;
}

void* NetworkManagerMPIBlockingMultithread::receiverLoopStatic(void* cookie) {
  reinterpret_cast<NetworkManagerMPIBlockingMultithread *>(cookie)->receiverLoop();
  return NULL;
}

void NetworkManagerMPIBlockingMultithread::senderLoop() {
  while (!m_stopFlag) {
    SendRequest* request;
    if (!m_sendQueue.get(request, QUEUE_TIMEOUT)) {
      printf("MPI send: batch number %d, seq number %d\n",
             *(uint64_t*)(request->m_buffer + request->m_bufferSize - 8), 
             *(uint64_t*)(request->m_buffer + request->m_bufferSize - 16));

      for (int i = 0; i < request->m_recvNodeIDs.size(); i++) {
        int res = MPI_Send(request->m_buffer, request->m_bufferSize, MPI_BYTE,
                           request->m_recvNodeIDs[i], request->m_channelID, MPI_COMM_WORLD);
        if (res != MPI_SUCCESS) {
          cerr << "Error in MPI_Send." << endl;
        }
      }

      printf("MPI send FIN: batch number %d, seq number %d\n",
             *(uint64_t*)(request->m_buffer + request->m_bufferSize - 8),
             *(uint64_t*)(request->m_buffer + request->m_bufferSize - 16));

      // TODO(adovis): (maybe) post-send callback here
      if (request->m_cleanupBuffer)
        free(request->m_buffer);

      free(request);
    }
  }
}

void NetworkManagerMPIBlockingMultithread::receiverLoop() {
  Buffer buffer;
  BufferSize bufSize;
  NodeID sendNodeID;
  ChannelID channelID;

  while (!m_stopFlag) {
    if(this->recv(MPI_ANY_TAG, &buffer, &bufSize, &sendNodeID, &channelID) &&
        m_recvCallbackMap.find(channelID) != m_recvCallbackMap.end()) {
      printf("MPI recv: batch number %d, seq number %d\n",
             *(uint64_t*)(buffer + bufSize - 8),  
             *(uint64_t*)(buffer + bufSize - 16));

      m_recvCallbackMap[channelID].second(m_recvCallbackMap[channelID].first,
                                          buffer, bufSize, sendNodeID, channelID);
    }
  }
}

}  // namespace mpi
}  // namespace net
}  // namespace fbx
