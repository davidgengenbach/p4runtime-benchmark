#ifndef P4RUNTIME_BENCHMARK_UTILS_H
#define P4RUNTIME_BENCHMARK_UTILS_H

#include <iostream>
#include <grpcpp/grpcpp.h>
#include "p4/v1/p4runtime.grpc.pb.h"
#include <ctime>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>
#include "argh.h"

using json = nlohmann::json;

using StreamChannel = std::shared_ptr<::grpc::ClientReaderWriter<
        ::p4::v1::StreamMessageRequest,
        ::p4::v1::StreamMessageResponse
>>;
using Client = std::shared_ptr<p4::v1::P4Runtime::Stub>;

std::time_t getTimestamp() {
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

long getTimestampLowPrecision() {
    return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

StreamChannel streamChannel;

template<typename MeasurementType>
void saveTimestamps(const std::string &filename, std::vector<MeasurementType *> measurements,
                    const std::string &header = "") {
    std::ofstream measurementFile(filename);
    if (measurementFile.is_open()) {
        if (!header.empty()) {
            measurementFile << header << "\n";
        }

        for (const auto *measurement: measurements) {
            measurementFile << measurement->line() << "\n";
        }
        measurementFile.close();
    }
}


StreamChannel arbitrate(
        const std::shared_ptr<p4::v1::P4Runtime::Stub> &client,
        unsigned int deviceId = 1,
        unsigned int low = 0,
        unsigned int high = 1
) {
    auto *ctx = new grpc::ClientContext();
    StreamChannel channel = client->StreamChannel(ctx);
    p4::v1::StreamMessageRequest request;
    p4::v1::StreamMessageResponse response;
    request.mutable_arbitration()->set_device_id(deviceId);
    request.mutable_arbitration()->mutable_election_id()->set_low(low);
    request.mutable_arbitration()->mutable_election_id()->set_high(high);
    channel->Write(request);

    auto status = channel->Read(&response);
    if (!status || !response.has_arbitration()) {
        throw std::runtime_error("Could not arbitrate");
    }

    return channel;
}

std::shared_ptr<p4::v1::P4Runtime::Stub> getStub(const std::string &address = "localhost:28000") {
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto client = std::shared_ptr<p4::v1::P4Runtime::Stub>(std::move(p4::v1::P4Runtime::NewStub(channel)));
    streamChannel = arbitrate(client);
    return client;
}

std::vector<p4::v1::TableEntry> getTableEntries(
        const Client &client,
        uint32_t tableId = 34173001
) {
    grpc::ClientContext ctx;
    p4::v1::ReadRequest request;
    request.set_device_id(1);
    auto *tableEntry = request.mutable_entities()->Add()->mutable_table_entry();
    tableEntry->set_table_id(tableId);
    auto reader = client->Read(&ctx, request);

    p4::v1::ReadResponse response;

    std::vector<p4::v1::TableEntry> tableEntries;

    while (reader->Read(&response)) {
        for (const auto &entity: response.entities()) {
            tableEntries.push_back(entity.table_entry());
        }
    }

    return tableEntries;
}

void printStatusError(const grpc::Status &status) {
    std::cerr << "gRPC status not ok:\n\t" << status.error_message() << "\n\n\t" << status.error_details() << std::endl;
}

grpc::Status deleteTableEntry(
        const Client &client,
        const p4::v1::TableEntry &tableEntry
) {
    grpc::ClientContext ctx;
    p4::v1::WriteRequest request;
    request.set_device_id(1);
    auto *electionId = request.mutable_election_id();
    electionId->set_low(0);
    electionId->set_high(1);

    auto *update = request.mutable_updates()->Add();
    update->set_type(p4::v1::Update_Type_DELETE);
    p4::v1::WriteResponse response;
    update->mutable_entity()->mutable_table_entry()->CopyFrom(tableEntry);
    return client->Write(&ctx, request, &response);
}

void deleteAllTableEntries(
        const Client &client,
        uint32_t tableId = 34173001
) {
    for (const auto &tableEntry: getTableEntries(client, tableId)) {
        auto status = deleteTableEntry(client, tableEntry);
        if (!status.ok()) {
            printStatusError(status);
            throw std::runtime_error("Could not delete table entry: " + tableEntry.DebugString());
        }
    }
}

void saveJSON(json &d, const std::string &filename_prefix, bool timestamped = true) {
    auto filename = filename_prefix;
    if (timestamped) {
        filename += "." + std::to_string(getTimestampLowPrecision());
    }
    filename += ".json";

    d["timestamp"] = getTimestampLowPrecision();

    std::ofstream measurementFile(filename);
    measurementFile << d.dump(4);
    measurementFile.close();
}

unsigned int getNumMeasurements(const argh::parser& cmdl) {
    unsigned int numMeasurements;
    cmdl("num_measurements", 1000) >> numMeasurements;
    return numMeasurements;
}

p4::v1::WriteRequest getTableEntryWriteRequest(
        uint32_t tableId = 34173001,
        uint32_t actionId = 24752669,
        uint32_t fieldId = 7,
        int deviceId = 1,
        int electionLow = 0,
        int electionHigh = 1
) {
    p4::v1::WriteRequest request;
    request.set_device_id(deviceId);
    auto *electionId = request.mutable_election_id();
    electionId->set_low(electionLow);
    electionId->set_high(electionHigh);

    auto *update = request.mutable_updates()->Add();
    update->set_type(p4::v1::Update_Type_INSERT);
    auto entity = update->mutable_entity();
    auto tableEntry = entity->mutable_table_entry();
    tableEntry->set_table_id(tableId);
    tableEntry->mutable_action()->mutable_action()->set_action_id(actionId);
    tableEntry->set_priority(1);
    auto match = tableEntry->mutable_match()->Add();
    match->set_field_id(fieldId);
    auto *fieldMatch = new p4::v1::FieldMatch_Ternary();

    char value[2];
    value[0] = 1;
    value[1] = 1;
    fieldMatch->set_value(value);
    fieldMatch->set_mask(value);
    match->set_allocated_ternary(fieldMatch);

    return request;
}

p4::v1::TableEntry* getTableEntry(p4::v1::WriteRequest request, int index = 0) {
    return request.mutable_updates()->at(0).mutable_entity()->mutable_table_entry();
}


#endif //P4RUNTIME_BENCHMARK_UTILS_H
