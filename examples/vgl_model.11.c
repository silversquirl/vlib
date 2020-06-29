#define VGL_IMPL
#include "../vgl.h"

#define _printerr(fmt, ...) fprintf(stderr, fmt "%c", __VA_ARGS__)
#define panic(...) exit((_printerr(__VA_ARGS__, '\n'), 1))

const char *vert_shader =
	"#version 330 core\n"
	"layout(location = 0) in vec3 vert;\n"
	"out vec3 v_color;\n"
	"uniform mat4 mvp;\n"
	"\n"
	"vec3 rand(vec3 seed) {\n"
	"	// float -> int\n"
	"	ivec3 iseed = floatBitsToInt(seed);\n"
	"	// sign+mag -> 1's compl\n"
	"	iseed ^= ~0 * (iseed >> 31);\n"
	"	// LCG\n"
	"	iseed = (0x38ECD75*iseed + 0xD) & ((1<<24) - 1);\n"
	"	// Convert back to float\n"
	"	return vec3(iseed) / (1<<24);\n"
	"}\n"
	"\n"
	"void main() {\n"
	"	gl_Position = mvp * vec4(vert, 1);\n"
	"	v_color = sqrt(rand(vert));\n"
	"}\n";

const char *frag_shader =
	"#version 330 core\n"
	"in vec3 v_color;\n"
	"out vec3 color;\n"
	"void main() {\n"
	"	color = v_color;\n"
	"}\n";

int main(int argc, char *argv[]) {
	const char *modelfn = "vgl_data/cube.vmsh";
	if (argc >= 2) modelfn = argv[1];

	int width = 1280, height = 1024;
	GLFWwindow *win = vgl_init(width, height, "Model loading example");
	if (!win) panic("Window creation failed");

	// Create VAO
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Load mesh
	struct vgl_mesh *mesh = vgl_load_vmesh(modelfn);
	if (!mesh) panic("Mesh loading failed: %s", vgl_mesherr);
	struct vgl_gpu_mesh gmesh = vgl_mesh_upload(mesh);
	vgl_mesh_del(mesh);
	struct vgl_gpu_layout layout = {0, -1, -1};

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	GLfloat verts[] = {
		-1, -1, 0,
		1, -1, 0,
		0, 1, 0,
	};
	glBufferData(GL_ARRAY_BUFFER, 3*3*sizeof (*verts), verts, GL_STATIC_DRAW);

	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	GLushort elems[] = {0, 1, 2};
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*sizeof (*elems), elems, GL_STATIC_DRAW);

	// Load shader
	GLuint shader = vgl_shader_source(vert_shader, -1, frag_shader, -1);
	if (!shader) panic("Shader compilation failed");
	GLuint shader_mvp = glGetUniformLocation(shader, "mvp");

	// MVP matrix
	mat44_t proj = vgl_perspective(vradians(35.0f), vaspect(width, height), 0.1f, 100.0f);
	vec3_t cam_pos = vec3(5, 5, 5);
	mat44_t cam = vgl_look(vec3(5, 5, 5), v3norm(v3neg(cam_pos)), vec3(0, 1, 0));
	mat44_t mvp = m4mul(cam, proj);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0, 0, 0, 0);
	glfwSwapInterval(1);

	while (!(glfwPollEvents(), glfwWindowShouldClose(win))) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shader);

		glUniformMatrix4fv(shader_mvp, 1, GL_FALSE, mvp.a);

		vgl_mesh_draw(gmesh, layout);

		glfwSwapBuffers(win);
	}

	return 0;
}
