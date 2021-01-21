#include "utils.h"


std::vector<long> measure(
        const std::shared_ptr<p4::v1::P4Runtime::Stub> &client,
        unsigned int numMeasurements,
        bool priorityLowToHigh
) {
    std::vector<long> measurements;
    p4::v1::WriteResponse response;

    p4::v1::WriteRequest request = getTableEntryWriteRequest();
    auto *tableEntry = getTableEntry(request);

    while (measurements.size() < numMeasurements) {
        if (measurements.size() % 100 == 0) {
            std::cout << "Measurement: " << measurements.size() << "/" << numMeasurements << std::endl;
        }
        grpc::ClientContext ctx;

        auto priority = priorityLowToHigh ? measurements.size() + 1 : numMeasurements - measurements.size() + 1;
        tableEntry->set_priority(priority);
        auto start = getTimestamp();
        auto status = client->Write(&ctx, request, &response);
        if (!status.ok()) {
            printStatusError(status);
            throw std::runtime_error("Could not write entry. Stopping");
        }
        measurements.push_back(getTimestamp() - start);

        // Sleep a little to allow switch to process entries
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return measurements;
}

int main(int argc, char *argv[]) {
    argh::parser cmdl(argc, argv);
    auto numMeasurements = getNumMeasurements(cmdl);

    auto client = getStub();
    for (bool priorityLowToHigh : {false, true}) {
        deleteAllTableEntries(client);
        auto measurements = measure(client, numMeasurements, priorityLowToHigh);
        json j = {
                {"priorityLowToHigh", priorityLowToHigh},
                {"measurements",      measurements}
        };
        saveJSON(j, "benchmark_write." + std::string(priorityLowToHigh ? "low_to_high" : "high_to_low"));
    }
    deleteAllTableEntries(client);
    return 0;
}
