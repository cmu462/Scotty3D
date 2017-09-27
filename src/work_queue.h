#ifndef __WORK_QUEUE_H__
#define __WORK_QUEUE_H__

#include <mutex>
#include <vector>

/**
 * Note that this queue has no wait-until-more-work-is-added capability; it's
 * intended for more isolated or batch-processing-like situations.
 */
template <class T>
class WorkQueue {
 private:
  std::vector<T> storage;
  std::mutex lock;

 public:
  WorkQueue() {}

  bool is_empty() {
    lock.lock();
    bool result = storage.empty();
    lock.unlock();
    return result;
  }

  bool try_get_work(T* outPtr) {
    lock.lock();
    if (storage.empty()) {
      lock.unlock();
      return false;
    }
    *outPtr = storage.front();
    storage.erase(storage.begin());
    lock.unlock();
    return true;
  }

  void put_work(const T& item) {
    lock.lock();
    storage.push_back(item);
    lock.unlock();
  }

  void clear() {
    lock.lock();
    storage.clear();
    lock.unlock();
  }
};

#endif  // WORK_QUEUE_H_
