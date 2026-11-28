#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

// Motor control for Flipsky / VESC-based dual ESCs.
// STOP must set duty/current to zero and keep the link armed.
// Motors are never powered down by the software STOP button.
class MotorEngine {
public:
    enum class Backend {
        Null,       // no hardware — logs only
        UartVesc,   // classic VESC UART (COMM_SET_DUTY / COMM_SET_CURRENT)
    };

    struct Status {
        bool connected = false;
        bool armed = false;
        bool stopped = true;   // duty/current forced to zero
        float leftDuty = 0.0f;
        float rightDuty = 0.0f;
        std::string backendName = "null";
        std::string lastError;
    };

    MotorEngine();
    ~MotorEngine();

    MotorEngine(const MotorEngine&) = delete;
    MotorEngine& operator=(const MotorEngine&) = delete;

    // Configure serial device (empty path keeps Null backend).
    // dualChannel: true for Mini Dual FSESC on one UART; false for two independent ESCs.
    bool configure(const std::string& devicePath, int baud = 115200, bool dualChannel = true);

    // Keep ESCs armed; only change commanded motion.
    void setMotion(float leftDuty, float rightDuty);   // duty in [-1, 1]
    void stopMotion();                                 // duty = 0, keep armed
    void arm();
    void disarm();                                     // only for full power-down (not used by UI STOP)

    Status status() const;

    // Called from the main loop when manualStop or vision.stopSignal changes.
    void applyStopFlag(bool stopRequested);

private:
    void sendDuty(float left, float right);
    void sendCurrent(float leftAmps, float rightAmps);
    bool openPort();
    void closePort();
    bool writePacket(const uint8_t* payload, size_t len);

    mutable std::mutex mutex_;
    Backend backend_ = Backend::Null;
    std::string devicePath_;
    int baud_ = 115200;
    bool dualChannel_ = true;
    int fd_ = -1;
    bool armed_ = false;
    bool stopForced_ = true;
    float leftDuty_ = 0.0f;
    float rightDuty_ = 0.0f;
    std::string lastError_;
};
