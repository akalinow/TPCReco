#include <iostream>
#include <boost/property_tree/json_parser.hpp>

#include "TPCReco/MLTrackBuilder.h"
#include "TPCReco/tf_functions.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/TrackSegment3D.h"
#include "TPCReco/colorText.h"



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
    inputVec.resize(totalSize);


    // Parse the output dimensions
    for (const auto& item : myConfig.get_child("reconstruction.outputDim")) {
        outputDim.push_back(item.second.get_value<std::int64_t>());
        output_lenght *= item.second.get_value<std::int64_t>();
    }
      outputVec.resize(output_lenght);

    // Load the TensorFlow model session.
    int status = tf_functions::load_session(model_path_cstr, &graph, &session);
    if (status != 0) {
        std::cout <<KRED<< "Error loading TensorFlow model from " <<RST<< model_path << std::endl;
        return;
    }

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
                inputVec[index++] = val;
            }
            }
        }
}
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void TensorflowModel::runML(const std::vector<float> & input_data){    

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

    for (std::int64_t i = 0; i < output_lenght; i++) {
        outputVec[i] = tensor_data[i];
    }

    // Clean up the temporary tensors.
    tf_functions::delete_tensor(input_tensor);
    tf_functions::delete_tensor(output_tensor);
}
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void TensorflowModel::convertOutput(){

    double offset = 0;
    aVertex.SetXYZ(outputVec[0], outputVec[3], outputVec[6]+offset);
    aAlphaEnd.SetXYZ(outputVec[1], outputVec[4], outputVec[7]+offset);
    aCarbonEnd.SetXYZ(outputVec[2], outputVec[5], outputVec[8]+offset);

    aTangent = (aAlphaEnd - aVertex).Unit();
    aBias = aVertex;
}
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void TensorflowModel::run(std::shared_ptr<EventTPC> aEventTPC) {

  if(!isValid()) {
        std::cout <<KRED<< "TensorFlow model is not valid." << RST << std::endl;
        return;
    }

    fillMLInput(aEventTPC);
    runML(inputVec);
    convertOutput();
}
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////