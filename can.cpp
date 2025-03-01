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


int main() {
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
    
    while (true) {
        ssize_t nbytes = read(sock, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            std::cerr << "Error while reading: " << strerror(errno) << std::endl;
            break;
        }
        
        std::cout << "Received CAN ID: " << std::hex << frame.can_id << " Data: ";
        for (int i = 0; i < frame.can_dlc; i++) {
            std::cout << std::hex << static_cast<int>(frame.data[i]) << " ";
        }
        std::cout << std::dec << std::endl;
    }
    
    close(sock);
    return EXIT_SUCCESS;
}
