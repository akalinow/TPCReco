#ifndef TENSORFLOWMODEL_H
#define TENSORFLOWMODEL_H

#include <vector>
#include <cstdint>
#include <tensorflow/c/c_api.h>
#include <boost/property_tree/ptree.hpp>

class EventTPC;

class TensorflowModel {
public:

    TensorflowModel(const boost::property_tree::ptree& aConfig);

    ~TensorflowModel();

    std::vector<float> reconstruct(std::shared_ptr<EventTPC>);

private:

    void fillMLInput(std::shared_ptr<EventTPC> eventTPC);

    std::vector<float>  runML(const std::vector<float>& input_data);

    TF_Graph* graph;
    TF_Session* session;
    TF_Output input;
    TF_Output output;
	boost::property_tree::ptree myConfig;
    std::vector<std::int64_t> inputDim = {};
    std::vector<std::int64_t> outputDim = {};
    std::int64_t output_lenght;
    mutable std::vector<float> input_tensor;
};

#endif // TENSORFLOWMODEL_H