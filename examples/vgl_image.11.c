// GPU-accelerated image viewer
#include "glad/glad.h"
#define VGL_IMPL
#include "../vgl.h"

const char *vert_shader =
	"#version 330 core\n"
	"layout(location = 0) in vec2 vert;\n"
	"void main() {\n"
	"	gl_Position = vec4(vert, 0, 1);\n"
	"}\n";

const char *frag_shader =
	"#version 330 core\n"
	"layout(origin_upper_left) in vec4 gl_FragCoord;\n"
	"out vec4 color;\n"
	"uniform sampler2DRect tex;\n"
	"void main() {\n"
	"	color = texture(tex, gl_FragCoord.xy);\n"
	"}\n";

int main(int argc, char **argv) {
	const char *filename = "test.ff";
	if (argc > 1) filename = argv[1];

	int width = 1024, height = 768;
	GLFWwindow *win = vgl_init(width, height, "Image Viewer");
	if (!win) {
		fprintf(stderr, "Error creating GLFW window\n");
		return 1;
	}

	// Load vertices
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	GLfloat verts[] = {
		1, 1,
		1, -1,
		-1, 1,
		-1, -1
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof verts, verts, GL_STATIC_DRAW);

	// Load texture
	GLuint tex;
	glGenTextures(1, &tex);
	struct vgl_image img = vgl_load_farbfeld(filename);
	if (!img.data) {
		fprintf(stderr, "Error opening image\n");
		return 1;
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, tex);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_SHORT, img.data);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// Load shaders
	GLuint shader = vgl_shader_source(vert_shader, -1, frag_shader, -1);
	if (!shader) {
		fprintf(stderr, "Error opening image\n");
		return 1;
	}
	glUseProgram(shader);
	glUniform1i(glGetUniformLocation(shader, "tex"), 0);

	glClearColor(0, 0, 0, 0);
	while (!(glfwPollEvents(), glfwWindowShouldClose(win))) {
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableVertexAttribArray(0);

		glfwSwapBuffers(win);
	}

	return 0;
}
