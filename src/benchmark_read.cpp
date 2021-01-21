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
        request.set_device_id(1);

        p4::v1::Entity *entity = request.mutable_entities()->Add();

        auto *tableEntry = entity->mutable_table_entry();
        tableEntry->set_table_id(34173001);
        tableEntry->mutable_counter_data()->set_byte_count(0);
        tableEntry->mutable_counter_data()->set_packet_count(0);

        //auto &tableEntry = tableEntries.at(0);
        //entity->mutable_table_entry()->CopyFrom(tableEntry);
        //entity->mutable_table_entry()->mutable_counter_data()->set_byte_count(1);
        //entity->mutable_table_entry()->mutable_counter_data()->set_packet_count(1);

        //auto *directCounter = entity->mutable_direct_counter_entry();
        //directCounter->mutable_table_entry()->set_table_id(0);


        p4::v1::ReadResponse response;

        auto start = getTimestamp();
        auto reader = client->Read(&ctx, request);

        int counter = 0;
        while (reader->Read(&response)) {
            if (response.entities_size() != 0) {
                counter++;
            }
        }
        if (counter == 0) {
            throw std::runtime_error("No table entries read. Exiting");
        }
        measurements.push_back(getTimestamp() - start);
    }
    return measurements;
}

int main(int argc, char *argv[]) {
    argh::parser cmdl(argc, argv);
    auto numMeasurements = getNumMeasurements(cmdl);

    auto client = getStub();
    deleteAllTableEntries(client);

    if (true) {
        grpc::ClientContext ctx;
        p4::v1::WriteResponse response;
        p4::v1::WriteRequest request = getTableEntryWriteRequest();
        auto status = client->Write(&ctx, request, &response);
        if (!status.ok()) {
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
