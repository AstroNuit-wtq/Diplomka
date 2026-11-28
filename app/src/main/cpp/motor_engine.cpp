#include "motor_engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#if defined(__linux__) || defined(__ANDROID__)
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace {

// CRC-16 (CCITT, poly 0x1021) used by VESC UART protocol.
uint16_t vescCrc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x8000) crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
            else crc = static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

// COMM_SET_DUTY = 5, payload: int32 duty * 1e5
std::vector<uint8_t> buildSetDutyPacket(float duty) {
    duty = std::clamp(duty, -1.0f, 1.0f);
    const int32_t scaled = static_cast<int32_t>(std::lround(duty * 100000.0f));
    uint8_t payload[5];
    payload[0] = 5; // COMM_SET_DUTY
    payload[1] = static_cast<uint8_t>((scaled >> 24) & 0xFF);
    payload[2] = static_cast<uint8_t>((scaled >> 16) & 0xFF);
    payload[3] = static_cast<uint8_t>((scaled >> 8) & 0xFF);
    payload[4] = static_cast<uint8_t>(scaled & 0xFF);

    const uint16_t crc = vescCrc16(payload, 5);
    std::vector<uint8_t> packet;
    packet.reserve(10);
    packet.push_back(0x02);           // short packet
    packet.push_back(5);              // length
    packet.insert(packet.end(), payload, payload + 5);
    packet.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<uint8_t>(crc & 0xFF));
    packet.push_back(0x03);           // end
    return packet;
}

// COMM_SET_CURRENT = 6, payload: int32 current * 1e3 (amperes)
std::vector<uint8_t> buildSetCurrentPacket(float amps) {
    amps = std::clamp(amps, -100.0f, 100.0f);
    const int32_t scaled = static_cast<int32_t>(std::lround(amps * 1000.0f));
    uint8_t payload[5];
    payload[0] = 6; // COMM_SET_CURRENT
    payload[1] = static_cast<uint8_t>((scaled >> 24) & 0xFF);
    payload[2] = static_cast<uint8_t>((scaled >> 16) & 0xFF);
    payload[3] = static_cast<uint8_t>((scaled >> 8) & 0xFF);
    payload[4] = static_cast<uint8_t>(scaled & 0xFF);

    const uint16_t crc = vescCrc16(payload, 5);
    std::vector<uint8_t> packet;
    packet.reserve(10);
    packet.push_back(0x02);
    packet.push_back(5);
    packet.insert(packet.end(), payload, payload + 5);
    packet.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<uint8_t>(crc & 0xFF));
    packet.push_back(0x03);
    return packet;
}

}  // namespace

MotorEngine::MotorEngine() = default;

MotorEngine::~MotorEngine() {
    closePort();
}

bool MotorEngine::configure(const std::string& devicePath, int baud, bool dualChannel) {
    std::lock_guard lock(mutex_);
    closePort();
    devicePath_ = devicePath;
    baud_ = baud;
    dualChannel_ = dualChannel;
    lastError_.clear();

    if (devicePath_.empty()) {
        backend_ = Backend::Null;
        return true;
    }

#if defined(__linux__) || defined(__ANDROID__)
    if (openPort()) {
        backend_ = Backend::UartVesc;
        return true;
    }
#endif
    lastError_ = "Не удалось открыть порт " + devicePath_;
    backend_ = Backend::Null;
    return false;
}

bool MotorEngine::openPort() {
#if defined(__linux__) || defined(__ANDROID__)
    if (devicePath_.empty()) return false;
    fd_ = ::open(devicePath_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        lastError_ = std::string("open: ") + std::strerror(errno);
        return false;
    }
    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) {
        lastError_ = "tcgetattr failed";
        closePort();
        return false;
    }
    cfmakeraw(&tty);
    speed_t speed = B115200;
    switch (baud_) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        default: speed = B115200; break;
    }
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        lastError_ = "tcsetattr failed";
        closePort();
        return false;
    }
    tcflush(fd_, TCIOFLUSH);
    return true;
#else
    return false;
#endif
}

