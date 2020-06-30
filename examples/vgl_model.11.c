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

struct global_data {
	double mousex, mousey;
	GLfloat cam_rotx, cam_roty;
	GLfloat cam_dist;
	mat44_t cam_mat;
};

static void update_camera(struct global_data *gd) {
	quat_t rot = qeuler(vec3(gd->cam_rotx, gd->cam_roty, 0), VGL_XYZ);
	vec3_t cam_dir = v3rot(vec3(0, 0, -1), rot);
	vec3_t cam_pos = v3sop(cam_dir, *, -gd->cam_dist);
	gd->cam_mat = vgl_look(cam_pos, cam_dir, vec3(0, 1, 0));
}

static void mousebtn(GLFWwindow *win, int btn, int act, int mods) {
	struct global_data *gd = glfwGetWindowUserPointer(win);
	if (btn == GLFW_MOUSE_BUTTON_MIDDLE) {
		if (act == GLFW_PRESS) {
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwGetCursorPos(win, &gd->mousex, &gd->mousey);
		} else {
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}

static void mousemove(GLFWwindow *win, double x, double y) {
	struct global_data *gd = glfwGetWindowUserPointer(win);
	if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) != GLFW_PRESS) return;

	GLfloat dx = x - gd->mousex;
	GLfloat dy = y - gd->mousey;

	gd->mousex = x;
	gd->mousey = y;

	const GLfloat ROTSCALE = -1.0/200.0;
	dx *= ROTSCALE;
	dy *= ROTSCALE;

	const GLfloat ylim = TAU_F/4.0001f;
	gd->cam_rotx += dy;
	if (gd->cam_rotx > ylim) gd->cam_rotx = ylim;
	else if (gd->cam_rotx < -ylim) gd->cam_rotx = -ylim;

	gd->cam_roty += dx;
	gd->cam_roty = fmod(gd->cam_roty, TAU_F);

	update_camera(gd);
}

static void mousescroll(GLFWwindow *win, double dx, double dy) {
	struct global_data *gd = glfwGetWindowUserPointer(win);
	const GLfloat ZOOMSCALE = -1.0f/1.5f;
	gd->cam_dist += ZOOMSCALE * (GLfloat)dy;
	if (gd->cam_dist < 0) gd->cam_dist = 0;
	update_camera(gd);
}

int main(int argc, char *argv[]) {
	const char *modelfn = "vgl_data/cube.vmsh";
	if (argc >= 2) modelfn = argv[1];

	int width = 1280, height = 1024;
	GLFWwindow *win = vgl_init(width, height, "Model loading example");
	if (!win) panic("Window creation failed");

	// Events
	struct global_data gd = {0};
	glfwSetWindowUserPointer(win, &gd);

	glfwSetMouseButtonCallback(win, mousebtn);
	glfwSetCursorPosCallback(win, mousemove);
	glfwSetScrollCallback(win, mousescroll);

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
	gd.cam_dist = 7.0f;
	update_camera(&gd);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0, 0, 0, 0);
	glfwSwapInterval(1);

	while (!(glfwPollEvents(), glfwWindowShouldClose(win))) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shader);

		mat44_t mvp = m4mul(gd.cam_mat, proj);
		glUniformMatrix4fv(shader_mvp, 1, GL_FALSE, mvp.a);

		vgl_mesh_draw(gmesh, layout);

		glfwSwapBuffers(win);
	}

	return 0;
}
