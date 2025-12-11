#ifndef TENSORFLOWMODEL_H
#define TENSORFLOWMODEL_H

#include <vector>
#include <cstdint>
#include <tensorflow/c/c_api.h>
#include <boost/property_tree/ptree.hpp>

#include <TVector3.h>

class EventTPC;
class TrackSegment3D;

class TensorflowModel {
public:

    TensorflowModel(const boost::property_tree::ptree& aConfig);

    ~TensorflowModel();

    void run(std::shared_ptr<EventTPC> aEventTPC);
    TVector3 getBias() const { return aBias; }
    TVector3 getTangent() const { return aTangent; }

    TVector3 getVertex() const {  return aVertex;}
    TVector3 getAlphaEnd() const { return aAlphaEnd; }
    TVector3 getCarbonEnd() const { return aCarbonEnd; }

    bool isValid() const {
        return (graph != nullptr) && (session != nullptr);
    }

private:

    void fillMLInput(std::shared_ptr<EventTPC> eventTPC);

    void  runML(const std::vector<float>& input_data);

    void convertOutput();

    TF_Graph* graph;
    TF_Session* session;
    TF_Output input;
    TF_Output output;
	boost::property_tree::ptree myConfig;
    std::vector<std::int64_t> inputDim = {};
    std::vector<std::int64_t> outputDim = {};
    std::int64_t output_lenght;
    std::vector<float> inputVec, outputVec;

    TVector3 aVertex, aAlphaEnd, aCarbonEnd;
    TVector3 aBias, aTangent;
};

#endif // TENSORFLOWMODEL_H