void MotorEngine::closePort() {
#if defined(__linux__) || defined(__ANDROID__)
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

bool MotorEngine::writePacket(const uint8_t* payload, size_t len) {
#if defined(__linux__) || defined(__ANDROID__)
    if (fd_ < 0 || !payload || len == 0) return false;
    size_t written = 0;
    while (written < len) {
        const ssize_t n = ::write(fd_, payload + written, len - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            lastError_ = std::string("write: ") + std::strerror(errno);
            return false;
        }
        written += static_cast<size_t>(n);
    }
    return true;
#else
    (void)payload;
    (void)len;
    return false;
#endif
}

void MotorEngine::sendDuty(float left, float right) {
    leftDuty_ = std::clamp(left, -1.0f, 1.0f);
    rightDuty_ = std::clamp(right, -1.0f, 1.0f);
    if (backend_ != Backend::UartVesc || fd_ < 0) return;

    auto pktL = buildSetDutyPacket(leftDuty_);
    writePacket(pktL.data(), pktL.size());
    if (!dualChannel_) {
        auto pktR = buildSetDutyPacket(rightDuty_);
        writePacket(pktR.data(), pktR.size());
    } else if (std::abs(leftDuty_ - rightDuty_) > 0.001f) {
        // Still send both so differential drive works on dual boards with separate IDs.
        auto pktR = buildSetDutyPacket(rightDuty_);
        writePacket(pktR.data(), pktR.size());
    }
}

void MotorEngine::sendCurrent(float leftAmps, float rightAmps) {
    if (backend_ != Backend::UartVesc || fd_ < 0) return;
    auto pktL = buildSetCurrentPacket(leftAmps);
    writePacket(pktL.data(), pktL.size());
    if (!dualChannel_ || std::abs(leftAmps - rightAmps) > 0.01f) {
        auto pktR = buildSetCurrentPacket(rightAmps);
        writePacket(pktR.data(), pktR.size());
    }
}

void MotorEngine::setMotion(float leftDuty, float rightDuty) {
    std::lock_guard lock(mutex_);
    if (stopForced_) {
        leftDuty = 0.0f;
        rightDuty = 0.0f;
    }
    if (!armed_) return;
    sendDuty(leftDuty, rightDuty);
}

void MotorEngine::stopMotion() {
    std::lock_guard lock(mutex_);
    stopForced_ = true;
    // Critical: duty/current = 0, but keep the serial link and arming state.
    // ESCs stay powered; only motion is cancelled.
    if (armed_) {
        sendDuty(0.0f, 0.0f);
        // Also force current to zero for hard stop on VESC.
        sendCurrent(0.0f, 0.0f);
    }
}

void MotorEngine::arm() {
    std::lock_guard lock(mutex_);
    armed_ = true;
    stopForced_ = false;
    sendDuty(0.0f, 0.0f);
}

void MotorEngine::disarm() {
    std::lock_guard lock(mutex_);
    // Full software disarm — not used by the UI STOP button.
    sendDuty(0.0f, 0.0f);
    sendCurrent(0.0f, 0.0f);
    armed_ = false;
}

void MotorEngine::applyStopFlag(bool stopRequested) {
    if (stopRequested) {
        stopMotion();
    } else {
        std::lock_guard lock(mutex_);
        stopForced_ = false;
        // Leave duty at zero until a new setMotion is issued by navigation.
        if (armed_) sendDuty(0.0f, 0.0f);
    }
}

MotorEngine::Status MotorEngine::status() const {
    std::lock_guard lock(mutex_);
    Status s;
    s.connected = (backend_ == Backend::UartVesc && fd_ >= 0) || backend_ == Backend::Null;
    s.armed = armed_;
    s.stopped = stopForced_ || (std::abs(leftDuty_) < 1e-4f && std::abs(rightDuty_) < 1e-4f);
    s.leftDuty = leftDuty_;
    s.rightDuty = rightDuty_;
    s.backendName = (backend_ == Backend::UartVesc) ? "uart-vesc" : "null";
    s.lastError = lastError_;
    return s;
}
