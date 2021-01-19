#include "utils.h"

struct Measurement {
    std::time_t timestamp;

    std::string line() const {
        return std::to_string(timestamp);
    }
};

std::vector<Measurement *> measurements;

void measure(
        const std::shared_ptr<p4::v1::P4Runtime::Stub> &client,
        unsigned int numMeasurements,
        uint32_t tableId = 34173001,
        uint32_t actionId = 24752669,
        uint32_t fieldId = 7,
        int deviceId = 1,
        int electionLow = 0,
        int electionHigh = 1
) {
    p4::v1::WriteResponse response;

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
    auto match = tableEntry->mutable_match()->Add();
    match->set_field_id(fieldId);
    auto *fieldMatch = new p4::v1::FieldMatch_Ternary();

    char value[3];
    value[0] = 1;
    value[1] = 1;
    fieldMatch->set_value(value);
    fieldMatch->set_mask(value);
    match->set_allocated_ternary(fieldMatch);

    while (measurements.size() <= numMeasurements) {
        if (measurements.size() % 100 == 0) {
            std::cout << "Measurement: " << measurements.size() << "/" << numMeasurements << std::endl;
        }
        grpc::ClientContext ctx;
        tableEntry->set_priority(measurements.size() + 1);
        auto start = getTimestamp();
        auto status = client->Write(&ctx, request, &response);
        if (!status.ok()) {
            printStatusError(status);
            throw std::runtime_error("Could not write entry. Stopping");
        }
        measurements.push_back(new Measurement{
                .timestamp = getTimestamp() - start
        });

        // Sleep a little to allow switch to process entries
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    auto client = getStub();
    deleteAllTableEntries(client);
    measure(client, 1000);
    deleteAllTableEntries(client);
    saveTimestamps("benchmark_write.txt", measurements, "duration_in_nanoseconds");
    return 0;
}
