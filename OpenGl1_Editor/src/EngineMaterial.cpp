#include "Engine.h"
#include "EngineMaterial.h"
#include <gl/glew.h>
#include <string>

Material::Material() {
	shader = Engine::unlitProgram;
}

Material::Material(Shader * shad) {
	shader = shad->pointer;
}

void Material::SetSampler(string name, Texture * texture) {
	SetSampler(name, texture, 0);
}
void Material::SetSampler(string name, Texture * texture, int id) {
	GLint imageLoc = glGetUniformLocation(shader, name.c_str());
	glUseProgram(shader);

	glUniform1i(imageLoc, 0);
	glActiveTexture(GL_TEXTURE0+id);
	glBindTexture(GL_TEXTURE_2D, texture->pointer);
}
