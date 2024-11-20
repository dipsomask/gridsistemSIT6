#include <boost/asio.hpp>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <vector>
#include <string.h>

#define SERVER_PORT 65000
#define TIMEOUT_IN_SECONDS 5


using str = std::string;
using std::to_string;
using std::vector;
using namespace boost::asio;
using ip::udp;

namespace settings{

std::array<udp::endpoint, 5> workers_endpoints = {
    udp::endpoint(udp::v4(), 65001),
    udp::endpoint(udp::v4(), 65002),
    udp::endpoint(udp::v4(), 65003),
    udp::endpoint(udp::v4(), 65004),
    udp::endpoint(udp::v4(), 65005)
};

}