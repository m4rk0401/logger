#include <iostream>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fstream>
#include <chrono>
#include <vector>
#include <cstdint>
#include <thread>
#include <mutex>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

#pragma pack(push, 1)
struct CanFrame {
    uint32_t can_id;
    uint64_t timestamp_sec;
    uint32_t timestamp_nsec;
    uint8_t data[8];
};
#pragma pack(pop)

std::string getCurrentTimestamp(const std::string& format) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now = *std::localtime(&time_t_now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_now, format.c_str());
    return oss.str();
}

std::string getUniqueFilename(const std::string& directory, const std::string& prefix) {
    std::string base_filename = prefix + "_" + getCurrentTimestamp("%Y_%m_%d-%H_%M_%S");
    std::string filename = directory + "/" + base_filename + ".bin";
    int counter = 1;
    
    while (fs::exists(filename)) {
        filename = directory + "/" + base_filename + "_" + std::to_string(counter) + ".bin";
        counter++;
    }
    
    return filename;
}

void captureCanData(const std::string& interface, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        return;
    }

    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        std::cerr << "Error while opening socket for " << interface << ": " << strerror(errno) << std::endl;
        return;
    }

    strcpy(ifr.ifr_name, interface.c_str());
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "Error getting interface index for " << interface << ": " << strerror(errno) << std::endl;
        close(sock);
        return;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error in socket bind for " << interface << ": " << strerror(errno) << std::endl;
        close(sock);
        return;
    }

    std::cout << "Listening on " << interface << "..." << std::endl;

    std::mutex file_mutex;
    bool running = true;

    std::thread flush_thread([&]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::lock_guard<std::mutex> lock(file_mutex);
            file.flush();
        }
    });

    while (true) {
        ssize_t nbytes = read(sock, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            std::cerr << "Error while reading from " << interface << ": " << strerror(errno) << std::endl;
            break;
        }

        CanFrame frame_line;
        frame_line.can_id = frame.can_id;

        auto now = std::chrono::system_clock::now();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() - (seconds * 1'000'000'000LL);

        frame_line.timestamp_sec = static_cast<uint64_t>(seconds);
        frame_line.timestamp_nsec = static_cast<uint32_t>(nanoseconds);

        std::copy(std::begin(frame.data), std::end(frame.data), std::begin(frame_line.data));

        std::lock_guard<std::mutex> lock(file_mutex);
        file.write(reinterpret_cast<const char*>(&frame_line), sizeof(CanFrame));
    }

    std::cout << "Stopping " << interface << " listener..." << std::endl;

    running = false;
    flush_thread.join();
    file.close();
    close(sock);
}

int main() {
    std::string base_path = "/mnt/mmcblk0p3";
    std::string date_folder = getCurrentTimestamp("%Y_%m_%d");
    std::string directory = base_path + "/" + date_folder;

    if (!fs::exists(directory)) {
        fs::create_directories(directory);

        // Change ownership to xb025 to enable FTP access
        std::string chownCmd = "sudo chown -R xb025:xb025 " + directory;
        if (system(chownCmd.c_str()) != 0) {
            std::cerr << "Error: chown failed!" << std::endl;
        }
    }

    std::string filename_can0 = getUniqueFilename(directory, "CAN1");
    std::string filename_can1 = getUniqueFilename(directory, "CAN2");

    std::thread can0_thread(captureCanData, "can0", filename_can0);
    std::thread can1_thread(captureCanData, "can1", filename_can1);

    can0_thread.join();
    can1_thread.join();

    return EXIT_SUCCESS;
}
