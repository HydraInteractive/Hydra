#include <barcode/exportermenu.hpp>
#include <hydra/component/roomcomponent.hpp>

using world = Hydra::World::World; 
ExporterMenu::ExporterMenu() : FileTree()
{
	this->_extWhitelist = { ".room", ".ROOM" };
	refresh("/assets");
}
ExporterMenu::~ExporterMenu()
{
	
}
void ExporterMenu::render(bool &openBool, Hydra::Renderer::Batch* previewBatch, float delta)
{
	ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiSetCond_Once);
	ImGui::Begin("Export", &openBool, ImGuiWindowFlags_MenuBar);
	_menuBar();

	Node* selectedNode = nullptr;
	bool doubleClicked = false;
	//File tree
	ImGui::BeginChild("Browser", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, ImGui::GetWindowContentRegionMax().y - 60));
	if (_root != nullptr)
		_root->render(&selectedNode, doubleClicked);
	ImGui::EndChild();

	ImGui::SameLine();

	//File name dialog
	ImGui::BeginChild("File", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, ImGui::GetWindowContentRegionMax().y - 60));

	//Refresh changes from file tree
	if (selectedNode != nullptr)
	{
		if (ImGui::IsMouseDoubleClicked(0) && selectedNode->getExt() == ".room")
		{
			_prepExporting = true;
		}
		if (selectedNode->openInFileExplorer)
		{
			openInExplorer(selectedNode->reverseEngineerPath());
			selectedNode->openInFileExplorer = false;
		}
		_selectedPath = selectedNode->reverseEngineerPath();
		if (selectedNode->isAllowedFile)
		{
			_selectedPath.erase(_selectedPath.end() - selectedNode->name().length(), _selectedPath.end());

			std::string fileNameWithoutExt = selectedNode->name();
			fileNameWithoutExt.erase(fileNameWithoutExt.end() - selectedNode->getExt().length(), fileNameWithoutExt.end());
			//Force empty the string before copying the new one
			memset(_selectedFileName, 0, sizeof(_selectedFileName));
			strncpy(_selectedFileName, fileNameWithoutExt.c_str(), fileNameWithoutExt.length());
		}
		else
		{
			_selectedPath.append("/");
		}
	}

	//Draw gui
	ImGui::Text("Path: "); ImGui::SameLine(); ImGui::Text("%s", _selectedPath.c_str());
	ImGui::InputText(".room", _selectedFileName, 128);

	if (ImGui::Button("Save"))
	{
		_prepExporting = true;
	}
	if (_prepExporting)
	{
		std::string fileToSave = "";
		fileToSave.append(_selectedPath);
		fileToSave.append(_selectedFileName);
		fileToSave.append(".room");
		std::ifstream infile(fileToSave);
		bool doExport = false;
		if (infile.good())
		{
			ImGui::OpenPopup("Overwrite?");
		}
		else
		{
			doExport = true;
		}

		if (ImGui::BeginPopupModal("Overwrite?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			std::string textline = "This action will overwrite the file at " + fileToSave + ". Continue?";
			ImGui::Text("%s", textline.c_str());
			ImGui::Separator();

			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				doExport = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{	
				_prepExporting = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (doExport)
		{
			std::ofstream outFile;
			if (getRoomEntity() != nullptr)
				BlueprintLoader::save(fileToSave, "Room", getRoomEntity());
			_prepExporting = false;
			openBool = false;
		}
	}
	ImGui::EndChild();
	ImGui::End();
}
