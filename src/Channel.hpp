#ifndef  Channel_INC
#define  Channel_INC

#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>

template<class T>
struct Channel {

  std::list<T> lst;
  mutable std::mutex m;
  std::condition_variable cv;
  bool closed=false;
  int maxSize = 0;

  Channel(size_t maxSize = 0){
      this->maxSize = maxSize;
  }

  //deleted due to condition_variable having it deleted
  Channel operator=(Channel &) = delete;

  bool put(const T &i) {
    std::unique_lock<std::mutex> lock(m);
    if( maxSize ){
        cv.wait(lock, [&](){ return closed || lst.size() < maxSize; });
    }
    if(closed){
        return false;
    }


    lst.push_back(i);
    cv.notify_one();

    return true;
  }

  template <typename... Args>
  bool emplace( Args&&  ... args ){
    std::unique_lock<std::mutex> lock(m);
    if( maxSize ){
        cv.wait(lock, [&](){ return closed || lst.size() < maxSize; });
    }

    if(closed){
      return false;
    }

    lst.emplace_back( std::forward<Args>( args )... );
    cv.notify_one();
    return true;
  }

  bool get(T &out, bool wait = true) {
    std::unique_lock<std::mutex> lock(m);

    if(wait){
      cv.wait(lock, [&](){ return closed || !lst.empty(); });
    }
    if(lst.empty()) {
        cv.notify_all();
        return false; 
    }

    out = std::move(lst.front());

    lst.pop_front();
    cv.notify_all();
    return true;
  }

  size_t size() const{
    std::unique_lock<std::mutex> lock(m);
    return lst.size();
  }

  void close() {
    std::unique_lock<std::mutex> lock(m);
    closed = true;
    cv.notify_all();
  }

  bool is_closed() const {
    std::unique_lock<std::mutex> lock(m);
    return closed;
  }

};
#endif   /* ----- #ifndef Channel_INC  ----- */
