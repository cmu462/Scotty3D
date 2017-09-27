/*
 * HalfedgeMesh.h
 *
 * Written By Keenan Crane for 15-462 Assignment 2.
 */

/**
 * A MutablePriorityQueue is a minimum-priority queue that
 * allows elements to be both inserted and removed from the
 * queue.  Together, one can easily change the priority of
 * an item by removing it, and re-inserting the same item
 * but with a different priority.  A priority queue, for
 * those who don't remember or haven't seen it before, is a
 * data structure that always keeps track of the item with
 * the smallest priority or "score," even as new elements
 * are inserted and removed.  Priority queues are often an
 * essential component of greedy algorithms, where one wants
 * to iteratively operate on the current "best" element.
 *
 * MutablePriorityQueue is templated on the type T of the object
 * being queued.  For this reason, T must define a comparison
 * operator of the form
 *
 *    bool operator<( const T& t1, const T& t2 )
 *
 * which returns true if and only if t1 is considered to have a
 * lower priority than t2.
 *
 * Basic use of a MutablePriorityQueue might look
 * something like this:
 *
 *    // initialize an empty queue
 *    MutablePriorityQueue<myItemType> queue;
 *
 *    // add some items (which we assume have been created
 *    // elsewhere, each of which has its priority stored as
 *    // some kind of internal member variable)
 *    queue.insertItem( item1 );
 *    queue.insertItem( item2 );
 *    queue.insertItem( item3 );
 *
 *    // get the highest priority item currently in the queue
 *    myItemType highestPriorityItem = queue.top();
 *
 *    // remove the highest priority item, automatically
 *    // promoting the next-highest priority item to the top
 *    queue.pop();
 *
 *    myItemType nextHighestPriorityItem = queue.top();
 *
 *    // Etc.
 *
 *    // We can also remove an item, making sure it is no
 *    // longer in the queue (note that this item may already
 *    // have been removed, if it was the 1st or 2nd-highest
 *    // priority item!)
 *    queue.remove( item2 );
 *
 */

#ifndef CMU462_CAMERA_H
#define CMU462_CAMERA_H

namespace CMU462 {

template <class T>
class MutablePriorityQueue {
 public:
  void insert(const T& item) { queue.insert(item); }

  void remove(const T& item) {
    if (queue.find(item) != queue.end()) {
      queue.erase(item);
    }
  }

  const T& top(void) const { return *(queue.begin()); }

  void pop(void) { queue.erase(queue.begin()); }

 protected:
  set<T> queue;
};

}  // namespace CMU462

#endif  // CMU462_CAMERA_H
