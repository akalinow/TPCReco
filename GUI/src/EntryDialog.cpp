#include <cstdlib>
#include <iostream>

#include <TGFrame.h>
#include <TGLabel.h>
#include <TGFontDialog.h>

#include <EntryDialog.h>
#include <MainFrame.h>
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EntryDialog::EntryDialog(const TGWindow* p, MainFrame* aFrame)
	: TGCompositeFrame(p, 10, 10, kVerticalFrame), theMainFrame(aFrame) {

	SetCleanup(kDeepCleanup);

	datasetInfoFrame = std::make_unique<TGHorizontalFrame>(this, 300, 100);
	fileInfoFrame = std::make_unique<TGHorizontalFrame>(this, 300, 100);
	modeInfoFrame = std::make_unique<TGHorizontalFrame>(this, 300, 100);

	TGLayoutHints* aLayoutHints = new TGLayoutHints(kLHintsTop | kLHintsLeft |
		kLHintsShrinkX | kLHintsShrinkY |
		kLHintsFillX | kLHintsFillY, 2, 2, 2, 2);

	AddFrame(modeInfoFrame.get() , new TGLayoutHints(kLHintsFillX, 2, 2, 2, 2));
	AddFrame(fileInfoFrame.get(), new TGLayoutHints(kLHintsFillX, 2, 2, 2, 2));
	AddFrame(datasetInfoFrame.get(), aLayoutHints);

	auto totalEventsFrame = std::make_unique<TGGroupFrame>(datasetInfoFrame.get(), "Events in the file:");
	auto currentEventFrame = std::make_unique<TGGroupFrame>(datasetInfoFrame.get(), "Current event:");
	auto fileNameFrame = std::make_unique<TGGroupFrame>(fileInfoFrame.get(), "Processing file:");
	auto modeFrame = std::make_unique<TGGroupFrame>(modeInfoFrame.get(), "Mode:");

	totalEventsLabel = std::make_unique<TGLabel>(totalEventsFrame.get(), "No input.");
	currentEventLabel = std::make_unique<TGLabel>(currentEventFrame.get(), "No input.");
	std::string tmp = "No input.";
	tmp.resize(fileNameLineLength, ' ');
	fileNameLabel = std::make_unique<TGLabel>(fileNameFrame.get(), tmp.c_str());

	std::string mode = "NONE";
	modeLabel = std::make_unique<TGLabel>(modeFrame.get(), mode.c_str());
	ULong_t iColor;
	gClient->GetColorByName("red", iColor);
	modeLabel->SetTextColor(iColor);

	aLayoutHints = new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5);
	totalEventsFrame->AddFrame(totalEventsLabel.get(), aLayoutHints);
	currentEventFrame->AddFrame(currentEventLabel.get(), aLayoutHints);
	fileNameFrame->AddFrame(fileNameLabel.get(), aLayoutHints);
	modeFrame->AddFrame(modeLabel.get(), aLayoutHints);

	aLayoutHints = new TGLayoutHints(kLHintsTop | kLHintsLeft, 2, 2, 1, 1);
	datasetInfoFrame->AddFrame(totalEventsFrame.get(), aLayoutHints);
	datasetInfoFrame->AddFrame(currentEventFrame.get(), aLayoutHints);
	fileInfoFrame->AddFrame(fileNameFrame.get(), aLayoutHints);
	modeInfoFrame->AddFrame(modeFrame.get(), aLayoutHints);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EntryDialog::initialize() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EntryDialog::updateEventNumbers(unsigned int nTotalEvents,
	unsigned int iCurrentEvent) {

	totalEventsLabel->SetText(Form("%u", nTotalEvents));
	currentEventLabel->SetText(Form("%u", iCurrentEvent));
	datasetInfoFrame->Layout();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EntryDialog::updateFileName(const std::string& fileName) {

	std::string fileNameWithBreaks = fileName;
	size_t previousBreakPoint = 0;
	for (size_t iPos = 0; iPos < fileNameWithBreaks.size();) {
		iPos = fileNameWithBreaks.find("/", iPos + 1);
		bool longPartFromStart = fileNameLineLength - iPos + previousBreakPoint < 10;
		if (longPartFromStart) {
			fileNameWithBreaks.insert(iPos + 1, "\n");
			previousBreakPoint = iPos;
		}
	}

	size_t iPos = fileNameWithBreaks.find_last_of("/");
	bool longLineToEnd = fileNameWithBreaks.length() - iPos > fileNameLineLength;
	if (longLineToEnd) {
		fileNameWithBreaks.insert(iPos + 1, "\n");
	}

	fileNameLabel->SetText(fileNameWithBreaks.c_str());
	fileInfoFrame->Layout();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EntryDialog::updateModeLabel(const std::string& aMode) {

	modeLabel->SetText(aMode.c_str());
	modeInfoFrame->Layout();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t EntryDialog::ProcessMessage(Long_t msg, Long_t parm1, Long_t /*parm2*/) {
	switch (GET_MSG(msg)) {
	case kC_COMMAND:
	{
		switch (GET_SUBMSG(msg)) {
		case kCM_BUTTON:
		{
			switch (parm1) {
				// exit button
			case 1:
			{
				std::cout << "EntryDialog::ProcessMessage(): msg: " << msg << std::endl;
				break;
			}
			// set button
			case 2:
			{
				break;
			}
			}
			break;
		}
		}
		break;
	}
	}
	return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
