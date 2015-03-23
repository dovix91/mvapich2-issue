#ifndef FBXNET_NETWORKMANAGER_H_
#define FBXNET_NETWORKMANAGER_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace fbx {
namespace net {

typedef void* Buffer;
typedef int32_t BufferSize;
typedef int32_t NodeID;
typedef int32_t ChannelID;

typedef void (*RecvCallback) (
      void* self,
      Buffer buffer,
      BufferSize bufSize,
      NodeID sendNodeID,
      ChannelID channelID);

/*
 * This class defines the interfaces to the inter-stub network.
 */
class NetworkManager {
public:
  /*
   * Initialize the underlying network framework and workers.
   */
  virtual bool initialize(int* argc, char*** argv, int numSenders, int numReceivers) = 0;

  /*
   * Register static function to be called, when something is received from channel channelID.
   * The 'self' object is passed, in order to call a member function in the static function.
   */
  virtual void registerCallback(ChannelID channelID, void* self, RecvCallback callback) = 0;

  /*
   * Deregistration of the callback associated with a channel.
   */
  virtual void deregisterCallback(ChannelID channelID) = 0;

  /*
   * Given a string representation of a mahcine, it returns its NodeID.
   */
  virtual NodeID getId(std::string machine) = 0;

  /*
   * Send a buffer to the specified channel channelID on the specified machines
   * recvNodeIDs (multicast).
   * It returns true in case of success, false otherwise.
   * The ownership of the buffer is given by the caller to the library, which is
   * responsible of freeing it, if cleanupBuffer is true; it is not freed by the library
   * otherwise.
   * TODO(adovis): (maybe) add pre/post-send callbacks.
   */
  virtual bool send(Buffer buffer, BufferSize bufSize, std::vector<NodeID>& recvNodeIDs,
      ChannelID channelID, bool cleanupBuffer = true) = 0;

  /*
   * Receive a buffer from channel requiredChannelID.
   * It returns true in case of success, false otherwise.
   * In case of success, the buffer is returned in the parameter, together
   * with the buffer size buffSize, the machine of the sender sendNodeID and
   * the ID 'recvChannelID' of the channel we received the buffer from (useful when the
   * requiredChannelID is 'any').
   * The ownership of the buffer is passed to the caller.
   */
  virtual bool recv(ChannelID requiredChannelID, Buffer* buffer, BufferSize* buffSize,
      NodeID* sendNodeID, ChannelID* recvChannelID) = 0;

  virtual ~NetworkManager() {}
};

}  // namespace net
}  // namespace fbx

#endif /* FBXNET_NETWORKMANAGER_H_ */
