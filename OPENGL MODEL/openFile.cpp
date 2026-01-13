// helper function to help open window file explorer

#define NOMINMAX	// prevent windows from breaking std::min/max
#include <Windows.h>
#include <commdlg.h>
#include <string>


std::string openFileDialog()
{
	OPENFILENAMEA ofn{};	// common dialog box structure
	char szFile[260]{ 0 };

	// initialize OPENFILENAME	
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	// pass glfwGetWin32Window if you want it to block your window
	ofn.hwndOwner = nullptr;		
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "3D Models\0*.obj;*.gltf;*.fbx\0All\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileNameA(&ofn) == TRUE)
	{
		return std::string(ofn.lpstrFile);
	}

	return std::string();	// return empty if cancelled
}