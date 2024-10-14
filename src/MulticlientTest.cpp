#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include "739kv.h"
#include <cstring>
#include <cassert>

using namespace std;
const int STATUS_NO_OLD_VALUE = 1;

void LogClientInfo(int client_id, const string& message) {
    std::cout << "[Client " << client_id << " INFO] " << message << std::endl;
}

void start_client(int client_id, const string& server_address) {
    // Initialize the gRPC client
    if (kv739_init(const_cast<char*>(server_address.c_str())) != 0) {
        LogClientInfo(client_id, "Failed to initialize gRPC client.");
        return;
    }
}

int test_put(char* key, char* value) {
    // test 1: put key "client id" with value "Value1", expecting no old value
    char old_value[256];
    return kv739_put(key, value, old_value);
}

string test_get(char* key) {
    // test 2: get key "client id", expecting value "Value1"
    char value[256];
    int status = kv739_get(key, value);
    return string(value);
}

void stop_client() {
    kv739_shutdown();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_address> <client_id>" << std::endl;
        return 1;
    }

    string server_address = argv[1];
    int client_id = std::stoi(argv[2]);
    char test_val[] = "vvvvv";

    start_client(client_id, server_address);

    // Test correctness of PUT operations
    std::cout << "Multiple Clients Test 1 ... Client " << client_id << " engages" << std::endl;
    string k1 = "Mulcle" + client_id;
    int status = test_put(const_cast<char*>(k1.c_str()), test_val);
    assert(status == STATUS_NO_OLD_VALUE);

    // Test correctness of get operations
    std::cout << "Multiple Clients Test 2 ... Client " << client_id << " engages" << std::endl;
    string val = test_get(const_cast<char*>(k1.c_str()));
    assert(val == test_val);

    // Test put in hight QPS
    std::cout << "Multiple Clients Test 3 ... Client " << client_id << " engages" << std::endl;
    for (int i=0; i<1000; i++) {
        test_put(const_cast<char*>(to_string(i).c_str()), const_cast<char*>(to_string(client_id).c_str()));
    }

    // Test correctness of PUT & GET operations simultaneously
    std::cout << "Multiple Clients Test 4 ... Client " << client_id << " engages" << std::endl;
    if (client_id % 2 != 0) {
        // 1,3,5 put
        test_put(const_cast<char*>(to_string(client_id + 1000).c_str()), test_val);
    } else {
        string val = test_get(const_cast<char*>(to_string(client_id + 1000 -1).c_str()));
        // value could be inserted or not
        assert(val == test_val || val == "");
    }

    stop_client();
    
    return 0;
}