// GPU-accelerated Conway's Game of Life
#include <stdio.h>
#include <stdlib.h>
#define VGL_IMPL
#include "../vgl.h"

#define _printerr(fmt, ...) fprintf(stderr, fmt "%c", __VA_ARGS__)
#define panic(...) exit((_printerr(__VA_ARGS__, '\n'), 1))

const char *vert_shader =
	"#version 330 core\n"
	"layout(location = 0) in vec2 vert;\n"
	"void main() {\n"
	"	gl_Position = vec4(vert, 0, 1);\n"
	"}\n";

const char *frag_shader =
	"#version 330 core\n"
	"out vec3 color;\n"
	"uniform ivec2 size;\n"
	"uniform sampler2DRect prev;\n"
	"void main() {\n"
	"	ivec2 pos = ivec2(gl_FragCoord.xy);"
#if 1
	"	bool me;\n"
	"	int neighbours = 0;\n"
	"	// compiler pls unroll kthx\n"
	"	for (ivec2 v = ivec2(-1, -1); v.y <= 1; v.y++) {\n"
	"		for (v.x = -1; v.x <= 1; v.x++) {\n"
	"			bool val = texelFetch(prev, pos+v).r != 0.0f;\n"
	"			if (v.y == 0 && v.x == 0) me = val;\n"
	"			else neighbours += int(val);\n"
	"		}\n"
	"	}\n"
	"\n"
	"	if (me) {\n"
	"		if (neighbours < 2 || neighbours > 3) me = false;\n"
	"	} else {\n"
	"		if (neighbours == 3) me = true;\n"
	"	}\n"
	"\n"
	"	color = float(me) * vec3(1, 1, 1);\n"
#endif
	//"	color = texelFetch(prev, pos).rgb;\n"
	"}\n";

// Square when used as a triangle fan
GLfloat verts[] = {
	1, 1,
	-1, 1,
	-1, -1,
	1, -1,
};
int n_verts = 4;

int main() {
	int width = 1024, height = 1024;
	enum { SCALE = 4 };
	// MSAA must be disabled to allow scaling the draw buffer with glBlitFramebuffer
	GLFWwindow *win = vgl_init(width, height, "Conway's Game of Life", .msaa = -1);
	if (!win) panic("Window creation failed");

	// Load vertex data
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof verts, verts, GL_STATIC_DRAW);

	// Generate framebuffers and textures
	GLuint fb[2], tex[2];
	glGenFramebuffers(2, fb);
	glGenTextures(2, tex);

	struct vgl_image img = vgl_load_farbfeld("gol_start.ff");
	if (!img.data) panic("Loading start state failed");
	if (img.width != width/SCALE || img.height != height/SCALE) {
		panic("Start state image was wrong size: %dx%d", img.width, img.height);
	}

	for (int i = 0; i < 2; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, fb[i]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE, tex[i]);
		glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_SHORT, img.data);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex[i], 0);
	}

	free(img.data);

	// Load shaders
	GLuint shader = vgl_shader_source(vert_shader, -1, frag_shader, -1);
	if (!shader) panic("Shader compilation failed");

	GLuint size = glGetUniformLocation(shader, "size");
	GLuint prev = glGetUniformLocation(shader, "prev");

	// Initialize values
	int w, h;
	glfwGetFramebufferSize(win, &w, &h);

	glUseProgram(shader);
	glUniform2i(size, w, h);
	glUniform1i(prev, 0);
	vgl_perror();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, tex[1]);
	glUniform1i(prev, 0);

	glClearColor(0, 0, 0, 0);

	// Main loop
	glfwSwapInterval(1);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb[0]);
	while (!(glfwPollEvents(), glfwWindowShouldClose(win))) {
		// Blit render buffer to screen
		glViewport(0, 0, width, height);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(
			0, 0, width/SCALE, height/SCALE,
			0, 0, width, height,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb[0]);
		glViewport(0, 0, width/SCALE, height/SCALE);

		// Draw screen
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLE_FAN, 0, n_verts);
		glDisableVertexAttribArray(0);

		// Blit render buffer to back buffer
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb[1]);
		glBlitFramebuffer(
			0, 0, width/SCALE, height/SCALE,
			0, 0, width/SCALE, height/SCALE,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glfwSwapBuffers(win);
	}

	return 0;
}
