#pragma once

#include <chrono>
#include <cstdio>
#include <string>

#ifdef _WIN32

namespace ydlidar_ros2_driver {

class StdoutWarningThrottler {
 public:
  explicit StdoutWarningThrottler(std::chrono::steady_clock::duration) {}
};

}  // namespace ydlidar_ros2_driver

#else

#include <array>
#include <thread>
#include <unistd.h>

namespace ydlidar_ros2_driver {

class FixedPointsWarningThrottle {
 public:
  using Clock = std::chrono::steady_clock;

  explicit FixedPointsWarningThrottle(Clock::duration interval)
  : interval_(interval) {}

  bool should_emit(const std::string &line, Clock::time_point now = Clock::now()) {
    if (!is_fixed_points_warning(line)) {
      return true;
    }

    if (!has_last_emit_ || (now - last_emit_) >= interval_) {
      has_last_emit_ = true;
      last_emit_ = now;
      return true;
    }

    return false;
  }

  static bool is_fixed_points_warning(const std::string &line) {
    return line.find("Real points ") != std::string::npos &&
      line.find(" > fixed points ") != std::string::npos;
  }

 private:
  Clock::duration interval_;
  Clock::time_point last_emit_{};
  bool has_last_emit_ = false;
};

class StdoutWarningThrottler {
 public:
  explicit StdoutWarningThrottler(
    FixedPointsWarningThrottle::Clock::duration interval = std::chrono::seconds(5))
  : filter_(interval) {
    start();
  }

  ~StdoutWarningThrottler() {
    stop();
  }

  StdoutWarningThrottler(const StdoutWarningThrottler &) = delete;
  StdoutWarningThrottler &operator=(const StdoutWarningThrottler &) = delete;

 private:
  void start() {
    fflush(stdout);

    if (pipe(pipe_fds_) != 0) {
      return;
    }

    original_stdout_fd_ = dup(STDOUT_FILENO);
    if (original_stdout_fd_ == -1) {
      close(pipe_fds_[0]);
      close(pipe_fds_[1]);
      pipe_fds_[0] = -1;
      pipe_fds_[1] = -1;
      return;
    }

    if (dup2(pipe_fds_[1], STDOUT_FILENO) == -1) {
      close(original_stdout_fd_);
      original_stdout_fd_ = -1;
      close(pipe_fds_[0]);
      close(pipe_fds_[1]);
      pipe_fds_[0] = -1;
      pipe_fds_[1] = -1;
      return;
    }

    active_ = true;
    reader_thread_ = std::thread([this]() { this->read_stdout(); });
  }

  void stop() {
    if (!active_) {
      return;
    }

    fflush(stdout);
    dup2(original_stdout_fd_, STDOUT_FILENO);
    close(pipe_fds_[1]);
    pipe_fds_[1] = -1;

    if (reader_thread_.joinable()) {
      reader_thread_.join();
    }

    close(pipe_fds_[0]);
    close(original_stdout_fd_);
    pipe_fds_[0] = -1;
    original_stdout_fd_ = -1;
    active_ = false;
  }

  void read_stdout() {
    std::array<char, 512> buffer{};
    std::string pending;

    ssize_t bytes_read = 0;
    while ((bytes_read = read(pipe_fds_[0], buffer.data(), buffer.size())) > 0) {
      pending.append(buffer.data(), static_cast<size_t>(bytes_read));

      std::string::size_type newline_pos = std::string::npos;
      while ((newline_pos = pending.find('\n')) != std::string::npos) {
        emit_line(pending.substr(0, newline_pos + 1));
        pending.erase(0, newline_pos + 1);
      }
    }

    if (!pending.empty()) {
      emit_line(pending);
    }
  }

  void emit_line(const std::string &line) {
    if (!filter_.should_emit(line)) {
      return;
    }

    const char *data = line.data();
    size_t remaining = line.size();

    while (remaining > 0) {
      const ssize_t written = write(original_stdout_fd_, data, remaining);
      if (written <= 0) {
        return;
      }

      data += written;
      remaining -= static_cast<size_t>(written);
    }
  }

  FixedPointsWarningThrottle filter_;
  int pipe_fds_[2] = {-1, -1};
  int original_stdout_fd_ = -1;
  bool active_ = false;
  std::thread reader_thread_;
};

}  // namespace ydlidar_ros2_driver

#endif
