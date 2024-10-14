#include <iostream>
#include <cassert>
#include <cstring>
#include <filesystem>  
#include <thread>
#include <chrono>
#include <cstdlib>      
#include <sys/types.h>  
#include <signal.h>     
#include <unistd.h>     
#include <sys/wait.h>   
#include "739kv.h"
#include <vector>

#define ASSERT_WITH_CLEANUP(condition, cleanup_action) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " #condition << std::endl; \
            cleanup_action; \
            assert(condition); \
        } \
    } while (0)

pid_t server_pid = -1;
const int GET_KEY_FOUND = 0;
const int GET_KEY_NOT_FOUND = -1;
const int PUT_NO_OLD_VALUE = 1;
const int PUT_OLD_VALUE_FOUND = 0;
const int PUT_FAILURE = -1;

// clean up database
void clear_db(const std::string& db_path) {
    try {
        std::filesystem::remove_all(db_path);  // delete the directory and its contents
        std::filesystem::create_directory(db_path);  // recreate the directory
        std::cout << "Database cleared and recreated at: " << db_path << std::endl;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error clearing DB: " << e.what() << std::endl;
    }
}

void start_server(const std::string& server_executable, const std::string& server_addr, const std::string& db_path) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: start the server
        execl(server_executable.c_str(), server_executable.c_str(), server_addr.c_str(), db_path.c_str(), (char*)NULL);
        // If execl returns, an error occurred
        std::cerr << "Failed to start server process." << std::endl;
        exit(1);
    } else if (pid > 0) {
        // Parent process: save the server PID
        server_pid = pid;
        std::cout << "Server started with PID: " << server_pid << std::endl;
        // Give the server time to start up
        std::this_thread::sleep_for(std::chrono::seconds(2));
    } else {
        // Fork failed
        std::cerr << "Failed to fork process to start server." << std::endl;
        exit(1);
    }
}

void stop_server() {
    if (server_pid > 0) {
        std::cout << "Stopping server with PID: " << server_pid << std::endl;
        kill(server_pid, SIGKILL);
        // Wait for the server process to terminate
        waitpid(server_pid, NULL, 0);
        server_pid = -1;
    }
}

// Helper function to run a PUT operation 
void test_put(const std::string& key, const std::string& new_value, const std::string& expected_old_value, int expected_status) {
    char old_value[256];
    int status = kv739_put(const_cast<char*>(key.c_str()), const_cast<char*>(new_value.c_str()), old_value);
    // std::cout << "Expected status: " << expected_status << ", Actual status: " << status << std::endl;
    // std::cout << "Expected old value: " << expected_old_value << ", Actual old value: " << old_value << std::endl;

    ASSERT_WITH_CLEANUP(status == expected_status, stop_server(); exit(1));
    if (expected_status == 0) {
        ASSERT_WITH_CLEANUP(strcmp(old_value, expected_old_value.c_str()) == 0, stop_server(); exit(1));
    }
}

// Helper function to run a GET operation 
void test_get(const std::string& key, const std::string& expected_value, int expected_status) {
    char value[256];
    // std::cout << std::endl;
    // std::cout << "Testing GET operation with key: " << key << std::endl;
    int status = kv739_get(const_cast<char*>(key.c_str()), value);
    // std::cout << "Expected status: " << expected_status << ", Actual status: " << status << std::endl;
    // std::cout << "Expected value: " << expected_value << ", Actual value: " << value << std::endl;
    ASSERT_WITH_CLEANUP(status == expected_status, stop_server(); exit(1));
    if (expected_status == 0) {
        ASSERT_WITH_CLEANUP(strcmp(expected_value.c_str(), value) == 0, stop_server(); exit(1));
    }
}

void test_reliability(const std::string& server_executable, const std::string& server_addr, const std::string& db_path) {
    std::cout << std::endl;
    std::cout << "**************************************************" << std::endl;
    std::cout << "Starting reliability test..." << std::endl;
    std::cout << "Starting reliability test using database: " << db_path << std::endl;

    // Step 1: Start the server process
    start_server(server_executable, server_addr, db_path);

    // Step 2: Initialize and perform a PUT operation
    int init_status = kv739_init(const_cast<char*>(server_addr.c_str()));
    ASSERT_WITH_CLEANUP(init_status == 0, stop_server(); exit(1));

    std::cout << "Putting key 'Durablekey' with value 'DurableValue1'" << std::endl;
    test_put("durablekey", "DurableValue1", "", 1);  // No old value should exist

    // Step 3: Simulate a crash by stopping the server
    std::cout << std::endl;
    std::cout << "Simulating server crash..." << std::endl;
    stop_server();

    // Step 4: Restart the server
    std::cout << std::endl;
    std::cout << "Restarting server..." << std::endl;
    start_server(server_executable, server_addr, db_path);

    // Reinitialize the client after server restart
    kv739_shutdown();  // Shutdown previous client
    init_status = kv739_init(const_cast<char*>(server_addr.c_str()));
    ASSERT_WITH_CLEANUP(init_status == 0, stop_server(); exit(1));

    // Step 5: Check if the data is still there after the crash
    std::cout << std::endl;
    std::cout << "Checking if 'durablekey' still has value 'DurableValue1' after restart" << std::endl;
    test_get("durablekey", "DurableValue1", 0);

    // Shutdown the client after test
    int shutdown_status = kv739_shutdown();

    // Step 6: Stop the server after the test
    stop_server();

    std::cout << "Reliability test passed!" << std::endl;
}

