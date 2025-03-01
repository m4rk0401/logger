#include <iostream>
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

void generate_log_file(const std::string& filename, int lines = 4500) {
    std::ofstream file(filename, std::ios::binary);
    
    if (!file) {
        std::cerr << "Error opening file for writing!" << std::endl;
        return;
    }

    auto start = std::chrono::high_resolution_clock::now(); // Start timing

    for (int i = 0; i < lines; i++) {
        CanFrame frame;
        frame.can_id = 0x123;

        // Get current timestamp with nanosecond precision
        auto now = std::chrono::system_clock::now();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() - (seconds * 1'000'000'000LL);

        frame.timestamp_sec = static_cast<uint64_t>(seconds);
        frame.timestamp_nsec = static_cast<uint32_t>(nanoseconds);

        // Example CAN data
        uint8_t example_data[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
        std::copy(std::begin(example_data), std::end(example_data), std::begin(frame.data));

        // Write binary data
        file.write(reinterpret_cast<const char*>(&frame), sizeof(CanFrame));
    }

    file.close();
    
    auto end = std::chrono::high_resolution_clock::now(); // End timing
    std::chrono::duration<double> duration = end - start;
    
    std::cout << "Time taken to write " << lines << " frames: " << duration.count() << " seconds" << std::endl;
}

int main() {
    std::string file_path = "/home/logger/can_data.bin";
    generate_log_file(file_path);
    std::cout << "Binary log file generated: " << file_path << std::endl;
    return 0;
}
