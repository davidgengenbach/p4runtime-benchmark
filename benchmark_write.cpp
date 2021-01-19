#include <iostream>
#include <grpcpp/grpcpp.h>
#include "p4/v1/p4runtime.grpc.pb.h"
#include <ctime>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

std::string P4RUNTIME_ADDRESS = "localhost:28000";
unsigned int NUM_MEASUREMENTS = 2000;

struct Measurement {
    std::time_t timestamp;
};

std::vector<Measurement *> measurements;

using StreamChannel = std::shared_ptr<::grpc::ClientReaderWriter<
        ::p4::v1::StreamMessageRequest,
        ::p4::v1::StreamMessageResponse
>>;

std::time_t getTimestamp() {
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

void measure(std::unique_ptr<p4::v1::P4Runtime::Stub> client, unsigned int numMeasurements) {
    p4::v1::WriteResponse response;
    while (measurements.size() <= numMeasurements) {
        grpc::ClientContext ctx;
        p4::v1::WriteRequest request;
        request.set_device_id(1);
        auto *electionId = request.mutable_election_id();
        electionId->set_low(0);
        electionId->set_high(1);

        auto *update = request.mutable_updates()->Add();
        update->set_type(p4::v1::Update_Type_INSERT);
        auto entity = update->mutable_entity();
        auto tableEntry = entity->mutable_table_entry();
        tableEntry->set_table_id(34173001);
        tableEntry->mutable_action()->mutable_action()->set_action_id(24752669);
        auto match = tableEntry->mutable_match()->Add();
        match->set_field_id(7);
        auto *fieldMatch = new p4::v1::FieldMatch_Ternary();

        tableEntry->set_priority(measurements.size() + 1);

        char value[3];
        value[0] = 1;
        value[1] = 1;
        fieldMatch->set_value(value);
        fieldMatch->set_mask(value);
        match->set_allocated_ternary(fieldMatch);

        if (measurements.size() % 100 == 0) {
            std::cout << "\tMeasurement: " << measurements.size() << "/" << numMeasurements << std::endl;
        }

        auto start = getTimestamp();
        auto status = client->Write(&ctx, request, &response);
        auto end = getTimestamp();
        auto duration = end - start;

        measurements.push_back(new Measurement{
                .timestamp = duration
        });

        // Sleep a little to allow switch to process entries
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

}

void arbitrateAndMeasure(
        std::unique_ptr<p4::v1::P4Runtime::Stub> client,
        unsigned int numMeasurements,
        unsigned int deviceId = 1,
        unsigned int low = 0,
        unsigned int high = 1
) {
    grpc::ClientContext ctx;
    StreamChannel channel = std::move(client->StreamChannel(&ctx));
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
    std::cout << "Starting to listen for " << NUM_MEASUREMENTS << " packets" << std::endl;
    measure(std::move(client), numMeasurements);
    ctx.TryCancel();
    channel->Finish();
}

void saveMeasurements(const std::string &filename) {
    std::ofstream measurementFile(filename);
    if (measurementFile.is_open()) {
        for (const auto *measurement: measurements) {
            measurementFile << measurement->timestamp << "\n";
        }
        measurementFile.close();
    }
}


int main() {
    auto channel = grpc::CreateChannel(P4RUNTIME_ADDRESS, grpc::InsecureChannelCredentials());
    auto client = p4::v1::P4Runtime::NewStub(channel);

    arbitrateAndMeasure(std::move(client), NUM_MEASUREMENTS);
    saveMeasurements("benchmark_write.txt");

    return 0;
}
