#ifndef EntryDialog_H
#define EntryDialog_H

#include <string>

#include <TGFrame.h>
#include <TGLayout.h>
#include <TGLabel.h>

class MainFrame;

class EntryDialog : public TGCompositeFrame {

public:
	EntryDialog(const TGWindow* p, MainFrame* aFrame);

	void initialize();

	virtual ~EntryDialog() = default;

	virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);

	void updateFileName(const std::string& fileName);

	void updateEventNumbers(unsigned int nTotalEvents, unsigned int iCurrentEvent);

	void updateModeLabel(const std::string& aMode);

private:

	MainFrame* theMainFrame;

	std::unique_ptr<TGHorizontalFrame> datasetInfoFrame;
	std::unique_ptr<TGHorizontalFrame> fileInfoFrame;
	std::unique_ptr<TGHorizontalFrame> modeInfoFrame;

	std::unique_ptr<TGLabel> totalEventsLabel;
	std::unique_ptr<TGLabel> currentEventLabel;
	std::unique_ptr<TGLabel> fileNameLabel;
	std::unique_ptr<TGLabel> modeLabel;

	unsigned int fileNameLineLength = 36;

};

#endif
