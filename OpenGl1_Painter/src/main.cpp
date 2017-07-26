#include <iostream>
#include <Windows.h>
#include <gl/GLUT.h>

using namespace std;

class Win {
public:
	static string OpenFile(string filter) {
		OPENFILENAME ofn;       // common dialog box structure
		char szFile[260];       // buffer for file name
		//HWND hwnd;              // owner window
		//HANDLE hf;              // file handle
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = GetForegroundWindow();
		ofn.lpstrTitle = "Select Model File";
		ofn.lpstrFile = szFile;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		//ofn.lpstrFilter = &filter[0];
		//ofn.nFilterIndex = 0;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		if (GetOpenFileName(&ofn) != 0)
			return string (ofn.lpstrFile);
		else return "";
	}
};

int main(int argc, char **argv)
{
	Win::OpenFile("Model file\0*.obj\0\0");
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(1366, 768);
	glutCreateWindow("Painter (loading...)");
}