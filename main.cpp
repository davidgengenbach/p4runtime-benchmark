#include <iostream>
#include <grpcpp/grpcpp.h>
#include "p4/v1/p4runtime.grpc.pb.h"
#include <ctime>
#include <iostream>
#include <fstream>

std::string P4RUNTIME_ADDRESS = "localhost:28000";
unsigned int NUM_MEASUREMENTS = 50000;

struct Measurement {
    int byte_count;
    std::time_t timestamp;
};

std::vector<Measurement *> measurements;

using StreamChannel = std::shared_ptr<::grpc::ClientReaderWriter<
        ::p4::v1::StreamMessageRequest,
        ::p4::v1::StreamMessageResponse
>>;

int getTimestamp() {
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

void measure(const StreamChannel &channel, unsigned int numMeasurements) {
    p4::v1::StreamMessageResponse response;
    while (measurements.size() <= numMeasurements) {
        if (measurements.size() % 100 == 0) {
            std::cout << "\tMeasurement: " << measurements.size() << "/" << numMeasurements << std::endl;
        }
        auto status = channel->Read(&response);
        if (!status) {
            throw std::runtime_error("status == false ... why?");
        }
        if (response.has_packet()) {
            response.PrintDebugString();
            measurements.push_back(new Measurement{
                    .byte_count = response.packet().ByteSize(),
                    .timestamp = getTimestamp()
            });
        } else {
            response.PrintDebugString();
        }
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
    measure(channel, numMeasurements);
    ctx.TryCancel();
    channel->Finish();
}

void saveMeasurements(const std::string &filename) {
    std::ofstream measurementFile(filename);
    if (measurementFile.is_open()) {
        for (const auto *measurement: measurements) {
            measurementFile << measurement->timestamp << "," << measurement->byte_count << "\n";
        }
        measurementFile.close();
    }
}


int main() {
    auto channel = grpc::CreateChannel(P4RUNTIME_ADDRESS, grpc::InsecureChannelCredentials());
    auto client = p4::v1::P4Runtime::NewStub(channel);

    arbitrateAndMeasure(std::move(client), NUM_MEASUREMENTS);
    saveMeasurements("measurement.txt");

    return 0;
}
