#include <iostream>
#include <string>
#include <thread>
#include <shared_mutex>
#include <unordered_map>
#include <leveldb/db.h>
#include <grpcpp/grpcpp.h>
#include "generated/kvstore.pb.h"
#include "generated/kvstore.grpc.pb.h"
#include <unistd.h>  

using namespace std;
const int GET_KEY_FOUND = 0;
const int GET_KEY_NOT_FOUND = -1;
const int PUT_NO_OLD_VALUE = 1;
const int PUT_OLD_VALUE_FOUND = 0;
const int PUT_FAILURE = -1;

void LogInfo(const string& message) {
    std::cout << "[SERVER INFO] " << message << std::endl;
}

void LogError(const string& message) {
    std::cerr << "[SERVER ERROR] " << message << std::endl;
}

class KVStorageServiceImpl final : public kvstore::KVStore::Service {
    
    // unordered_map<string, shared_ptr<shared_mutex>> key_locks;
    leveldb::DB* db_;    
    shared_mutex db_lock;

public:
    KVStorageServiceImpl(const string& db_path) {
        leveldb::Options options;
        options.create_if_missing = true;
        leveldb::Status status = leveldb::DB::Open(options, db_path, &db_);
        if (!status.ok()) {
            LogError("Unable to open/create database " + db_path);
            LogError(status.ToString());
            exit(1);
        }
    }

    ~KVStorageServiceImpl() {
        delete db_;
    }

    grpc::Status Put(grpc::ServerContext* context, const kvstore::PutRequest* request, kvstore::PutResponse* response) {
        // LogInfo("PUT request received. Key: " + request->key() + ", Value: " + request->value());

        unique_lock lock_guard(db_lock);
        string old_value;

        // get the old value
        auto status = DbGet(request->key(), &old_value);

        if (status.IsNotFound()) {
            // key not found, return status -1
            response->set_status(PUT_NO_OLD_VALUE);
            response->set_old_value("");
        } else if (status.ok()) {
            // found key, return value and status 0
            response->set_old_value(old_value);
            response->set_status(PUT_OLD_VALUE_FOUND);
        } else {
            // error in retrieving key (e.g., I/O error), return status -1
            response->set_status(PUT_FAILURE);
            return grpc::Status::CANCELLED;
        }

        // write the new value
        status = DbPut(request->key(), request->value());
        if (!status.ok()) {
            LogError("PUT failed for key: " + request->key());
            response->set_status(PUT_FAILURE);  
            return grpc::Status::CANCELLED;
        }

        // LogInfo("PUT successful for key: " + request->key());
        return grpc::Status::OK;
    }

    grpc::Status Get(grpc::ServerContext* context, const kvstore::GetRequest* request, kvstore::GetResponse* response) {
        // LogInfo("GET request received. Key: " + request->key());

        string value;
        auto status = DbGet(request->key(), &value);

        if (status.IsNotFound()) {
            // key not found, return status -1
            response->set_status(GET_KEY_NOT_FOUND);
            // LogInfo("Key not found: " + request->key());
            return grpc::Status::OK;
        } else if (status.ok()) {
            // found key, return value and status 0
            response->set_value(value);
            response->set_status(GET_KEY_FOUND);
            // LogInfo("GET successful. Key: " + request->key() + ", Value: " + value);
            return grpc::Status::OK;
        } else {
            // error in retrieving key (e.g., I/O error), return status -1
            response->set_status(GET_KEY_NOT_FOUND);
            // LogError("Failed to retrieve key: " + request->key());
            return grpc::Status::CANCELLED;
        }
    }

private:
    leveldb::Status DbPut(const string& key, const string& value) {
        leveldb::WriteOptions options;
        return db_->Put(options, key, value);
    }

    leveldb::Status DbGet(const string& key, string* value) {
        leveldb::ReadOptions options;
        return db_->Get(options, key, value);
    }
};

void RunServer(const string& server_address, const string& db_path) {
    KVStorageServiceImpl service(db_path);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LogInfo("Server listening on " + server_address);
    server->Wait();
}

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <server_address:port> <db_path>" << std::endl;
    std::cerr << "Example: " << program_name << " 0.0.0.0:5001 ./leveldb" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    string server_address(argv[1]);
    string db_path(argv[2]);

    // Print the PID of the server process
    pid_t pid = getpid();
    LogInfo("Server process PID: " + to_string(pid));
    LogInfo("Starting server with database path: " + db_path);

    RunServer(server_address, db_path);

    return 0;
}