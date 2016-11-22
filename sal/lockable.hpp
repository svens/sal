#pragma once

/**
 * \file sal/lockable.hpp
 */

#include <sal/config.hpp>
#include <sal/assert.hpp>
#include <sal/error.hpp>
#include <mutex>


namespace sal {
__sal_begin


template <typename T, typename Mutex>
class locked_ptr
{
public:

  using value_t = T;
  using mutex_t = Mutex;


  locked_ptr (const locked_ptr &) = delete;
  locked_ptr &operator= (const locked_ptr &) = delete;


  locked_ptr () noexcept = default;


  explicit locked_ptr (value_t *data, mutex_t &mutex)
    : mutex_(&mutex)
    , data_(data)
  {
    mutex_->lock();
  }


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


  locked_ptr (value_t *data, mutex_t &mutex, std::adopt_lock_t) noexcept
    : mutex_(&mutex)
    , data_(data)
  {}


  locked_ptr (value_t *data, mutex_t &, std::defer_lock_t) noexcept
    : mutex_(nullptr)
    , data_(data)
  {}


  locked_ptr (locked_ptr &&that) noexcept
    : mutex_(that.mutex_)
    , data_(that.data_)
  {
    that.mutex_ = nullptr;
    that.data_ = nullptr;
  }


  locked_ptr &operator= (locked_ptr &&that)
  {
    if (mutex_)
    {
      unlock();
    }
    locked_ptr(std::move(that)).swap(*this);
    return *this;
  }


  ~locked_ptr ()
  {
    if (mutex_)
    {
      unlock();
    }
  }


  void unlock ()
  {
    mutex_->unlock();
    mutex_ = nullptr;
    data_ = nullptr;
  }


  void swap (locked_ptr &that) noexcept
  {
    std::swap(data_, that.data_);
    std::swap(mutex_, that.mutex_);
  }


  value_t *get () const noexcept
  {
    return data_;
  }


  explicit operator bool () const noexcept
  {
    return get() != nullptr;
  }


  value_t *operator-> () const noexcept
  {
    return get();
  }


  typename std::add_lvalue_reference<value_t>::type operator* () const noexcept
  {
    return *get();
  }


private:

  mutex_t *mutex_{};
  value_t *data_{};
};


template <typename T, typename Mutex>
inline void swap (locked_ptr<T, Mutex> &a, locked_ptr<T, Mutex> &b) noexcept
{
  a.swap(b);
}


template <typename T, typename Mutex=std::mutex>
class lockable_t
{
public:

  using value_t = T;
  using mutex_t = Mutex;
  using ptr = locked_ptr<T, Mutex>;
  using const_ptr = locked_ptr<const T, Mutex>;


  lockable_t (value_t &data)
    : data_(data)
  {}


  ptr lock ()
  {
    return ptr(&data_, mutex_);
  }


  const_ptr lock () const
  {
    return const_ptr(&data_, mutex_);
  }


  ptr try_lock ()
  {
    return ptr(&data_, mutex_, std::try_to_lock);
  }


  const_ptr try_lock () const
  {
    return const_ptr(&data_, mutex_, std::try_to_lock);
  }


  ptr unlocked ()
  {
    return ptr(&data_, mutex_, std::defer_lock);
  }


  const_ptr unlocked () const
  {
    return const_ptr(&data_, mutex_, std::defer_lock);
  }


private:

  value_t &data_;
  mutable mutex_t mutex_{};
};


__sal_end
} // namespace sal
