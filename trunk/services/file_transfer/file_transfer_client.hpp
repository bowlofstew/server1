#ifndef FILE_TRANSFER_CLIENT_HPP_
#define FILE_TRANSFER_CLIENT_HPP_
#include "base/base.hpp"
#include "base/hash.hpp"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "thread/threadpool.hpp"
#include "server/protobuf_connection.hpp"
#include "services/file_transfer/checkbook.hpp"
#include <boost/iostreams/device/mapped_file.hpp>
class TransferTask;
class SliceStatus;
class FileTransferClient {
 public:
  void PushChannel(FullDualChannel *channel);
  void Start();
  void Stop();
  void set_threadpool(ThreadPool *pool) {
    out_threadpool_ = pool;
  }
  void set_finish_listener(const boost::function0<void> h) {
    finish_handler_ = h;
  }
  bool finished() const {
    return status_ == FINISHED;
  }
  const string &host() const {
    return host_;
  }
  const string &port() const {
    return port_;
  }
  const string &src_filename() const {
    return src_filename_;
  }
  const string &dest_filename() const {
    return dest_filename_;
  }
  void set_timeout(int timeout) {
    timeout_ = timeout;
  }
  static FileTransferClient *Create(
      const string &host, const string &port,
      const string &src_filename,
      const string &dest_filename,
      int threadpool_size);
  // The percent * 1000, 1000 means transfer finished.
  int Percent();
 private:
  static const int kDefaultTimeOutMs = 5000;
  ThreadPool *GetThreadPool() {
    if (out_threadpool_ == NULL) {
      return &pool_;
    }
    return out_threadpool_;
  }
  enum Status {
    SYNC_CHECKBOOK = 0,
    PREPARE_SLICE,
    SYNC_SLICE,
    FINISHED,
  };
  FileTransferClient(const string &host, const string &port,
                     const string &src_filename, const string &dest_filename,
                     int thread_pool_size) :
    host_(host), port_(port), src_filename_(src_filename),
    dest_filename_(dest_filename), pool_("FileTransferClientThreadPool", thread_pool_size),
    sync_checkbook_failed_(0), finished_(false), status_(SYNC_CHECKBOOK), out_threadpool_(NULL),
    timeout_(kDefaultTimeOutMs) {
  }
  void Schedule();
  void SyncCheckBook();
  void PrepareSlice();
  void ScheduleSlice();
  void SyncSlice(boost::shared_ptr<SliceStatus> slice,
                 boost::shared_ptr<TransferTask> tasker);
  void SyncSliceDone(
      boost::shared_ptr<TransferTask> tasker,
      bool succeed, boost::shared_ptr<SliceStatus> status);
  void ChannelClosed(boost::shared_ptr<TransferTask> tasker,
                     boost::shared_ptr<SliceStatus> status);
  static const int kSyncCheckBookRetry = 3;
  typedef deque<boost::shared_ptr<TransferTask> > TransferTaskQueue;
  typedef list<boost::shared_ptr<SliceStatus> > SliceStatusLink;
  ThreadPool *out_threadpool_;
  ThreadPool pool_;
  boost::function0<void> finish_handler_;
  PCQueue<boost::shared_ptr<TransferTask> > transfer_task_queue_;
  PCQueue<boost::shared_ptr<SliceStatus> > slice_status_queue_;
  boost::mutex transfer_task_set_mutex_;
  hash_set<boost::shared_ptr<TransferTask> > transfer_task_set_;
  scoped_ptr<CheckBook> checkbook_;
  boost::iostreams::mapped_file_source src_file_;
  boost::mutex sync_checkbook_mutex_;
  boost::mutex prepare_slice_mutex_;
  boost::mutex transfering_slice_mutex_;
  SliceStatusLink transfering_slice_;
  Status status_;
  int sync_checkbook_failed_;
  bool finished_;
  string host_, port_, src_filename_, dest_filename_;
  scoped_ptr<boost::asio::io_service> io_service_;
  scoped_ptr<boost::asio::io_service::work> work_;
  int timeout_;
  friend class TransferTask;
};
#endif  // FILE_TRANSFER_CLIENT_HPP_
