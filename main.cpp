#include <iostream>
#include <grpcpp/grpcpp.h>
#include "p4/v1/p4runtime.grpc.pb.h"
#include <ctime>
#include <iostream>
#include <fstream>

unsigned int NUM_MEASUREMENTS = 50000;

struct Measurement {
    unsigned int byte_count;
    std::time_t timestamp;
};

std::vector<Measurement*> measurements;

void arbitrate(
        std::unique_ptr<p4::v1::P4Runtime::Stub> client,
        unsigned int low,
        unsigned int high
) {
    grpc::ClientContext ctx;
    auto chan = client->StreamChannel(&ctx);
    p4::v1::StreamMessageRequest request;
    p4::v1::StreamMessageResponse response;
    request.mutable_arbitration()->set_device_id(1);
    request.mutable_arbitration()->mutable_election_id()->set_low(low);
    request.mutable_arbitration()->mutable_election_id()->set_high(high);
    chan->Write(request);

    while (true) {
        auto status = chan->Read(&response);
        if (!status || measurements.size() >= NUM_MEASUREMENTS) break;
        if (response.has_packet()) {
            auto length = response.packet().ByteSize();

            auto timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            auto measurement = new Measurement();
            measurement->byte_count = length;
            measurement->timestamp = timestamp;
            measurements.push_back(measurement);
        } else {
            response.PrintDebugString();
        }
    }

    ctx.TryCancel();
    chan->Finish();
}


int main() {
    auto server_address = "localhost:28000";
    auto channel = grpc::CreateChannel(
            server_address,
            grpc::InsecureChannelCredentials()
    );
    auto client = p4::v1::P4Runtime::NewStub(channel);
    arbitrate(std::move(client), 0, 1);

    std::ofstream measurementFile ("measurement.txt");
    if (measurementFile.is_open()) {
        for(const auto* measurement: measurements) {
            measurementFile << measurement->timestamp << "," << measurement->byte_count << "\n";
        }
        measurementFile.close();
    }

    std::cout << measurements.size() << std::endl;
    return 0;
}
