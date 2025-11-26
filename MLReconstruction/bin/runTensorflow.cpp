#include <cstdlib>
#include <iostream>
#include "TPCReco/colorText.h"

#include "TPCReco/ConfigManager.h"
#include "TPCReco/EventSourceFactory.h"
#include "TPCReco/TrackBuilder.h"
#include "TPCReco/MLTrackBuilder.h"
#include "TPCReco/FileOutput.h"
#include "TPCReco/colorText.h"


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

	ConfigManager cm;
	boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

    TensorflowModel model(myConfig);
    FileOutput myFileOutput(myConfig);

	std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(myConfig);
    
	while(myEventSource->getNextEventLoop()){

        std::cout<<KRED<<"Processing event "<<myEventSource->currentEventId()<<RST<<std::endl;
        std::shared_ptr<EventTPC> currentEvent = myEventSource->getCurrentEvent();
        std::vector<float> output_data = model.reconstruct(currentEvent);
        std::cout <<KBLU<<  "Prediction output:" <<RST<< std::endl;
        std::cout <<KBLU<<  "\tvertex:" <<RST
        <<"(" << output_data[0]<<" "<< output_data[3]<<" "<< output_data[6]<<") "
        << std::endl;

        myFileOutput.update(myEventSource);
	}

	return 0;
}
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/*






        // Flatten histogram into input tensor
        auto histPtr = eventTPC.GetRawHisto();
        std::vector<float> input_tensor;
        constexpr size_t MODEL_INPUT_SIZE = 256 * 512 * 3;

        if (!histPtr) {
            std::cerr << "Warning: No histogram found. Using zero input." << std::endl;
            input_tensor.resize(MODEL_INPUT_SIZE, 0.0f);
        } else {
            int nBinsX = histPtr->GetNbinsX();
            int nBinsY = histPtr->GetNbinsY();
            int nBinsZ = histPtr->GetNbinsZ();

            for (int x = 1; x <= nBinsX; ++x)
                for (int y = 1; y <= nBinsY; ++y)
                    for (int z = 1; z <= nBinsZ; ++z)
                        input_tensor.push_back(histPtr->GetBinContent(x, y, z));

            if (input_tensor.size() < MODEL_INPUT_SIZE)
                input_tensor.resize(MODEL_INPUT_SIZE, 0.0f);
            else if (input_tensor.size() > MODEL_INPUT_SIZE)
                input_tensor.resize(MODEL_INPUT_SIZE);
        }

        //the input tensor turns out to be all zeros
        std::cout << "\n[DEBUG] Input tensor (first 100 values):\n";
        for (size_t i = 0; i < std::min(input_tensor.size(), size_t(100)); ++i) {
            std::cout << input_tensor[i] << " ";
            if ((i + 1) % 10 == 0) std::cout << std::endl;
        }
        std::cout << std::endl;

        float sum = std::accumulate(input_tensor.begin(), input_tensor.end(), 0.0f);
        float max = *std::max_element(input_tensor.begin(), input_tensor.end());
        std::cout << "[DEBUG] Input tensor sum: " << sum << ", max value: " << max << "\n" << std::endl;

        // Run inference
        std::vector<float> output_tensor = model.run(input_tensor);

        // Display results
        std::cout << "Prediction:" << std::endl;
        for (float val : output_tensor)
            std::cout << val << std::endl;

        std::cout << "Press Enter to continue or type 'exit' to break the loop: ";
        std::string user_input;
        std::getline(std::cin, user_input);
        if (user_input == "exit") break;
    }

    return 0;
}
    */
