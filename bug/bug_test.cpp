/*
 *  Mostly inspired by: osu-micro-benchmarks-4.4.1/mpi/pt2pt/osu_bw.c
 *
 *  Run using: /opt/openmpi-1.8.4/bin/mpirun -host ... --mca btl ... \
 *                ./multithread_bandwidth_test <version> <thread_num> <net_senders>
 *
 *  sharedMPI_version:
 *    0 = SharedMPIBlockingMultithread
 *    1 = SharedMPIBlockingSynchronized
 *
 *  If it deadlocks on local machine, try using:
 *    --mca btl openib,self OR --mca btl tcp,self
 *  depending on your local implementation (the deadlock is in btl sm
 *  [shared memory]).
 */

#include <iostream>
#include <cstdlib>
#include <memory>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <vector>
#include <pthread.h>
#include <unistd.h>

#include <mpi.h>

#include "net/mpi/NetworkManagerMPIBlockingMultithread.h"

#define BUF_SIZE 2351136
#define BATCH_NUM 10000
#define SEQUENCE_NUM 6

using fbx::net::NetworkManager;
using fbx::net::mpi::NetworkManagerMPIBlockingMultithread;

using namespace std;

vector<fbx::net::NodeID> zero {0};
vector<fbx::net::NodeID> one {1};

void pinThread(uint32_t coreId) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(coreId, &cpuset);

  pthread_t current_thread = pthread_self();
  pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

int myid, numprocs;
char **s_buf;
int total_thread_num;
NetworkManager* mpi;
pthread_barrier_t start_barrier, end_barrier;

class Callback {
public:
  static void handleRecvStatic(
      void* self,
      fbx::net::Buffer buffer,
      fbx::net::BufferSize bufSize,
      fbx::net::NodeID sendNodeID,
      fbx::net::ChannelID channelID) {
    free(buffer);
  }
};

static void* benchmark_body(void *arg) {
  int mythread = (int64_t) arg;

  int i, j;

  pinThread(myid * total_thread_num + mythread + 2);

  if (myid == 0) {
    for (i = 1; i < BATCH_NUM; i++) {
      for (j = 0; j < SEQUENCE_NUM; j++) {
        int bufSize = BUF_SIZE;
        void* buffer = malloc(BUF_SIZE);
        *(uint64_t*)(buffer + bufSize - 8) = i;
        *(uint64_t*)(buffer + bufSize - 16) = j;

        mpi->send(buffer, bufSize, one, mythread, 1);
      }

      if (i % 1000 == 0) sleep(1);
    }

    return 0;
  }

  else if (myid == 1) {
    mpi->registerCallback(mythread, NULL, Callback::handleRecvStatic);

    while (true) ;

    return 0;
  } else {
    return 0;
  }
}

int main(int argc, char** argv) {
  switch (atoi(argv[1])) {
  case 0:
    mpi = new NetworkManagerMPIBlockingMultithread();
    break;
  default:
    cout << "Unknown library version." << endl;
    return 1;
  }

  total_thread_num = atoi(argv[2]);
  int netWorkers = atoi(argv[3]);

  if (!mpi->initialize(&argc, &argv, netWorkers, netWorkers)) {
    cerr << "Cannot initialize MPI with correct level of thread support"
        << endl;
    return 1;
  }

  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  pinThread(myid);

  pthread_t* thread_ids = (pthread_t*) malloc(
      total_thread_num * sizeof(pthread_t));

  // Bug test
  // Run the benchmark body and wait all to complete
  for (int i = 0; i < total_thread_num; i++)
    pthread_create(&thread_ids[i], NULL, &benchmark_body, (void*) i);

  // Join threads
  for (int i = 0; i < total_thread_num; i++)
    pthread_join(thread_ids[i], NULL);


  free(thread_ids);

  MPI_Finalize();

  return 0;
}
