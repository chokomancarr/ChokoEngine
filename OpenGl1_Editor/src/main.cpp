#define PLATFORM_LNX
#include "ChokoLait.h"
#include "Gromacs.h"
CHOKOLAIT_INIT_VARS;

Background* bg = new Background(IO::path + "/refl.hdr");
Font* font = new Font(IO::path + "/arimo.ttf", ALIGN_TOPLEFT);

float camz = 10;
float rw = 0, rz = 0;

Gromacs* gro;
Material* mat;
Shader* shad;
Mesh* mesh;

void updateFunc() {
	if (Input::KeyHold(Key_UpArrow)) camz -= 10*Time::delta;
	else if (Input::KeyHold(Key_DownArrow)) camz += 10*Time::delta;
	camz = max(camz, 0.0f);
	if (Input::KeyHold(Key_W)) rw -= 100 * Time::delta;
	else if (Input::KeyHold(Key_S)) rw += 100 * Time::delta;
	//camz = Clamp<float>(camz, 0.5f, 10);
	if (Input::KeyHold(Key_A)) rz -= 100 * Time::delta;
	else if (Input::KeyHold(Key_D)) rz += 100 * Time::delta;
	//camz = Clamp<float>(camz, 0.5f, 10);
	//std::cout << rw << std::endl;
	//std::cout << glfwGetKey(Display::window, GLFW_KEY_W) << " " << Input::KeyHold(Key_W) << std::endl;
}

void rendFunc() {
	MVP::Switch(false);
	MVP::Clear();
	float csz = cos(-rz*deg2rad);
	float snz = sin(-rz*deg2rad);
	float csw = cos(rw*deg2rad);
	float snw = sin(rw*deg2rad);
	Mat4x4 mMatrix = Mat4x4(1, 0, 0, 0, 0, csw, snw, 0, 0, -snw, csw, 0, 0, 0, 0, 1) * Mat4x4(csz, 0, -snz, 0, 0, 1, 0, 0, snz, 0, csz, 0, 0, 0, 0, 1);
	MVP::Mul(mMatrix);
	MVP::Translate(-gro->boundingBox.x / 2, -gro->boundingBox.y / 2, -gro->boundingBox.z / 2);
	Engine::DrawMeshInstanced(mesh, 0, mat, gro->frames[0].count);
}

uint fpsc, fps;
float told;
string s = "hello";
void paintfunc() {
	if (++fpsc == 100) {
		fps = (uint)roundf(100.0f / (Time::time - told));
		told = Time::time;
		fpsc = 0;
	}
	UI::Label(10, 10, 12, "fps: " + std::to_string(fps), font, white());
	s = UI::EditText(10, 40, 100, 16, 12, white(1, 0.4f), s, font, true, nullptr, white());
}

int main(int argc, char **argv)
{
	ChokoLait::Init(500, 500);

	auto& set = Scene::active->settings;
	set.sky = bg;
	set.skyStrength = 2;
	set.skyBrightness = 0;

	gro = new Gromacs(IO::path + "/md.gro");
	shad = new Shader(IO::GetText(IO::path + "/groV.txt"), IO::GetText(IO::path + "/instF.txt"));
	mat = new Material(shad);
	mat->SetBuffer(5, gro->ubo_positions);
	mat->SetBuffer(6, gro->ubo_colors);
	mesh = Procedurals::UVSphere(16, 8);

	while (ChokoLait::alive()) {
		ChokoLait::Update(updateFunc);
		ChokoLait::mainCamera->object->transform.localPosition(Vec3(0, 0, -camz));
		
		ChokoLait::Paint(rendFunc, paintfunc);
	}
}
