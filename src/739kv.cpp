#include <grpcpp/grpcpp.h>
#include "kvstore.pb.h"
#include "kvstore.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace std;

class KV739Client {
public:
    KV739Client(shared_ptr<Channel> channel)
        : stub_(kvstore::KVStore::NewStub(channel)) {}

    // GET operation
    int kv739_get(const string& key, string& value) {
        kvstore::GetRequest request;
        request.set_key(key);

        kvstore::GetResponse response;
        ClientContext context;

        // Call the remote Get function
        Status status = stub_->Get(&context, request, &response);

        // Process the response
        if (status.ok()) {
            if (response.status() == 0) {
                // Key found
                value = response.value();
                return 0;  // Success
            } else if (response.status() == 1) {
                // Key not found
                return 1;
            }
        }
        return -1;  // Error in communication or server failure
    }

    // PUT operation
    int kv739_put(const string& key, const string& value, string& old_value) {
        kvstore::PutRequest request;
        request.set_key(key);
        request.set_value(value);

        kvstore::PutResponse response;
        ClientContext context;

        // Call the remote Put function
        Status status = stub_->Put(&context, request, &response);

        // Process the response
        if (status.ok()) {
            if (response.status() == 0) {
                // Old value existed
                old_value = response.old_value();
                return 0;
            } else if (response.status() == 1) {
                // No old value existed
                return 1;
            }
        }
        return -1;  // Error in communication or server failure
    }

private:
    // The stub to communicate with the server
    std::unique_ptr<kvstore::KVStore::Stub> stub_;
};

// Global client instance to be used by the external C functions
KV739Client* client = nullptr;

// C API: Initialize the client connection
extern "C" int kv739_init(char *server_name) {
    string server_address(server_name);
    client = new KV739Client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    return client ? 0 : -1;
}

// C API: Shutdown the client and free resources
extern "C" int kv739_shutdown(void) {
    if (client) {
        delete client;
        client = nullptr;
        return 0;
    }
    return -1;
}

// C API: Get the value for a given key 0:present, 1:not present, -1:error
extern "C" int kv739_get(char *key, char *value) {
    string val;
    int status = client->kv739_get(key, val);
    strcpy(value, val.c_str());
    
    return status;
}

// C API: Put a key-value pair and get the old value (if exists) 0:present, 1:not present, -1:error
extern "C" int kv739_put(char *key, char *value, char *old_value) {
    string old_val;
    int status = client->kv739_put(key, value, old_val);
    strcpy(old_value, old_val.c_str());
    
    return status;  
}   

// Test main function
// int main(int argc, char** argv) {
//     if (argc != 5) {
//         cerr << "Usage: " << argv[0] << " <server_address> <operation> <key> <value>" << endl;
//         cerr << "Operations: put | get" << endl;
//         return -1;
//     }

//     char* server_addr = argv[1];
//     string op = argv[2];
//     char* key = argv[3];
    
//     // Initialize the gRPC client
//     if (kv739_init(server_addr) != 0) {
//         cerr << "Failed to initialize gRPC client." << endl;
//         return -1;
//     }

//     if (op == "put") {
//         char old_value[256];
//         auto status = kv739_put(key, argv[4], old_value);
//         if (status == 0) {
//             cout << "Success. old value: " << old_value << endl;
//         } else if (status == 1) {
//             cout << "Success. No old value." << endl;
//         } else {
//             cerr << "Failed to put key-value pair." << endl;
//         } 
//     } else if (op == "get") {
//         char value[256];
//         auto status = kv739_get(key, value);
//         if (status == 0) {
//             cout << "Success. value: " << value << endl;
//         } else if (status == 1) {
//             cout << "Key not found." << endl;
//         } else {
//             cerr << "Failed to get value for key." << endl;
//         }
//     } else {
//         cerr << "Invalid operation." << endl;
//     }

//     // Shutdown the gRPC client
//     kv739_shutdown();
//     return 0;
// }