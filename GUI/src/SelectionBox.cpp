#include <SelectionBox.h>
#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void SelectionBox::DoSelect(Long_t msg) {
	fSelected->Clear();
	fListBox->GetSelectedEntries(fSelected.get());

	Emit("DoSelect(Long_t)", fSelected.get());
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void SelectionBox::DoExit() {
	fMain->CloseWindow();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
SelectionBox::SelectionBox(const TGWindow* p, TGWindow* main, UInt_t w,
	UInt_t h, UInt_t options) {

	fMain = std::make_unique<TGTransientFrame>(p, main, w, h, options);
	fMain->Connect("CloseWindow()", "SelectionBox", this, "DoExit()");
	fMain->DontCallClose(); // to avoid double deletions.
	fMain->SetCleanup(kDeepCleanup);
	fMain->Resize(300, 500);

	fFrame = std::make_unique<TGHorizontalFrame>(fMain.get(), 60, 20, kFixedWidth);
	fListBox = std::make_unique<TGListBox>(fFrame.get(), 89);
	fListBox->SetMultipleSelections(true);
	fListBox->Resize(300, 500);
	fSelected = std::make_unique<TList>();

	fFrame->AddFrame(fListBox.get(), new TGLayoutHints(kLHintsTop | kLHintsLeft |
		kLHintsExpandX | kLHintsExpandY,
		5, 5, 5, 5));

	auto hframe = std::make_unique<TGHorizontalFrame>(fMain.get(), 250, 20, kFixedWidth);
	TGTextButton* show = new TGTextButton(hframe.get(), "&Set");
	show->Connect("Pressed()", "SelectionBox", this, "DoSelect()");
	hframe->AddFrame(show, new TGLayoutHints(kLHintsExpandX, 5, 5, 3, 4));
	TGTextButton* exit = new TGTextButton(hframe.get(), "&Exit ");
	exit->Connect("Pressed()", "SelectionBox", this, "DoExit()");
	hframe->AddFrame(exit, new TGLayoutHints(kLHintsExpandX, 5, 5, 3, 4));
	fMain->AddFrame(hframe.get(), new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 2, 2, 5, 1));

	// Set a name to the main frame
   // position relative to the parent's window
	TGLayoutHints* fL1 = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX,
		2, 2, 2, 2);

	fMain->AddFrame(fFrame.get(), fL1);
	fMain->CenterOnParent();
	fMain->SetWindowName("Histogram selection");
	fMain->MapWindow();
	fMain->MapSubwindows();
	fMain->Resize();

	Connect("DoSelect(Long_t)", "MainFrame", main, "HandleHistoSelect(Long_t)");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
SelectionBox::~SelectionBox() {
	fMain->DeleteWindow();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void SelectionBox::HandleButtons()
{

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void SelectionBox::Initialize(const std::vector<std::string>& hNames) {

	for (unsigned int iHisto = 0; iHisto < hNames.size(); ++iHisto) {
		fListBox->AddEntry(hNames[iHisto].c_str(), iHisto + 1);
	}
	fListBox->Select(1);
	fMain->Layout();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
