#include "utils.h"


std::vector<long> measure(
        const std::shared_ptr<p4::v1::P4Runtime::Stub> &client,
        unsigned int numMeasurements
) {
    std::vector<long> measurements;

    auto tableEntries = getTableEntries(client);

    while (measurements.size() < numMeasurements) {
        if (measurements.size() % 100 == 0) {
            std::cout << "Measurement: " << measurements.size() << "/" << numMeasurements << std::endl;
        }
        grpc::ClientContext ctx;
        p4::v1::ReadRequest request;
        request.set_device_id(DEVICE_ID);

        p4::v1::Entity *entity = request.mutable_entities()->Add();
        entity->mutable_table_entry()->CopyFrom(tableEntries.at(0));
        //entity->mutable_table_entry()->set_table_id(ENTITY_TABLE_PUNT_TABLE_ID);
        entity->mutable_table_entry()->mutable_counter_data();

        p4::v1::ReadResponse response;
        auto start = getTimestamp();
        auto reader = client->Read(&ctx, request);

        int counter = 0;
        while (reader->Read(&response)) {
            for (const auto &e: response.entities()) {
                if (e.table_entry().counter_data().byte_count() != 0) {
                    counter++;
                }
            }
        }
        reader->Finish();
        if (counter == 0) {
            std::cerr << "Could not find any counter_data. Retrying" << std::endl;
            std::this_thread::sleep_for (std::chrono::seconds(1));
            continue;
        }
        measurements.push_back(getTimestamp() - start);
    }
    return measurements;
}

int main(int argc, char *argv[]) {
    argh::parser cmdl(argc, argv);
    auto deleteAndInsertEntry = true;
    auto numMeasurements = getNumMeasurements(cmdl);

    auto client = getStub();
    if (deleteAndInsertEntry) {
        deleteAllTableEntries(client);
        grpc::ClientContext ctx;
        p4::v1::WriteResponse response;
        p4::v1::WriteRequest request = getTableEntryWriteRequest();
        auto status = client->Write(&ctx, request, &response);
        if (!status.ok()) {
            printStatusError(status);
            throw std::runtime_error("Could not write entry");
        }
    }

    auto measurements = measure(client, numMeasurements);
    json j = {
            {"measurements", measurements}
    };
    saveJSON(j, "benchmark_read");
    //deleteAllTableEntries(client);
    return 0;
}
