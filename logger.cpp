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

#pragma pack(push, 1)  // Force 1-byte packing (disable padding)
struct CanFrame {
    uint32_t can_id;        // 4 bytes
    uint64_t timestamp_sec; // 8 bytes
    uint32_t timestamp_nsec;// 4 bytes
    uint8_t data[8];        // 8 bytes
};  // Total: 24 bytes exactly
#pragma pack(pop)  // Restore default struct packing

int main() {
    std::string filename = "/home/logger/can_data_2.bin";

    std::ofstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Error opening file for writing!" << std::endl;
        return EXIT_FAILURE;
    }

    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    
    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        std::cerr << "Error while opening socket: " << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }
    
    strcpy(ifr.ifr_name, "can1");
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "Error getting interface index: " << strerror(errno) << std::endl;
        close(sock);
        return EXIT_FAILURE;
    }
    
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error in socket bind: " << strerror(errno) << std::endl;
        close(sock);
        return EXIT_FAILURE;
    }
    
    std::cout << "Listening on can1..." << std::endl;

    int counter = 0;
    
    while (true) {
        ssize_t nbytes = read(sock, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            std::cerr << "Error while reading: " << strerror(errno) << std::endl;
            break;
        }

        CanFrame frame_line;
        frame_line.can_id = frame.can_id;

        // Get current timestamp with nanosecond precision
        auto now = std::chrono::system_clock::now();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() - (seconds * 1'000'000'000LL);

        frame_line.timestamp_sec = static_cast<uint64_t>(seconds);
        frame_line.timestamp_nsec = static_cast<uint32_t>(nanoseconds);

        std::copy(std::begin(frame.data), std::end(frame.data), std::begin(frame_line.data));

        // Write binary data
        file.write(reinterpret_cast<const char*>(&frame_line), sizeof(CanFrame));

        counter++;

        std::cout << "Counter: " << counter << std::endl;

        if(counter >= 99)
        {
            std::cout << "Break while loop" << std::endl;
            break;
        }
    }

    std::cout << "Received 100 messages. Stop program now..." << std::endl;

    file.close();
    
    close(sock);
    return EXIT_SUCCESS;
}
