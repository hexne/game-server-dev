/********************************************************************************
* @Author : hexne
* @Date   : 2026/04/25 14:03:43
********************************************************************************/
export module croutine;
import std;

// @TODO
// 异常处理
// void处理
template <typename T>
struct Value {
    T value;
    using Type = T;
};

template <>
struct Value<void> {
    using Type = Value;
};


template <typename T>
struct Promise {
    T value;
    std::coroutine_handle<> handle{};

    auto get_return_object() {
        return std::coroutine_handle<Promise>::from_promise(*this);
    }

    void unhandled_exception() noexcept {  }

    auto initial_suspend() noexcept {
        return std::suspend_always();
    }

    auto final_suspend() noexcept {
        return std::suspend_always();
    }

    auto yield_value(T value) {
        this->value = value;
        return std::suspend_always();
    }

    void return_value(T value) {
        this->value = value;
    }
};

export template <typename T>
struct Task {
    using promise_type = Promise<T>;
    Task(std::coroutine_handle<promise_type> h) : handle_(h) {  }

    template <typename U>
    struct Awaiter {
        std::coroutine_handle<Promise<U>> handle;
        bool await_ready() { return false;  }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) {
            // return h;
            return handle;
        }
        U await_resume() {
            return handle.promise().value;
        }
    };

    auto operator co_await() {
        return Awaiter<T>{handle_};
    }

    T result() {
        return handle_.promise().value;
    }

    bool done() {
        return handle_.done();
    }

    void resume() {
        handle_.resume();
    }
    ~Task() {
        handle_.destroy();
    }
private:
    std::coroutine_handle<promise_type> handle_;
};
