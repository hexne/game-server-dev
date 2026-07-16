/********************************************************************************
* @Author : hexne
* @Date   : 2026/04/30 14:52:57
********************************************************************************/
module;
export module log;
import std;
import lock_free_queue;
import time;

class log {
    std::jthread thread_{};
    MPSCQueue<std::string> message_queue_{};
    std::atomic_int queue_size_{};
    std::ofstream log_file_{};

    std::string log_name() {
        auto now = Time::now();
        return std::format("{}.log", now.get_date_string());
    }

    void run() {
        if (!std::filesystem::exists("./logs")) {
            std::filesystem::create_directory("./logs");
            log_file_ = std::ofstream("/logs/" + log_name());
        }

        while (!thread_.get_stop_token().stop_requested()
            || queue_size_.load(std::memory_order_acquire)) {
            auto msg = message_queue_.pop();
            if (!msg) {
                // std::this_thread::yield();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            else {
                std::println("{}", msg.value());
                std::println(log_file_, "{}", msg.value());
                queue_size_.fetch_sub(1, std::memory_order_release);
            }
        }
    }
public:
    log() {
        thread_ = std::jthread(&log::run, this);
    }
    void push_log(const std::string& msg, bool info = false, const std::source_location& loc = std::source_location::current()) {

        auto now = LocalTime::now();
        // [time] [file:line] :
        queue_size_.fetch_add(1, std::memory_order_release);
        if (info) {
            auto file = std::filesystem::path(loc.file_name()).filename();
            auto message = std::format("[{}] [{}:{}:{}] {}",
                now.get_string(), file, loc.line(), loc.function_name(), msg);
            message_queue_.push(message);
            return;
        }
        auto message = std::format("[{}] {}", now.get_string(), msg);
        message_queue_.push(message);

    }

    ~log() {
        while (auto size = queue_size_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        thread_.request_stop();
        thread_.join();
    }
};

export log& Log() {
    static log ret;
    return ret;
}
