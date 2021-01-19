#include "utils.h"

struct Measurement {
    int byte_count;
    std::time_t timestamp;

    std::string line() const {
        return std::to_string(timestamp) + "," + std::to_string(byte_count);
    }
};

std::vector<Measurement *> measurements;

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
            measurements.push_back(new Measurement{
                    .byte_count = response.packet().ByteSize(),
                    .timestamp = getTimestamp()
            });
        } else {
            response.PrintDebugString();
        }
    }
}

int main() {
    auto client = getStub();
    measure(streamChannel, 10000);
    saveTimestamps("benchmark_packet_in.txt", measurements, "timestamp,byte_count");
    return 0;
}
