#pragma once
#include <yvals_core.h>
#include <__msvc_chrono.hpp>
#include <cstdlib>
#include <system_error>
#include <thread>
#include <utility>
#include <xcall_once.h>
#include <mutex>
#include <xthreads.h>

namespace Sarah {
    class condition_variable;

    class BaseMutex {
    public:
        BaseMutex(int _Flags = 0) noexcept {
            _Mtx_init_in_situ(_Mymtx(), _Flags | _Mtx_try);
        }

        BaseMutex(const BaseMutex&) = delete;
        BaseMutex& operator=(const BaseMutex&) = delete;

        void lock() {
            if (_Mtx_lock(_Mymtx()) != _Thrd_result::_Success) {
                // undefined behavior, only occurs for plain mutexes (N4950 [thread.mutex.requirements.mutex.general]/6)
                _STD _Throw_Cpp_error(std::_RESOURCE_DEADLOCK_WOULD_OCCUR);
            }

            if (!_Verify_ownership_levels()) {
                // only occurs for recursive mutexes (N4950 [thread.mutex.recursive]/3)
                // POSIX specifies EAGAIN in the corresponding situation:
                // https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutex_lock.html
                _STD _Throw_Cpp_error(std::_RESOURCE_UNAVAILABLE_TRY_AGAIN);
            }
        }

        _NODISCARD bool try_lock() {
            return _Mtx_trylock(_Mymtx()) == _Thrd_result::_Success;
        }

        void unlock() {
            _Mtx_unlock(_Mymtx());
        }

protected:
    _NODISCARD_TRY_CHANGE_STATE bool _Verify_ownership_levels() noexcept {
        if (_Mtx_storage._Count == INT_MAX) {
            --_Mtx_storage._Count;
            return false;
        }

        return true;
    }

    public:
        friend condition_variable;
        friend std::condition_variable_any;

        _Mtx_internal_imp_t _Mtx_storage{};

        _Mtx_t _Mymtx() noexcept {
            return &_Mtx_storage;
        }
    };

    class mutex : public BaseMutex {
    public:
        mutex() noexcept
            : BaseMutex() {}

        mutex(const mutex&) = delete;
        mutex& operator=(const mutex&) = delete;
    };

    class condition_variable { // class for waiting for conditions
    public:
        condition_variable() noexcept /* strengthened */ {
            _Cnd_init_in_situ(_Mycnd());
        }

       /* ~condition_variable() noexcept {
            _Cnd_destroy_in_situ(_Mycnd());
        }*/

        condition_variable(const condition_variable&)            = delete;
        condition_variable& operator=(const condition_variable&) = delete;

        void notify_one() noexcept { // wake up one waiter
            _Cnd_signal(_Mycnd());
        }

        void notify_all() noexcept { // wake up all waiters
            _Cnd_broadcast(_Mycnd());
        }

        void wait(std::unique_lock<mutex>& _Lck) noexcept /* strengthened */ { // wait for signal
            // Nothing to do to comply with LWG-2135 because std::mutex lock/unlock are nothrow
            _Cnd_wait(_Mycnd(), _Lck.mutex()->_Mymtx());
        }

        template <class _Predicate>
        void wait(std::unique_lock<mutex>& _Lck, _Predicate _Pred) { // wait for signal and test predicate
            while (!_Pred()) {
                wait(_Lck);
            }
        }

        template <class _Rep, class _Period>
        std::cv_status wait_for(std::unique_lock<mutex>& _Lck, const std::chrono::duration<_Rep, _Period>& _Rel_time) {
            // wait for duration
            if (_Rel_time <= std::chrono::duration<_Rep, _Period>::zero()) {
                // we don't unlock-and-relock _Lck for this case because it's not observable
                return std::cv_status::timeout;
            }
            return wait_until(_Lck, _To_absolute_time(_Rel_time));
        }

        template <class _Rep, class _Period, class _Predicate>
        bool wait_for(std::unique_lock<mutex>& _Lck, const std::chrono::duration<_Rep, _Period>& _Rel_time, _Predicate _Pred) {
            // wait for signal with timeout and check predicate
            return wait_until(_Lck, _To_absolute_time(_Rel_time), _STD _Pass_fn(_Pred));
        }

        template <class _Clock, class _Duration>
        std::cv_status wait_until(std::unique_lock<mutex>& _Lck, const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
            // wait until time point
    #if _HAS_CXX20
            static_assert(std::chrono::is_clock_v<_Clock>, "Clock type required");
    #endif // _HAS_CXX20
    #if _CONTAINER_DEBUG_LEVEL > 0
            _STL_VERIFY(
                _Lck.owns_lock(), "wait_until's caller must own the lock argument (N4958 [thread.condition.condvar]/17)");
            _STL_VERIFY(_Mtx_current_owns(_Lck.mutex()->_Mymtx()),
                "wait_until's calling thread must hold the lock argument's mutex (N4958 [thread.condition.condvar]/17)");
    #endif // _CONTAINER_DEBUG_LEVEL > 0
            for (;;) {
                const auto _Now = _Clock::now();
                if (_Abs_time <= _Now) {
                    // we don't unlock-and-relock _Lck for this case because it's not observable
                    return std::cv_status::timeout;
                }

                const unsigned long _Rel_ms_count = _Clamped_rel_time_ms_count(_Abs_time - _Now)._Count;

                const _Thrd_result _Res = _Cnd_timedwait_for_unchecked(_Mycnd(), _Lck.mutex()->_Mymtx(), _Rel_ms_count);
                if (_Res == _Thrd_result::_Success) {
                   return std::cv_status::no_timeout;
                }
            }
        }

        template <class _Clock, class _Duration, class _Predicate>
        bool wait_until(
            std::unique_lock<mutex>& _Lck, const std::chrono::time_point<_Clock, _Duration>& _Abs_time, _Predicate _Pred) {
            // wait for signal with timeout and check predicate
    #if _HAS_CXX20
            static_assert(std::chrono::is_clock_v<_Clock>, "Clock type required");
    #endif // _HAS_CXX20
            while (!_Pred()) {
                if (wait_until(_Lck, _Abs_time) == std::cv_status::timeout) {
                    return _Pred();
                }
            }

            return true;
        }

        // native_handle_type and native_handle() have intentionally been removed. See GH-3820.

        void _Register(std::unique_lock<mutex>& _Lck, int* _Ready) noexcept { // register this object for release at thread exit
            _Cnd_register_at_thread_exit(_Mycnd(), _Lck.release()->_Mymtx(), _Ready);
        }

        void _Unregister(mutex& _Mtx) noexcept { // unregister this object for release at thread exit
            _Cnd_unregister_at_thread_exit(_Mtx._Mymtx());
        }

    private:
        std::_Aligned_storage_t<72, alignof(void*)> _Cnd_storage;

        _Cnd_t _Mycnd() noexcept { // get pointer to _Cnd_internal_imp_t inside _Cnd_storage
            return reinterpret_cast<_Cnd_t>(&_Cnd_storage);
        }
    };
}
