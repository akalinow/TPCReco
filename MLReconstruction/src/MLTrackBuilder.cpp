#include <iostream>
#include <boost/property_tree/json_parser.hpp>

#include "TPCReco/MLTrackBuilder.h"
#include "TPCReco/tf_functions.h"
#include "TPCReco/EventTPC.h"



TensorflowModel::~TensorflowModel(){}
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
TensorflowModel::TensorflowModel(const boost::property_tree::ptree& aConfig)
    : graph(nullptr), session(nullptr), myConfig(aConfig), output_lenght(1)
{
    const std::string model_path = myConfig.get<std::string>("reconstruction.tfModelPath");
    const char* model_path_cstr = model_path.c_str();

    // Parse the input dimensions
    int totalSize = 1;
    for (const auto& item : myConfig.get_child("reconstruction.inputDim")) {
        inputDim.push_back(item.second.get_value<std::int64_t>());
        totalSize *= inputDim.back();
    }
    input_tensor.resize(totalSize);


    // Parse the output dimensions
    for (const auto& item : myConfig.get_child("reconstruction.outputDim")) {
        outputDim.push_back(item.second.get_value<std::int64_t>());
        output_lenght *= item.second.get_value<std::int64_t>();
    }

    // Load the TensorFlow model session.
    tf_functions::load_session(model_path_cstr, &graph, &session);

    // Initialize the input operation.
    TF_Operation* input_op = TF_GraphOperationByName(graph, "serving_default_input_image");
    input = TF_Output{input_op, 0};
    if (input.oper == nullptr) {
        std::cerr << "Can't init input_op" << std::endl;
    } else {
        std::cout << "Initialized input_op" << std::endl;
    }

    // Initialize the output operation.
    TF_Operation* output_op = TF_GraphOperationByName(graph, "StatefulPartitionedCall_1");
    output = TF_Output{output_op, 0};
    if (output.oper == nullptr) {
        std::cerr << "Can't init output_op" << std::endl;
    } else {
        std::cout << "Initialized output_op" << std::endl;
    }
}
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void TensorflowModel::fillMLInput(std::shared_ptr<EventTPC> eventTPC){

    std::vector<std::shared_ptr<TH2D> > rawHits;
    double max = 1.0;
    for(int iDir=definitions::projection_type::DIR_U;iDir<=definitions::projection_type::DIR_W;++iDir){
      std::shared_ptr<TH2D> hRawHits = eventTPC->get2DProjection(get2DProjectionType(iDir),
							   filter_type::none, scale_type::raw);

        max = std::max(max, hRawHits->GetMaximum());
        rawHits.push_back(hRawHits); 
    }

        double val = 0.0;
        int index = 0;
        for (int iBinY = 0; iBinY <= inputDim[1]; ++iBinY){
             for(int iBinX = 1; iBinX <= inputDim[2]; ++iBinX){
                for(auto hRawHits:  rawHits){
                if (iBinX<=hRawHits->GetNbinsX() && iBinY<=hRawHits->GetNbinsY()){
                    val = hRawHits->GetBinContent(iBinX, iBinY) / max;
                }
                else val = 0.0;
                //input_tensor.push_back(val);
                input_tensor[index++] = val;
            }
            }
        }
}
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
std::vector<float> TensorflowModel::runML(const std::vector<float> & input_data)
{    

    // Create the input tensor using the stored dimensions.
    TF_Tensor* input_tensor = nullptr;
    tf_functions::create_tensor(TF_FLOAT, inputDim, inputDim.size(), input_data, &input_tensor);


    // Run the session.
    TF_Tensor* output_tensor = nullptr;
    tf_functions::run_session(
        session,
        &input, &input_tensor, 1,
        &output, &output_tensor, 1
    );
    // Retrieve the results from the output tensor.
    float* tensor_data = static_cast<float*>(TF_TensorData(output_tensor));

    std::vector<float> results;

    results.reserve(output_lenght);

    for (std::int64_t i = 0; i < output_lenght; i++) {
        results.push_back(tensor_data[i]);
    }

    // Clean up the temporary tensors.
    tf_functions::delete_tensor(input_tensor);
    tf_functions::delete_tensor(output_tensor);

    return results;
}
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
std::vector<float> TensorflowModel::reconstruct(std::shared_ptr<EventTPC> aEventTPC){

    fillMLInput(aEventTPC);
    return runML(this->input_tensor);

}
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////