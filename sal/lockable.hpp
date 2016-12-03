#pragma once

/**
 * \file sal/lockable.hpp
 * Locked pointer idiom implementation.
 */

#include <sal/config.hpp>
#include <mutex>


__sal_begin


/**
 * Scoped locking idiom extension to hold pointer to data during locked scope.
 * Once unlocked, data pointer is reset also. It is undefined behaviour to
 * store pointer to external variable and continue using is after this
 * locked_ptr is unlocked.
 *
 * Usage:
 * \code
 * using lockable_int = sal::lockable_t<int>;
 *
 * int data;
 * lockable_int lockable_data{data};
 * if (auto data_lock = lockable_data.lock())
 * {
 *   *data_lock = 1;
 * }
 * \endcode
 */
template <typename T, typename Mutex>
class locked_ptr
{
public:

  /**
   * Associated data type
   */
  using value_t = T;

  /**
   * Synchronisation device type
   */
  using mutex_t = Mutex;


  locked_ptr (const locked_ptr &) = delete;
  locked_ptr &operator= (const locked_ptr &) = delete;


  /**
   * Construct locked_ptr that is not associated to any data or mutex.
   */
  locked_ptr () noexcept = default;


  /**
   * Construct new locked_ptr bound to \a data and using \a mutex as
   * synchronisation device. Lock is acquired immediately.
   */
  explicit locked_ptr (value_t *data, mutex_t &mutex)
    : mutex_(&mutex)
    , data_(data)
  {
    mutex_->lock();
  }


  /**
   * Construct new locked_ptr associated with \a data and using \a mutex as
   * synchronisation device. Lock is tried to be acquired immediately. On
   * failure, \a data is not associated to \a this.
   */
  locked_ptr (value_t *data, mutex_t &mutex, std::try_to_lock_t)
    : mutex_(&mutex)
    , data_(data)
  {
    if (!mutex.try_lock())
    {
      mutex_ = nullptr;
      data_ = nullptr;
    }
  }


  /**
   * Construct new locked_ptr bound to \a data and using \a mutex as
   * synchronisation device. \a mutex is already assumed to be acquired.
   */
  locked_ptr (value_t *data, mutex_t &mutex, std::adopt_lock_t) noexcept
    : mutex_(&mutex)
    , data_(data)
  {}


  /**
   * Construct new locked_ptr associated to \a data but is not actually
   * locked. It can be used pass locked_ptr with associated data around but
   * not actually synchronising access to it.
   */
  locked_ptr (value_t *data, mutex_t &, std::defer_lock_t) noexcept
    : mutex_(nullptr)
    , data_(data)
  {}


  /**
   * Construct new locked_ptr from \a that. Source variable \a that is set to
   * undefined state. It can be set to defined state again by moving another
   * locked_ptr to it.
   */
  locked_ptr (locked_ptr &&that) noexcept
    : mutex_(that.mutex_)
    , data_(that.data_)
  {
    that.mutex_ = nullptr;
    that.data_ = nullptr;
  }


  /**
   * Acquire associated data pointer and mutex from \a that. If before move
   * \a this was locked, it will be unlocked.
   */
  locked_ptr &operator= (locked_ptr &&that)
  {
    if (mutex_)
    {
      mutex_->unlock();
    }
    mutex_ = that.mutex_;
    that.mutex_ = nullptr;
    data_ = that.data_;
    that.data_ = nullptr;
    return *this;
  }


  /**
   * Release mutex.
   */
  ~locked_ptr ()
  {
    if (mutex_)
    {
      unlock();
    }
  }


  /**
   * Release mutex. \a *this is set to undefined state. It can be set to
   * deined state again by moving another locked_ptr to it.
   */
  void unlock ()
  {
    mutex_->unlock();
    mutex_ = nullptr;
    data_ = nullptr;
  }


  /**
   * Swap \a *this data and mutex with \a that.
   */
  void swap (locked_ptr &that) noexcept
  {
    std::swap(data_, that.data_);
    std::swap(mutex_, that.mutex_);
  }


  /**
   * Return pointer to bound data.
   */
  value_t *get () const noexcept
  {
    return data_;
  }


  /**
   * Return true if object has associated data or false otherwise.
   */
  explicit operator bool () const noexcept
  {
    return get() != nullptr;
  }


  /// \see get()
  value_t *operator-> () const noexcept
  {
    return get();
  }


  /**
   * Return reference to associated data
   */
  value_t &operator* () const noexcept
  {
    return *get();
  }


private:

  mutex_t *mutex_{};
  value_t *data_{};
};


/**
 * Swap \a a and \a b associated data and mutex fields.
 */
template <typename T, typename Mutex>
inline void swap (locked_ptr<T, Mutex> &a, locked_ptr<T, Mutex> &b) noexcept
{
  a.swap(b);
}


/**
 * Extended Lockable concept. This class internally owns mutable \a Mutex and
 * reference to asscociated data of type \a T. To access associated data, call
 * any of locking methods or for explicitly unsynchronised access, use
 * unlocked() method to acquire locked_ptr.
 *
 */
template <typename T, typename Mutex=std::mutex>
class lockable_t
{
public:

  /**
   * Associated data type.
   */
  using value_t = T;

  /**
   * Synchronisation device type
   */
  using mutex_t = Mutex;

  /**
   * Locked pointer
   */
  using ptr = locked_ptr<T, Mutex>;

  /**
   * Locked const pointer
   */
  using const_ptr = locked_ptr<const T, Mutex>;


  /**
   * Construct new lockable_t with associated \a data
   */
  lockable_t (value_t &data)
    : data_(data)
  {}


  /**
   * Return locked_ptr with associated data and mutex
   */
  ptr lock ()
  {
    return ptr(&data_, mutex_);
  }


  /**
   * Return locked_ptr with associated data and mutex
   */
  const_ptr lock () const
  {
    return const_ptr(&data_, mutex_);
  }


  /**
   * Return locked_ptr with associated data and mutex. If try_lock() fails, it
   * returns locked_ptr with no associated data.
   */
  ptr try_lock ()
  {
    return ptr(&data_, mutex_, std::try_to_lock);
  }


  /**
   * Return locked_ptr with associated data and mutex. If try_lock() fails, it
   * returns locked_ptr with no associated data.
   */
  const_ptr try_lock () const
  {
    return const_ptr(&data_, mutex_, std::try_to_lock);
  }


  /**
   * Return locked_ptr with associated data but with no mutex lock.
   */
  ptr unlocked ()
  {
    return ptr(&data_, mutex_, std::defer_lock);
  }


  /**
   * Return locked_ptr with associated data but with no mutex lock.
   */
  const_ptr unlocked () const
  {
    return const_ptr(&data_, mutex_, std::defer_lock);
  }


private:

  value_t &data_;
  mutable mutex_t mutex_{};
};


__sal_end
