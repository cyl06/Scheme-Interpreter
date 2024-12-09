#ifndef WEAK_PTR_H
#define WEAK_PTR_H
/*
 *actually sharedptr and weakptr
 */

#include <functional>
template <typename T>
class WeakPtr;
/* SharePtr */
template <typename T>
class SharedPtr {
    friend class WeakPtr<T>;

  private:
    struct Track {
        T* _ptr;
        size_t strong_cnt;
        size_t weak_cnt;
        Track(T* other)
            : _ptr(other), strong_cnt(other != nullptr), weak_cnt(0) { }
        ~Track() {
            delete _ptr;
        }
    };
    Track* _p;
    void release() {
        if (_p) {
            if (--(*_p).strong_cnt == 0) {
                delete _p->_ptr;
                _p->_ptr = nullptr;
            }
            if ((*_p).strong_cnt == 0 && (*_p).weak_cnt == 0) {
                delete _p;
            }
        }
        _p = nullptr;
    }

  public:
    SharedPtr(T* ptr = nullptr) : _p(ptr ? new Track(ptr) : nullptr) { }
    SharedPtr(Track* ptr) : _p(ptr) {
        if (_p)
            (*_p).strong_cnt++;
    }
    SharedPtr(const SharedPtr& other) : _p(other._p) {
        if (_p)
            (*_p).strong_cnt++;
    }
    SharedPtr(SharedPtr&& other) noexcept : _p(other._p) { other._p = nullptr; }
    ~SharedPtr() { release(); }

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            release();
            _p = other._p;
            if (_p)
                (*_p).strong_cnt++;
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            release();
            _p = other._p;
            other._p = nullptr;
        }
        return *this;
    }

    operator bool() const { return _p != nullptr; }
    size_t use_count() const { return _p ? (*_p).strong_cnt : 0; }
    T* get() const { return _p ? _p->_ptr : nullptr; }
    T& operator*() const { return *(_p->_ptr); }
    T* operator->() const { return _p->_ptr; }
    void reset(T* ptr = nullptr) {
        release();
        _p = ptr ? new Track(ptr) : nullptr;
    }
};
template <typename T, typename... Args>
SharedPtr<T> make_shared(Args&&... args) {
    return SharedPtr<T>(new T(std::forward<Args>(args)...));
}
/* WeakPtr */
template <typename T>
class WeakPtr {
  private:
    typedef typename SharedPtr<T>::Track sp;
    sp* _p;
    void release() {
        if (_p && --(*_p).weak_cnt == 0 && (*_p).strong_cnt == 0) {
            delete _p;
        }
        _p = nullptr;
    }

  public:
    WeakPtr() : _p(nullptr) { }
    WeakPtr(const WeakPtr& other) : _p(other._p) {
        if (_p)
            (*_p).weak_cnt++;
    }
    WeakPtr(WeakPtr&& other) noexcept : _p(other._p) { other._p = nullptr; }
    WeakPtr(const SharedPtr<T>& other) : _p(other._p) {
        if (_p)
            (*_p).weak_cnt++;
    }
    ~WeakPtr() { release(); }

    WeakPtr& operator=(const WeakPtr& other) {
        if (this != &other) {
            release();
            _p = other._p;
            if (_p)
                (*_p).weak_cnt++;
        }
        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) noexcept {
        if (this != &other) {
            release();
            _p = other._p;
            other._p = nullptr;
        }
        return *this;
    }
    WeakPtr& operator=(const SharedPtr<T>& other) {
        release();
        _p = other._p;
        if (_p)
            (*_p).weak_cnt++;

        return *this;
    }

    void reset(T* ptr = nullptr) {
        release();
        _p = ptr ? new sp(ptr) : nullptr;
    }
    size_t use_count() const { return _p ? (*_p).strong_cnt : 0; }
    bool expired() const { return (!_p) || (*_p).strong_cnt == 0; }
    SharedPtr<T> lock() const {
        return !expired() ? SharedPtr<T>(_p) : SharedPtr<T>();
    }
    void swap(WeakPtr& other) noexcept { std::swap(_p, other._p); }
};

// Non-member swap function
template <typename T>
void swap(WeakPtr<T>& lhs, WeakPtr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

#endif // WEAK_PTR