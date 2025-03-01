#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdint>

#pragma pack(push, 1)  // Force 1-byte packing (disable padding)
struct CanFrame {
    uint32_t can_id;        // 4 bytes
    uint64_t timestamp_sec; // 8 bytes
    uint32_t timestamp_nsec;// 4 bytes
    uint8_t data[8];        // 8 bytes
};  // Total: 24 bytes exactly
#pragma pack(pop)  // Restore default struct packing

void convert_bin_to_txt(const std::string& bin_filename, const std::string& txt_filename) {
    std::ifstream bin_file(bin_filename, std::ios::binary);
    std::ofstream txt_file(txt_filename);

    if (!bin_file || !txt_file) {
        std::cerr << "Error opening files!" << std::endl;
        return;
    }

    CanFrame frame;
    while (bin_file.read(reinterpret_cast<char*>(&frame), sizeof(CanFrame))) {
        txt_file << frame.timestamp_sec << "." << std::setw(9) << std::setfill('0') << frame.timestamp_nsec
                 << " " << frame.can_id << " ";

        // Convert binary data to hex
        for (uint8_t byte : frame.data) {
            txt_file << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }

        txt_file << std::dec << std::endl; // Reset to decimal format
    }

    bin_file.close();
    txt_file.close();
    
    std::cout << "Conversion complete. Output file: " << txt_filename << std::endl;
}

int main(int argc, char* argv[]) {
    std::string file_name = "2025_03_01-23_46_51";

    std::string base_path = "/mnt/mmcblk0p3/2025_03_01/";
    std::string bin_file_path = base_path + file_name + ".bin";
    std::string txt_file_path = "/home/logger/" + file_name + ".txt";

    convert_bin_to_txt(bin_file_path, txt_file_path);
    return 0;
}
