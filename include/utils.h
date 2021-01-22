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


auto DEVICE_ID = 1;
auto ENTITY_TABLE_PUNT_TABLE_ID = 34173001;
auto ENTITY_MATCH_FIELD_ID = 7;
auto ENTITY_ACTION_ID = 24752669;

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

void printStatusError(const grpc::Status &status) {
    std::cerr << "gRPC status not ok:\n\t" << status.error_message() << "\n\n\t" << status.error_details() << std::endl;
}

StreamChannel arbitrate(
        const std::shared_ptr<p4::v1::P4Runtime::Stub> &client,
        unsigned int deviceId = DEVICE_ID,
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
        response.PrintDebugString();
        throw std::runtime_error("Could not arbitrate");
    }

    return StreamChannel(std::move(channel));
}

std::shared_ptr<p4::v1::P4Runtime::Stub> getStub(const std::string &address = "localhost:28000") {
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto client = std::shared_ptr<p4::v1::P4Runtime::Stub>(std::move(p4::v1::P4Runtime::NewStub(channel)));
    streamChannel = arbitrate(client);
    return client;
}

std::vector<p4::v1::TableEntry> getTableEntries(
        const Client &client,
        uint32_t tableId = ENTITY_TABLE_PUNT_TABLE_ID,
        uint32_t deviceId = DEVICE_ID
) {
    grpc::ClientContext ctx;
    p4::v1::ReadRequest request;
    request.set_device_id(deviceId);
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


void deleteAllTableEntries(
        const Client &client,
        uint32_t tableId = ENTITY_TABLE_PUNT_TABLE_ID,
        unsigned int deviceId = DEVICE_ID
) {
    grpc::ClientContext ctx;
    p4::v1::WriteRequest request;
    request.set_device_id(deviceId);
    auto *electionId = request.mutable_election_id();
    electionId->set_low(0);
    electionId->set_high(1);
    p4::v1::WriteResponse response;

    for (const auto &tableEntry: getTableEntries(client, tableId)) {
        auto *update = request.mutable_updates()->Add();
        update->set_type(p4::v1::Update_Type_DELETE);
        update->mutable_entity()->mutable_table_entry()->CopyFrom(tableEntry);
    }

    auto status = client->Write(&ctx, request, &response);
    if (!status.ok()) {
        printStatusError(status);
        response.PrintDebugString();
        throw std::runtime_error("Could not delete table entries");
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

unsigned int getNumMeasurements(const argh::parser &cmdl) {
    unsigned int numMeasurements;
    cmdl("num_measurements", 1000) >> numMeasurements;
    return numMeasurements;
}

p4::v1::WriteRequest getTableEntryWriteRequest(
        // "SwitchIngress.punt_table"
        uint32_t tableId = ENTITY_TABLE_PUNT_TABLE_ID,
        // "SwitchIngress.set_egress_port_static"
        uint32_t actionId = ENTITY_ACTION_ID,
        // "ig_intr_md.ingress_port"
        uint32_t fieldId = ENTITY_MATCH_FIELD_ID,
        int deviceId = DEVICE_ID,
        int electionLow = 0,
        int electionHigh = 1
) {
    p4::v1::WriteRequest request;
    request.set_device_id(deviceId);
    auto *electionId = request.mutable_election_id();
    electionId->set_low(electionLow);
    electionId->set_high(electionHigh);

    unsigned int port = 0;

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

    unsigned char value[2] = {0, static_cast<unsigned char>(port)};
    unsigned char mask[2] = {0, 255};
    auto size = 2;
    fieldMatch->set_value(static_cast<char *>(static_cast<void *>(value)), size);
    fieldMatch->set_mask(static_cast<char *>(static_cast<void *>(mask)), size);
    match->set_allocated_ternary(fieldMatch);

    return request;
}

p4::v1::TableEntry *getTableEntry(p4::v1::WriteRequest request, int index = 0) {
    return request.mutable_updates()->Mutable(index)->mutable_entity()->mutable_table_entry();
}


#endif //P4RUNTIME_BENCHMARK_UTILS_H