// Test function to validate the correctness of the kv739 operations
void test_correctness(const std::string& server_executable, const std::string& server_addr,  const std::string& db_path, int num_operations) {
    // Step 1: Start the server process before running the correctness tests
    std::cout << std::endl;
    std::cout << "**************************************************" << std::endl;
    std::cout << "Starting server for correctness tests using database: " << db_path << std::endl;
    start_server(server_executable, server_addr, db_path);

    // Initialize the client
    int init_status = kv739_init(const_cast<char*>(server_addr.c_str()));
    ASSERT_WITH_CLEANUP(init_status == 0, stop_server(); exit(1));

    std::cout << "Running correctness tests..." << std::endl;

    // Test 1: Put key "correctkey1" with value "Value1"
    printf("Correctness Test 1 ...\n");
    test_put("correctkey1", "Value1", "", PUT_NO_OLD_VALUE);  // No old value should exist

    // Test 2: Get key "correctkey1", expecting value "Value1"
    printf("Correctness Test 2 ...\n");
    test_get("correctkey1", "Value1", GET_KEY_FOUND);

    // Test 3: Put key "correctkey1" with a new value "Value2"
    printf("Correctness Test 3 ...\n");
    test_put("correctkey1", "Value2", "Value1", PUT_OLD_VALUE_FOUND);  // Old value should be "Value1"

    // Test 4: Get key "correctkey1", expecting the updated value "Value2"
    printf("Correctness Test 4 ...\n");
    test_get("correctkey1", "Value2", GET_KEY_FOUND);

    // Test 5: Get a non-existent key "correctkey2", expecting not found status
    printf("Correctness Test 5 ...\n");
    test_get("correctkey2", "", GET_KEY_NOT_FOUND);

    // Test 6: Put key "correctkey2" with value "Value3"
    printf("Correctness Test 6 ...\n");
    test_put("correctkey2", "Value3", "", PUT_NO_OLD_VALUE);  // No old value should exist

    // Test 7: Get key "correctkey2", expecting value "Value3"
    printf("Correctness Test 7 ...\n");
    test_get("correctkey2", "Value3", GET_KEY_FOUND);

    // Test 8: Put sequence of keys & values
    printf("Correctness Test 8 ...\n");
    for (int i = 1; i <= num_operations; ++i) {
        std::string key = "cat" + std::to_string(i);
        std::string value = "meow" + std::to_string(i);
        test_put(key, value, "", PUT_NO_OLD_VALUE);  // No old value should exist
    }

    for (int i = 1; i <= num_operations; ++i) {
        std::string key = "cat" + std::to_string(i);
        std::string value = "meow" + std::to_string(i);
        test_get(key, value, GET_KEY_FOUND);
    }

    // Shutdown the client
    int shutdown_status = kv739_shutdown();
    std::cout << "All correctness tests passed!" << std::endl;

    // Step 4: Stop the server after all correctness tests are done
    stop_server();
}

void run_clients(const std::string& client_executable, const std::string& server_addr, int num_clients) {
    std::vector<pid_t> client_pids;

    for (int i = 1; i < num_clients + 1; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process: start the client
            execl(client_executable.c_str(), client_executable.c_str(), server_addr.c_str(), std::to_string(i).c_str(), (char*)NULL);
            // If execl returns, an error occurred
            std::cerr << "Failed to start client process." << std::endl;
            exit(1);
        } else if (pid > 0) {
            // Parent process: save the client PID
            client_pids.push_back(pid);
        } else {
            // Fork failed
            std::cerr << "Failed to fork process to start client." << std::endl;
            exit(1);
        }
    }

    // Wait for all client processes to finish
    for (pid_t pid : client_pids) {
        waitpid(pid, NULL, 0);
    }
    printf("All clients have finished. MultiClient Tests Pass\n");
}

void test_multiple_clients(const std::string& server_executable, const std::string& client_executable, const std::string& server_addr, const std::string& db_path, int num_clients) {
    std::cout << std::endl;
    std::cout << "**************************************************" << std::endl;
    std::cout << "Starting multiple clients test...\n" << std::endl;

    // Step 1: Start the server process
    start_server(server_executable, server_addr, db_path);

    // Step 2: Run multiple clients
    run_clients(client_executable, server_addr, num_clients);

    // Step 3: Stop the server process
    stop_server();
}

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <server_executable> <client_executable> <server_address> <db_path> <num_clients> <num_operations>" << std::endl;
    std::cerr << "Example: " << program_name << " ./server ./client 0.0.0.0:5001 ./leveldb 5 1000" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        print_usage(argv[0]);
        return 1;
    }

    std::string server_executable = argv[1];
    std::string client_executable = argv[2];
    std::string server_addr = argv[3];
    std::string db_path = argv[4];
    int num_clients = std::stoi(argv[5]);
    int num_operations = std::stoi(argv[6]);

    std::cout << "Server Executable: " << server_executable << std::endl;
    std::cout << "Server Address: " << server_addr << std::endl;
    std::cout << "Database Path: " << db_path << std::endl;

    clear_db(db_path);
    test_correctness(server_executable, server_addr, db_path, num_operations);
    
    clear_db(db_path);  // This ensures the database is clean for the next test
    test_reliability(server_executable, server_addr, db_path);

    clear_db(db_path);  
    test_multiple_clients(server_executable, client_executable, server_addr, db_path, num_clients);

    return 0;
}
