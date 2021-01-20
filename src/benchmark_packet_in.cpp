#include "utils.h"

struct Measurement {
    int byte_count;
    int timestamp;
};

std::vector<json> measurements;

void measure(const StreamChannel &channel, unsigned int numMeasurements) {
    p4::v1::StreamMessageResponse response;
    while (measurements.size() <= numMeasurements) {
        if (measurements.size() % 100 == 0) {
            std::cout << "\tMeasurement: " << measurements.size() << "/" << numMeasurements << std::endl;
        }
        auto status = channel->Read(&response);
        if (!status) {
            response.PrintDebugString();
            throw std::runtime_error("status == false ... why?");
        }
        if (response.has_packet()) {
            measurements.push_back({
                                           {"byte_count", response.packet().ByteSize()},
                                           {"timestamp",  getTimestamp()}
                                   });
        } else {
            response.PrintDebugString();
        }
    }
}

int main(int argc, char *argv[]) {
    argh::parser cmdl(argc, argv);
    auto numMeasurements = getNumMeasurements(cmdl);

    auto client = getStub();
    measure(streamChannel, numMeasurements);
    json j = {{"measurements", measurements}};
    saveJSON(j, "benchmark_packet_in");
    return 0;
}
