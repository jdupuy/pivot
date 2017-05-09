////////////////////////////////////////////////////////////////////////////////
//
// Complete program (this compiles):
// Sphere Light Shading Demo
//
// g++ `sdl2-config --cflags` -I imgui planets.cpp gl_core_4_3.cpp  imgui/imgui*.cpp `sdl2-config --libs` -ldl -lGL -o planets
//

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <list>
#include <exception>
#include <SDL2/SDL.h>
#include "gl_core_4_3.h"

#define LOG(fmt, ...)  fprintf(stdout, fmt, ##__VA_ARGS__); fflush(stdout);

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define DJG_LOG(fmt, ...) LOG(fmt, ##__VA_ARGS__)
#define DJ_OPENGL_IMPLEMENTATION 1
#include "dj_opengl.h"

#define DJA_LOG(fmt, ...) LOG(fmt, ##__VA_ARGS__)
#define DJ_ALGEBRA_IMPLEMENTATION 1
#include "dj_algebra.h"

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"


////////////////////////////////////////////////////////////////////////////////
// Tweakable Constants
//
////////////////////////////////////////////////////////////////////////////////
#define VIEWER_DEFAULT_WIDTH  1280
#define VIEWER_DEFAULT_HEIGHT 720

////////////////////////////////////////////////////////////////////////////////
// Global Variables
//
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// Framebuffer Manager
enum { AA_NONE, AA_MSAA2, AA_MSAA4, AA_MSAA8, AA_MSAA16 };
struct FramebufferManager {
	int w, h, aa, pass, samplesPerPass, samplesPerPixel;
	struct {bool progressive, reset;} flags;
	struct {int fixed;} msaa;
	struct {float r, g, b;} clearColor;
} g_framebuffer = {
	VIEWER_DEFAULT_WIDTH, VIEWER_DEFAULT_HEIGHT, AA_MSAA2, 0, 8, 1024,
	{true, true},
	{false},
	{61./255., 119./255., 192./225}
};

// -----------------------------------------------------------------------------
// Camera Manager
struct CameraManager {
	float fovy, zNear, zFar; // perspective settings
	dja::vec3 pos;           // 3D position
	dja::mat3 axis;          // 3D frame
} g_camera = {
	55.f, 0.01f, 1024.f,
	dja::vec3(1.5, 0, 0.4),
	dja::mat3(
		0.971769, -0.129628, -0.197135,
		0.127271, 0.991562, -0.024635,
		0.198665, -0.001150, 0.980067
	)
};

// -----------------------------------------------------------------------------
// Planet Manager
enum {
	SHADING_PIVOT,
	SHADING_MC_MIS,
	SHADING_MC_MIS_JOINT,
	SHADING_MC_CAP,
	SHADING_MC_GGX,
	SHADING_MC_COS,
	SHADING_MC_H2,
	SHADING_MC_S2,
	SHADING_DEBUG
};
struct PlanetManager {
	struct {bool animate, showLines;} flags;
	struct {
		int xTess, yTess;
		int vertexCnt, indexCnt;
	} sphere;
	struct {
		const char **files;
		int cnt;
	} roughnessTextures, albedoTextures;
	struct Planet {
		float orbitRadius, orbitAngle, orbitVelocity;
		float rotationAngle, rotationVelocity;
		float scale;
		float roughness;
		float emissionIntensity;
		struct {float r, g, b;} emissionColor;
		int roughnessTexture, albedoTexture;
	} planets[4];
	int activePlanet;
	int shadingMode;
} g_planets = {
	{true, false},
	{24, 48, -1, -1}, // sphere
	{NULL, -1},       // roughnessTextures
	{NULL, -1},       // albedoTextures
	{
		{
			0, 0, 0,
			0, 0,
			0.2,
			1,
			5,
			{224.f/255.f, 224.f/255.f, 255.f/255.f},
			0, 0
		},
		{
			0.35, 45, 0.1,
			0.0, 0.5,
			0.1,
			1,
			0,
			{0.1, 0.1, 0.1},
			0, 0
		},
		{
			0.58, 170, 0.4,
			0, 0.8,
			0.08,
			1,
			10,
			{224.f/255.f, 0.f/255.f, 0.f/255.f},
			0, 0
		},
		{
			0.9, 0, 0.15,
			0, 0.2,
			0.17,
			1,
			0,
			{0.1, 0.1, 0.1},
			0, 0
		}
	},
	1,
	SHADING_PIVOT
};

// -----------------------------------------------------------------------------
// Application Manager
struct AppManager {
	struct {
		const char *shader;
		const char *output;
	} dir;
	struct {
		int w, h;
		bool hud;
		float gamma, exposure;
	} viewer;
	struct {
		int on, frame, capture;
	} recorder;
	int frame, frameLimit;
} g_app = {
	/*dir*/    {"./shaders/", "./"},
	/*viewer*/ {
	               VIEWER_DEFAULT_WIDTH, VIEWER_DEFAULT_HEIGHT,
	               true,
	               2.2f, -1.0f
	           },
	/*record*/ {false, 0, 0},
	/*frame*/  0, -1
};

// -----------------------------------------------------------------------------
// OpenGL Manager
enum { CLOCK_SPF, CLOCK_COUNT };
enum { FRAMEBUFFER_BACK, FRAMEBUFFER_SCENE, FRAMEBUFFER_COUNT };
enum { VERTEXARRAY_EMPTY, VERTEXARRAY_SPHERE, VERTEXARRAY_COUNT };
enum { STREAM_SPHERES, STREAM_TRANSFORM, STREAM_RANDOM, STREAM_COUNT };
enum {
	TEXTURE_BACK,
	TEXTURE_SCENE,
	TEXTURE_Z,
	TEXTURE_ROUGHNESS,
	TEXTURE_ALBEDO,
	TEXTURE_PIVOT,
	TEXTURE_COUNT
};
enum {
	BUFFER_SPHERE_VERTICES,
	BUFFER_SPHERE_INDEXES,
	BUFFER_COUNT
};
enum {
	PROGRAM_VIEWER,
	PROGRAM_BACKGROUND,
	PROGRAM_SPHERE,
	PROGRAM_COUNT
};
enum {
	UNIFORM_VIEWER_FRAMEBUFFER_SAMPLER,
	UNIFORM_VIEWER_EXPOSURE,
	UNIFORM_VIEWER_GAMMA,
	UNIFORM_VIEWER_VIEWPORT,

	UNIFORM_BACKGROUND_CLEAR_COLOR,

	UNIFORM_SPHERE_SAMPLES_PER_PASS,
	UNIFORM_SPHERE_PIVOT_SAMPLER,
	UNIFORM_SPHERE_ROUGHNESS_SAMPLER,
	UNIFORM_SPHERE_COUNT,

	UNIFORM_COUNT
};
struct OpenGLManager {
	GLuint programs[PROGRAM_COUNT];
	GLuint framebuffers[FRAMEBUFFER_COUNT];
	GLuint textures[TEXTURE_COUNT];
	GLuint vertexArrays[VERTEXARRAY_COUNT];
	GLuint buffers[BUFFER_COUNT];
	GLint uniforms[UNIFORM_COUNT];
	djg_buffer *streams[STREAM_COUNT];
	djg_clock *clocks[CLOCK_COUNT];
	djg_font *font;
} g_gl = {{0}};


////////////////////////////////////////////////////////////////////////////////
// Utility functions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef M_PI
#define M_PI 3.141592654
#endif
#define BUFFER_SIZE(x)    ((int)(sizeof(x)/sizeof(x[0])))
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

float radians(float degrees)
{
	return degrees * M_PI / 180.f;
}

char *strcat2(char *dst, const char *src1, const char *src2)
{
	strcpy(dst, src1);

	return strcat(dst, src2);
}

////////////////////////////////////////////////////////////////////////////////
// Program Configuration
//
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// set viewer program uniforms
void configureViewerProgram()
{
	glProgramUniform1i(g_gl.programs[PROGRAM_VIEWER],
	                   g_gl.uniforms[UNIFORM_VIEWER_FRAMEBUFFER_SAMPLER],
	                   TEXTURE_SCENE);
	glProgramUniform1f(g_gl.programs[PROGRAM_VIEWER],
	                   g_gl.uniforms[UNIFORM_VIEWER_EXPOSURE],
	                   g_app.viewer.exposure);
	glProgramUniform1f(g_gl.programs[PROGRAM_VIEWER],
	                   g_gl.uniforms[UNIFORM_VIEWER_GAMMA],
	                   g_app.viewer.gamma);
}

// -----------------------------------------------------------------------------
// set background program uniforms
void configureBackgroundProgram()
{
	glProgramUniform3f(g_gl.programs[PROGRAM_BACKGROUND],
	                   g_gl.uniforms[UNIFORM_BACKGROUND_CLEAR_COLOR],
	                   g_framebuffer.clearColor.r,
	                   g_framebuffer.clearColor.g,
	                   g_framebuffer.clearColor.b);
}

// -----------------------------------------------------------------------------
// set Sphere program uniforms
void configureSphereProgram()
{
	glProgramUniform1i(g_gl.programs[PROGRAM_SPHERE],
	                   g_gl.uniforms[UNIFORM_SPHERE_SAMPLES_PER_PASS],
	                   g_framebuffer.samplesPerPass);
	glProgramUniform1i(g_gl.programs[PROGRAM_SPHERE],
	                   g_gl.uniforms[UNIFORM_SPHERE_PIVOT_SAMPLER],
	                   TEXTURE_PIVOT);
	glProgramUniform1i(g_gl.programs[PROGRAM_SPHERE],
	                   g_gl.uniforms[UNIFORM_SPHERE_ROUGHNESS_SAMPLER],
	                   TEXTURE_ROUGHNESS);
}

////////////////////////////////////////////////////////////////////////////////
// Program Loading
//
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
/**
 * Load the Viewer Program
 *
 * This program is responsible for blitting the scene framebuffer to 
 * the back framebuffer, while applying gamma correction and tone mapping to 
 * the rendering.
 */
bool loadViewerProgram()
{
	djg_program *djp = djgp_create();
	GLuint *program = &g_gl.programs[PROGRAM_VIEWER];
	char buf[1024];

	LOG("Loading {Framebuffer-Blit-Program}\n");
	if (g_framebuffer.aa >= AA_MSAA2 && g_framebuffer.aa <= AA_MSAA16)
		djgp_push_string(djp, "#define MSAA_FACTOR %i\n", 1 << g_framebuffer.aa);
	djgp_push_file(djp, strcat2(buf, g_app.dir.shader, "viewer.glsl"));
	if (!djgp_gl_upload(djp, 430, false, true, program)) {
		LOG("=> Failure <=\n");
		djgp_release(djp);

		return false;
	}
	djgp_release(djp);

	g_gl.uniforms[UNIFORM_VIEWER_FRAMEBUFFER_SAMPLER] =
		glGetUniformLocation(g_gl.programs[PROGRAM_VIEWER], "u_FramebufferSampler");
	g_gl.uniforms[UNIFORM_VIEWER_VIEWPORT] =
		glGetUniformLocation(g_gl.programs[PROGRAM_VIEWER], "u_Viewport");
	g_gl.uniforms[UNIFORM_VIEWER_EXPOSURE] =
		glGetUniformLocation(g_gl.programs[PROGRAM_VIEWER], "u_Exposure");
	g_gl.uniforms[UNIFORM_VIEWER_GAMMA] =
		glGetUniformLocation(g_gl.programs[PROGRAM_VIEWER], "u_Gamma");

	configureViewerProgram();

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load the Background Program
 *
 * This program renders a Background.
 */
bool loadBackgroundProgram()
{
	djg_program *djp = djgp_create();
	GLuint *program = &g_gl.programs[PROGRAM_BACKGROUND];
	char buf[1024];

	LOG("Loading {Background-Program}\n");
	djgp_push_file(djp, strcat2(buf, g_app.dir.shader, "background.glsl"));
	if (!djgp_gl_upload(djp, 430, false, true, program)) {
		LOG("=> Failure <=\n");
		djgp_release(djp);

		return false;
	}
	djgp_release(djp);

	g_gl.uniforms[UNIFORM_BACKGROUND_CLEAR_COLOR] =
		glGetUniformLocation(g_gl.programs[PROGRAM_BACKGROUND], "u_ClearColor");

	configureBackgroundProgram();

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load the Sphere Program
 *
 * This program is responsible for rendering the spheres to the 
 * framebuffer
 */
bool loadSphereProgram()
{
	djg_program *djp = djgp_create();
	GLuint *program = &g_gl.programs[PROGRAM_SPHERE];
	char buf[1024];

	LOG("Loading {Sphere-Program}\n");
	switch (g_planets.shadingMode) {
		case SHADING_DEBUG:
			djgp_push_string(djp, "#define SHADE_DEBUG 1\n");
			break;
		case SHADING_PIVOT:
			djgp_push_string(djp, "#define SHADE_PIVOT 1\n");
			break;
		case SHADING_MC_GGX:
			djgp_push_string(djp, "#define SHADE_MC_GGX 1\n");
			break;
		case SHADING_MC_CAP:
			djgp_push_string(djp, "#define SHADE_MC_CAP 1\n");
			break;
		case SHADING_MC_MIS:
			djgp_push_string(djp, "#define SHADE_MC_MIS 1\n");
			break;
		case SHADING_MC_COS:
			djgp_push_string(djp, "#define SHADE_MC_COS 1\n");
			break;
		case SHADING_MC_H2:
			djgp_push_string(djp, "#define SHADE_MC_H2 1\n");
			break;
		case SHADING_MC_S2:
			djgp_push_string(djp, "#define SHADE_MC_S2 1\n");
			break;
		case SHADING_MC_MIS_JOINT:
			djgp_push_string(djp, "#define SHADE_MC_MIS_JOINT 1\n");
			break;
	};
	djgp_push_string(djp, "#define BUFFER_BINDING_RANDOM %i\n", STREAM_RANDOM);
	djgp_push_string(djp, "#define BUFFER_BINDING_TRANSFORMS %i\n", STREAM_TRANSFORM);
	djgp_push_string(djp, "#define BUFFER_BINDING_SPHERES %i\n", STREAM_SPHERES);
	djgp_push_string(djp, "#define SPHERE_COUNT %i\n", BUFFER_SIZE(g_planets.planets));
	djgp_push_file(djp, strcat2(buf, g_app.dir.shader, "ggx.glsl"));
	djgp_push_file(djp, strcat2(buf, g_app.dir.shader, "pivot.glsl"));
	djgp_push_file(djp, strcat2(buf, g_app.dir.shader, "sphere.glsl"));

	if (!djgp_gl_upload(djp, 430, false, true, program)) {
		LOG("=> Failure <=\n");
		djgp_release(djp);

		return false;
	}
	djgp_release(djp);

	g_gl.uniforms[UNIFORM_SPHERE_SAMPLES_PER_PASS] =
		glGetUniformLocation(g_gl.programs[PROGRAM_SPHERE], "u_SamplesPerPass");
	g_gl.uniforms[UNIFORM_SPHERE_PIVOT_SAMPLER] =
		glGetUniformLocation(g_gl.programs[PROGRAM_SPHERE], "u_PivotSampler");
	g_gl.uniforms[UNIFORM_SPHERE_ROUGHNESS_SAMPLER] =
		glGetUniformLocation(g_gl.programs[PROGRAM_SPHERE], "u_RoughnessSampler");

	configureSphereProgram();

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load All Programs
 *
 */
bool loadPrograms()
{
	bool v = true;

	v&= loadViewerProgram();
	v&= loadBackgroundProgram();
	v&= loadSphereProgram();

	return v;
}

////////////////////////////////////////////////////////////////////////////////
// Texture Loading
//
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
/**
 * Load the Scene Framebuffer Textures
 *
 * Depending on the scene framebuffer AA mode, this function load 2 or
 * 3 textures. In FSAA mode, two RGBA16F and one DEPTH24_STENCIL8 textures
 * are created. In other modes, one RGBA16F and one DEPTH24_STENCIL8 textures
 * are created.
 */
bool loadSceneFramebufferTexture()
{
	if (glIsTexture(g_gl.textures[TEXTURE_SCENE]))
		glDeleteTextures(1, &g_gl.textures[TEXTURE_SCENE]);
	if (glIsTexture(g_gl.textures[TEXTURE_Z]))
		glDeleteTextures(1, &g_gl.textures[TEXTURE_Z]);
	glGenTextures(1, &g_gl.textures[TEXTURE_Z]);
	glGenTextures(1, &g_gl.textures[TEXTURE_SCENE]);

	switch (g_framebuffer.aa) {
		case AA_NONE:
			LOG("Loading {Scene-Z-Framebuffer-Texture}\n");
			glActiveTexture(GL_TEXTURE0 + TEXTURE_Z);
			glBindTexture(GL_TEXTURE_2D, g_gl.textures[TEXTURE_Z]);
			glTexStorage2D(GL_TEXTURE_2D,
			               1,
			               GL_DEPTH24_STENCIL8,
			               g_framebuffer.w,
			               g_framebuffer.h);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			LOG("Loading {Scene-RGBA-Framebuffer-Texture}\n");
			glActiveTexture(GL_TEXTURE0 + TEXTURE_SCENE);
			glBindTexture(GL_TEXTURE_2D, g_gl.textures[TEXTURE_SCENE]);
			glTexStorage2D(GL_TEXTURE_2D,
			               1,
			               GL_RGBA32F,
			               g_framebuffer.w,
			               g_framebuffer.h);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		case AA_MSAA2:
		case AA_MSAA4:
		case AA_MSAA8:
		case AA_MSAA16: {
			int samples = 1 << g_framebuffer.aa;
			int maxSamples;

			glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &maxSamples);
			if (samples > maxSamples) {
				LOG("note: MSAA is %ix\n", maxSamples);
				samples = maxSamples;
			}
			LOG("Loading {Scene-MSAA-Z-Framebuffer-Texture}\n");
			glActiveTexture(GL_TEXTURE0 + TEXTURE_Z);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_gl.textures[TEXTURE_Z]);
			glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
			                          samples,
			                          GL_DEPTH24_STENCIL8,
			                          g_framebuffer.w,
			                          g_framebuffer.h,
			                          g_framebuffer.msaa.fixed);

			LOG("Loading {Scene-MSAA-RGBA-Framebuffer-Texture}\n");
			glActiveTexture(GL_TEXTURE0 + TEXTURE_SCENE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 
			              g_gl.textures[TEXTURE_SCENE]);
			glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
			                          samples,
			                          GL_RGBA32F,
			                          g_framebuffer.w,
			                          g_framebuffer.h,
			                          g_framebuffer.msaa.fixed);
		} break;
	}
	glActiveTexture(GL_TEXTURE0);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load the Back Framebuffer Texture
 *
 * This loads an RGBA8 texture used as a color buffer for the back 
 * framebuffer.
 */
bool loadBackFramebufferTexture()
{
	LOG("Loading {Back-Framebuffer-Texture}\n");
	if (glIsTexture(g_gl.textures[TEXTURE_BACK]))
		glDeleteTextures(1, &g_gl.textures[TEXTURE_BACK]);
	glGenTextures(1, &g_gl.textures[TEXTURE_BACK]);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_BACK);
	glBindTexture(GL_TEXTURE_2D, g_gl.textures[TEXTURE_BACK]);
	glTexStorage2D(GL_TEXTURE_2D,
	               1,
	               GL_RGBA8,
	               g_app.viewer.w,
	               g_app.viewer.h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE0);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load the Pivot Texture
 *
 * This loads a precomputed table that is used to map a GGX BRDF to a
 * Uniform PTSD parameter.
 */
bool loadPivotTexture()
{
	LOG("Loading {Pivot-Texture}\n");
	if (glIsTexture(g_gl.textures[TEXTURE_PIVOT]))
		glDeleteTextures(1, &g_gl.textures[TEXTURE_PIVOT]);
	glGenTextures(1, &g_gl.textures[TEXTURE_PIVOT]);

	// 64x64 table
	const float data[] = {
	#include "fit.inl"
	};

	glActiveTexture(GL_TEXTURE0 + TEXTURE_PIVOT);
	glBindTexture(GL_TEXTURE_2D, g_gl.textures[TEXTURE_PIVOT]);
	glTexStorage2D(GL_TEXTURE_2D,
	               1,
	               GL_RGBA32F,
	               64,
	               64);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RGBA, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE0);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load the Roughness Texture
 *
 * This loads an R8 texture used as a roughness texture map.
 */
bool loadRoughnessTextures()
{
	LOG("Loading {Pivot-Texture}\n");
	if (glIsTexture(g_gl.textures[TEXTURE_ROUGHNESS]))
		glDeleteTextures(1, &g_gl.textures[TEXTURE_ROUGHNESS]);
	glGenTextures(1, &g_gl.textures[TEXTURE_ROUGHNESS]);

	djg_texture *djgt = djgt_create(1);
	GLuint *glt = &g_gl.textures[TEXTURE_ROUGHNESS];

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ROUGHNESS);
	djgt_push_image(djgt, "./textures/moon.png", 0);

	if (!djgt_gl_upload(djgt, GL_TEXTURE_2D, GL_R8, 1, 1, glt)) {
		LOG("=> Failure <=\n");
		djgt_release(djgt);

		return false;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.f);
	glActiveTexture(GL_TEXTURE0);

	djgt_release(djgt);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load All Textures
 */
bool loadTextures()
{
	bool v = true;

	v&= loadSceneFramebufferTexture();
	v&= loadBackFramebufferTexture();
	v&= loadRoughnessTextures();
	v&= loadPivotTexture();

	return v;
}

////////////////////////////////////////////////////////////////////////////////
// Buffer Loading
//
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
/**
 * Load Sphere Data Buffers
 *
 * This procedure updates the transformations and the data of the spheres that
 * are used in the demo; it is updated each frame.
 */
void animatePlanets(float dt)
{
	if (g_planets.flags.animate) {
		for (int i = 0; i < BUFFER_SIZE(g_planets.planets); ++i) {
			g_planets.planets[i].orbitAngle+=
				g_planets.planets[i].orbitVelocity * dt;
			g_planets.planets[i].rotationAngle+=
				g_planets.planets[i].rotationVelocity * dt;

			while (g_planets.planets[i].orbitAngle > 360.f)
				g_planets.planets[i].orbitAngle-= 360.f;
			while (g_planets.planets[i].rotationAngle > 360.f)
				g_planets.planets[i].rotationAngle-= 360.f;
		}
		g_framebuffer.flags.reset = true;
	}
}

bool loadSphereDataBuffers(float dt = 0)
{
	static bool first = true;
	struct Transform {
		dja::mat4 model, modelView, modelViewProjection, viewInv;
	} transforms[BUFFER_SIZE(g_planets.planets)];
	struct Sphere {
		dja::vec4 geometry;
		dja::vec4 light;
		dja::vec4 brdf;
		dja::vec4 reserved;
	} spheres[BUFFER_SIZE(g_planets.planets)];

	if (first) {
		g_gl.streams[STREAM_TRANSFORM] = djgb_create(sizeof(transforms));
		g_gl.streams[STREAM_SPHERES] = djgb_create(sizeof(spheres));
		first = false;
	}

	// extract view and projection matrices
	dja::mat4 projection = dja::mat4::homogeneous::perspective(
		radians(g_camera.fovy),
		(float)g_framebuffer.w / (float)g_framebuffer.h,
		g_camera.zNear,
		g_camera.zFar
	);
	dja::mat4 viewInv = dja::mat4::homogeneous::translation(g_camera.pos)
	                  * dja::mat4::homogeneous::from_mat3(g_camera.axis);
	dja::mat4 view = dja::inverse(viewInv);

	// compute new planet positions
	animatePlanets(dt);
	for (int i = 0; i < BUFFER_SIZE(g_planets.planets); ++i) {
		float orbitAngle = radians(g_planets.planets[i].orbitAngle);
		float rotationAngle = radians(g_planets.planets[i].rotationAngle);
		dja::mat4 m1 = dja::mat4::homogeneous::rotation(
			dja::vec3(0, 0, 1), orbitAngle
		);
		dja::mat4 m2 = dja::mat4::homogeneous::translation(
			dja::vec3(g_planets.planets[i].orbitRadius, 0, 0)
		);
		dja::mat4 m3 = dja::mat4::homogeneous::rotation(
			dja::vec3(0, 0, 1), rotationAngle
		);
		dja::mat4 m4 = dja::mat4::homogeneous::scale(
			dja::vec3(g_planets.planets[i].scale)
		);

		// upload transformations
		transforms[i].model     = m1 * m2 * m3 * m4;
		transforms[i].modelView = view * transforms[i].model;
		transforms[i].modelViewProjection = projection * transforms[i].modelView;

		dja::vec4 spherePos = transforms[i].modelView * dja::vec4(0, 0, 0, 1);
		spheres[i].geometry = dja::vec4(
			spherePos.x, spherePos.y, spherePos.z, g_planets.planets[i].scale
		);
		spheres[i].light = dja::vec4(
			g_planets.planets[i].emissionColor.r * g_planets.planets[i].emissionIntensity,
			g_planets.planets[i].emissionColor.g * g_planets.planets[i].emissionIntensity,
			g_planets.planets[i].emissionColor.b * g_planets.planets[i].emissionIntensity,
			g_planets.planets[i].emissionIntensity > 0. ? 1.f : 0.f
		);
		spheres[i].brdf = dja::vec4(
			g_planets.planets[i].roughness
		);
	}

	// upload planet data
	djgb_gl_upload(g_gl.streams[STREAM_TRANSFORM], (const void *)transforms, NULL);
	djgb_glbindrange(g_gl.streams[STREAM_TRANSFORM],
	                 GL_UNIFORM_BUFFER,
	                 STREAM_TRANSFORM);
	djgb_gl_upload(g_gl.streams[STREAM_SPHERES], (const void *)spheres, NULL);
	djgb_glbindrange(g_gl.streams[STREAM_SPHERES],
	                 GL_UNIFORM_BUFFER,
	                 STREAM_SPHERES);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load Random Buffer
 *
 * This buffer holds the random samples used by the GLSL Monte Carlo integrator.
 * It should be updated at each frame. The random samples are generated using 
 * the Marsaglia pseudo-random generator.
 */
uint32_t mrand() // Marsaglia random generator
{
	static uint32_t m_z = 1, m_w = 2;

	m_z = 36969u * (m_z & 65535u) + (m_z >> 16u);
	m_w = 18000u * (m_w & 65535u) + (m_w >> 16u);

	return ((m_z << 16u) + m_w);
}

bool loadRandomBuffer()
{
	static bool first = true;
	float buffer[256];
	int offset = 0;

	if (first) {
		g_gl.streams[STREAM_RANDOM] = djgb_create(sizeof(buffer));
		first = false;
	}

	for (int i = 0; i < BUFFER_SIZE(buffer); ++i) {
		buffer[i] = (float)((double)mrand() / (double)0xFFFFFFFFu);
		assert(buffer[i] <= 1.f && buffer[i] >= 0.f);
	}

	djgb_gl_upload(g_gl.streams[STREAM_RANDOM], (const void *)buffer, &offset);
	djgb_glbindrange(g_gl.streams[STREAM_RANDOM],
	                 GL_UNIFORM_BUFFER,
	                 STREAM_RANDOM);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load Sphere Mesh Buffer
 *
 * This loads a vertex and an index buffer for a mesh.
 */
bool loadSphereMeshBuffers()
{
	int vertexCnt, indexCnt;
	djg_mesh *mesh = djgm_load_sphere(
		1.f, g_planets.sphere.xTess, g_planets.sphere.yTess
	);
	const djgm_vertex *vertices = djgm_get_vertices(mesh, &vertexCnt);
	const uint16_t *indexes = djgm_get_triangles(mesh, &indexCnt);

	if (glIsBuffer(g_gl.buffers[BUFFER_SPHERE_VERTICES]))
		glDeleteBuffers(1, &g_gl.buffers[BUFFER_SPHERE_VERTICES]);
	if (glIsBuffer(g_gl.buffers[BUFFER_SPHERE_INDEXES]))
		glDeleteBuffers(1, &g_gl.buffers[BUFFER_SPHERE_INDEXES]);

	LOG("Loading {Mesh-Vertex-Buffer}\n");
	glGenBuffers(1, &g_gl.buffers[BUFFER_SPHERE_VERTICES]);
	glBindBuffer(GL_ARRAY_BUFFER, g_gl.buffers[BUFFER_SPHERE_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER,
	             sizeof(djgm_vertex) * vertexCnt,
	             (const void*)vertices,
	             GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	LOG("Loading {Mesh-Grid-Index-Buffer}\n");
	glGenBuffers(1, &g_gl.buffers[BUFFER_SPHERE_INDEXES]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_gl.buffers[BUFFER_SPHERE_INDEXES]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             sizeof(uint16_t) * indexCnt,
	             (const void *)indexes,
	             GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	g_planets.sphere.indexCnt = indexCnt;
	g_planets.sphere.vertexCnt = vertexCnt;
	djgm_release(mesh);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load All Buffers
 *
 */
bool loadBuffers()
{
	bool v = true;

	v&= loadSphereDataBuffers();
	v&= loadRandomBuffer();
	v&= loadSphereMeshBuffers();

	return v;
}

////////////////////////////////////////////////////////////////////////////////
// Vertex Array Loading
//
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
/**
 * Load an Empty Vertex Array
 *
 * This will be used to draw procedural geometry, e.g., a fullscreen quad.
 */
bool loadEmptyVertexArray()
{
	LOG("Loading {Empty-VertexArray}\n");
	if (glIsVertexArray(g_gl.vertexArrays[VERTEXARRAY_EMPTY]))
		glDeleteVertexArrays(1, &g_gl.vertexArrays[VERTEXARRAY_EMPTY]);

	glGenVertexArrays(1, &g_gl.vertexArrays[VERTEXARRAY_EMPTY]);
	glBindVertexArray(g_gl.vertexArrays[VERTEXARRAY_EMPTY]);
	glBindVertexArray(0);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load Mesh Vertex Array
 *
 * This will be used to draw the sphere mesh loaded with the dj_opengl library.
 */
bool loadSphereVertexArray()
{
	LOG("Loading {Mesh-VertexArray}\n");
	if (glIsVertexArray(g_gl.vertexArrays[VERTEXARRAY_SPHERE]))
		glDeleteVertexArrays(1, &g_gl.vertexArrays[VERTEXARRAY_SPHERE]);

	glGenVertexArrays(1, &g_gl.vertexArrays[VERTEXARRAY_SPHERE]);
	glBindVertexArray(g_gl.vertexArrays[VERTEXARRAY_SPHERE]);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, g_gl.buffers[BUFFER_SPHERE_VERTICES]);
	glVertexAttribPointer(0, 4, GL_FLOAT, 0, sizeof(djgm_vertex),
	                      BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 4, GL_FLOAT, 0, sizeof(djgm_vertex),
	                      BUFFER_OFFSET(4 * sizeof(float)));
	glVertexAttribPointer(2, 4, GL_FLOAT, 0, sizeof(djgm_vertex),
	                      BUFFER_OFFSET(8 * sizeof(float)));
	glVertexAttribPointer(3, 4, GL_FLOAT, 0, sizeof(djgm_vertex),
	                      BUFFER_OFFSET(12 * sizeof(float)));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_gl.buffers[BUFFER_SPHERE_INDEXES]);
	glBindVertexArray(0);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load All Vertex Arrays
 *
 */
bool loadVertexArrays()
{
	bool v = true;

	v&= loadEmptyVertexArray();
	v&= loadSphereVertexArray();

	return v;
}

////////////////////////////////////////////////////////////////////////////////
// Framebuffer Loading
//
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
/**
 * Load the Back Framebuffer
 *
 * This framebuffer contains the final image. It will be blitted to the 
 * OpenGL window's backbuffer.
 */
bool loadBackFramebuffer()
{
	LOG("Loading {Back-Framebuffer}\n");
	if (glIsFramebuffer(g_gl.framebuffers[FRAMEBUFFER_BACK]))
		glDeleteFramebuffers(1, &g_gl.framebuffers[FRAMEBUFFER_BACK]);

	glGenFramebuffers(1, &g_gl.framebuffers[FRAMEBUFFER_BACK]);
	glBindFramebuffer(GL_FRAMEBUFFER, g_gl.framebuffers[FRAMEBUFFER_BACK]);
	glFramebufferTexture2D(GL_FRAMEBUFFER,
	                       GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D,
	                       g_gl.textures[TEXTURE_BACK],
	                       0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		LOG("=> Failure <=\n");

		return false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load the Scene Framebuffer
 *
 * This framebuffer is used to draw the 3D scene.
 * A single framebuffer is created, holding a color and Z buffer. 
 * The scene writes directly to it.
 */
bool loadSceneFramebuffer()
{
	LOG("Loading {Scene-Framebuffer}\n");
	if (glIsFramebuffer(g_gl.framebuffers[FRAMEBUFFER_SCENE]))
		glDeleteFramebuffers(1, &g_gl.framebuffers[FRAMEBUFFER_SCENE]);

	glGenFramebuffers(1, &g_gl.framebuffers[FRAMEBUFFER_SCENE]);
	glBindFramebuffer(GL_FRAMEBUFFER, g_gl.framebuffers[FRAMEBUFFER_SCENE]);

	if (g_framebuffer.aa >= AA_MSAA2 && g_framebuffer.aa <= AA_MSAA16) {
		glFramebufferTexture2D(GL_FRAMEBUFFER,
		                       GL_COLOR_ATTACHMENT0,
		                       GL_TEXTURE_2D_MULTISAMPLE,
		                       g_gl.textures[TEXTURE_SCENE],
		                       0);
		glFramebufferTexture2D(GL_FRAMEBUFFER,
		                       GL_DEPTH_STENCIL_ATTACHMENT,
		                       GL_TEXTURE_2D_MULTISAMPLE,
		                       g_gl.textures[TEXTURE_Z],
		                       0);
	} else {
		glFramebufferTexture2D(GL_FRAMEBUFFER,
		                       GL_COLOR_ATTACHMENT0,
		                       GL_TEXTURE_2D,
		                       g_gl.textures[TEXTURE_SCENE],
		                       0);
		glFramebufferTexture2D(GL_FRAMEBUFFER,
		                       GL_DEPTH_STENCIL_ATTACHMENT,
		                       GL_TEXTURE_2D,
		                       g_gl.textures[TEXTURE_Z],
		                       0);
	}

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		LOG("=> Failure <=\n");

		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return (glGetError() == GL_NO_ERROR);
}

// -----------------------------------------------------------------------------
/**
 * Load All Framebuffers
 *
 */
bool loadFramebuffers()
{
	bool v = true;

	v&= loadBackFramebuffer();
	v&= loadSceneFramebuffer();

	return v;
}

////////////////////////////////////////////////////////////////////////////////
// OpenGL Resource Loading
//
////////////////////////////////////////////////////////////////////////////////

void load()
{
	bool v = true;
	int i;

#ifndef NDEBUG
	djg_log_debug_output();
#endif

	for (i = 0; i < CLOCK_COUNT; ++i) {
		if (g_gl.clocks[i])
			djgc_release(g_gl.clocks[i]);
		g_gl.clocks[i] = djgc_create();
	}

	if (g_gl.font) djgf_release(g_gl.font);
	g_gl.font = djgf_create(GL_TEXTURE0 + TEXTURE_COUNT);

	if (v) v&= loadTextures();
	if (v) v&= loadBuffers();
	if (v) v&= loadFramebuffers();
	if (v) v&= loadVertexArrays();
	if (v) v&= loadPrograms();

	if (!v) throw std::exception();
}

void release()
{
	int i;

	for (i = 0; i < CLOCK_COUNT; ++i)
		if (g_gl.clocks[i])
			djgc_release(g_gl.clocks[i]);
	for (i = 0; i < STREAM_COUNT; ++i)
		if (g_gl.streams[i])
			djgb_release(g_gl.streams[i]);
	for (i = 0; i < PROGRAM_COUNT; ++i)
		if (glIsProgram(g_gl.programs[i]))
			glDeleteProgram(g_gl.programs[i]);
	for (i = 0; i < TEXTURE_COUNT; ++i)
		if (glIsTexture(g_gl.textures[i]))
			glDeleteTextures(1, &g_gl.textures[i]);
	for (i = 0; i < BUFFER_COUNT; ++i)
		if (glIsBuffer(g_gl.buffers[i]))
			glDeleteBuffers(1, &g_gl.buffers[i]);
	for (i = 0; i < FRAMEBUFFER_COUNT; ++i)
		if (glIsFramebuffer(g_gl.framebuffers[i]))
			glDeleteFramebuffers(1, &g_gl.framebuffers[i]);
	for (i = 0; i < VERTEXARRAY_COUNT; ++i)
		if (glIsVertexArray(g_gl.vertexArrays[i]))
			glDeleteVertexArrays(1, &g_gl.vertexArrays[i]);
	if (g_gl.font) djgf_release(g_gl.font);
}

////////////////////////////////////////////////////////////////////////////////
// OpenGL Rendering
//
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
/**
 * Render the Scene
 *
 * This drawing pass renders the 3D scene to the framebuffer.
 */
void renderSceneProgressive()
{
	// configure GL state
	glBindFramebuffer(GL_FRAMEBUFFER, g_gl.framebuffers[FRAMEBUFFER_SCENE]);
	glViewport(0, 0, g_framebuffer.w, g_framebuffer.h);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	if (g_framebuffer.flags.reset) {
		glClearColor(0, 0, 0, g_framebuffer.samplesPerPass);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		g_framebuffer.pass = 0;
		g_framebuffer.flags.reset = false;
	}

	// enable blending only after the first is complete 
	// (otherwise backfaces might be included in the rendering)
	if (g_framebuffer.pass > 0) {
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		loadRandomBuffer();
	} else {
		glDepthFunc(GL_LESS);
		glDisable(GL_BLEND);
	}

	// stop progressive drawing once the desired sampling rate has been reached
	if (g_framebuffer.pass * g_framebuffer.samplesPerPass
		< g_framebuffer.samplesPerPixel) {

		// draw planets
		if (g_planets.flags.showLines)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glUseProgram(g_gl.programs[PROGRAM_SPHERE]);
		glBindVertexArray(g_gl.vertexArrays[VERTEXARRAY_SPHERE]);
		glDrawElementsInstanced(GL_TRIANGLES,
		                        g_planets.sphere.indexCnt,
		                        GL_UNSIGNED_SHORT,
		                        NULL,
		                        BUFFER_SIZE(g_planets.planets));

		if (g_planets.flags.showLines)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// draw background
		glUseProgram(g_gl.programs[PROGRAM_BACKGROUND]);
		glBindVertexArray(g_gl.vertexArrays[VERTEXARRAY_EMPTY]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		++g_framebuffer.pass;
	}

	// restore GL state
	if (g_framebuffer.pass > 0) {
		glDepthFunc(GL_LESS);
		glDisable(GL_BLEND);
	}
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void renderScene()
{
	loadSphereDataBuffers(1.f);
	if (g_framebuffer.flags.progressive) {
		renderSceneProgressive();
	} else {
		int passCnt = g_framebuffer.samplesPerPixel
		            / g_framebuffer.samplesPerPass;

		if (!passCnt) passCnt = 1;
		for (int i = 0; i < passCnt; ++i) {
			loadRandomBuffer();
			renderSceneProgressive();
		}
	}
}

// -----------------------------------------------------------------------------
/**
 * Blit the Scene Framebuffer and draw GUI
 *
 * This drawing pass blits the scene framebuffer with possible magnification
 * and renders the HUD and TweakBar.
 */
void imguiSetAa()
{
	if (!loadSceneFramebufferTexture() || !loadSceneFramebuffer() 
	|| !loadViewerProgram()) {
		LOG("=> Framebuffer config failed <=\n");
		throw std::exception();
	}
	g_framebuffer.flags.reset = true;
}

void renderViewer(double cpuDt, double gpuDt)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_gl.framebuffers[FRAMEBUFFER_BACK]);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, g_gl.framebuffers[FRAMEBUFFER_SCENE]);
	glViewport(0, 0, g_app.viewer.w, g_app.viewer.h);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	// post process the scene framebuffer
	glUseProgram(g_gl.programs[PROGRAM_VIEWER]);
	glBindVertexArray(g_gl.vertexArrays[VERTEXARRAY_EMPTY]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// draw HUD
	if (g_app.viewer.hud) {
		glUseProgram(0);
		glBindVertexArray(0);

		djgf_print_fast(g_gl.font,
			            DJG_FONT_SMALL,
			            g_app.viewer.w - 200,
			            10,
			            "CPU_dt: %10.5f %s\n"\
			            "GPU_dt: %10.5f %s\n",
			            cpuDt < 1. ? cpuDt * 1e3 : cpuDt,
			            cpuDt < 1. ? "ms" : " s",
			            gpuDt < 1. ? gpuDt * 1e3 : gpuDt,
			            gpuDt < 1. ? "ms" : " s");

		// ImGui
		// Viewer Widgets
		ImGui::SetNextWindowPos(ImVec2(270, 10)/*, ImGuiSetCond_FirstUseEver*/);
		ImGui::SetNextWindowSize(ImVec2(250, 120)/*, ImGuiSetCond_FirstUseEver*/);
		ImGui::Begin("Framebuffer");
		{
			const char* aaItems[] = { 
				"None",
				"MSAA x2",
				"MSAA x4",
				"MSAA x8",
				"MSAA x16"
			};
			if (ImGui::Combo("AA", &g_framebuffer.aa, aaItems, BUFFER_SIZE(aaItems)))
				imguiSetAa();
			if (ImGui::Combo("MSAA", &g_framebuffer.msaa.fixed, "Fixed\0Random\0\0"))
				imguiSetAa();
			ImGui::Checkbox("Progressive", &g_framebuffer.flags.progressive);
			if (g_framebuffer.flags.progressive) {
				ImGui::SameLine();
				if (ImGui::Button("Reset"))
					g_framebuffer.flags.reset = true;
			}
		}
		ImGui::End();
		// Framebuffer Widgets
		ImGui::SetNextWindowPos(ImVec2(530, 10)/*, ImGuiSetCond_FirstUseEver*/);
		ImGui::SetNextWindowSize(ImVec2(250, 120)/*, ImGuiSetCond_FirstUseEver*/);
		ImGui::Begin("Viewer");
		{
			if (ImGui::SliderFloat("Exposure", &g_app.viewer.exposure, -3.0f, 3.0f))
				configureViewerProgram();
			if (ImGui::SliderFloat("Gamma", &g_app.viewer.gamma, 1.0f, 4.0f))
				configureViewerProgram();
			if (ImGui::Button("Take Screenshot")) {
				static int cnt = 0;
				char buf[1024];

				snprintf(buf, 1024, "screenshot%03i", cnt);
				glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
				djgt_save_glcolorbuffer_bmp(GL_FRONT, GL_RGBA, buf);
				++cnt;
			}
			if (ImGui::Button("Record"))
				g_app.recorder.on = !g_app.recorder.on;
			if (g_app.recorder.on) {
				ImGui::SameLine();
				ImGui::Text("Recording...");
			}
		}
		ImGui::End();
		// Camera Widgets
		ImGui::SetNextWindowPos(ImVec2(10, 10)/*, ImGuiSetCond_FirstUseEver*/);
		ImGui::SetNextWindowSize(ImVec2(250, 120)/*, ImGuiSetCond_FirstUseEver*/);
		ImGui::Begin("Camera");
		{
			if (ImGui::SliderFloat("FOVY", &g_camera.fovy, 1.0f, 179.0f))
				g_framebuffer.flags.reset = true;
			if (ImGui::SliderFloat("zNear", &g_camera.zNear, 0.01f, 100.f)) {
				if (g_camera.zNear >= g_camera.zFar)
					g_camera.zNear = g_camera.zFar - 0.01f;
			}
			if (ImGui::SliderFloat("zFar", &g_camera.zFar, 1.f, 1500.f)) {
				if (g_camera.zFar <= g_camera.zNear)
					g_camera.zFar = g_camera.zNear + 0.01f;
			}
		}
		ImGui::End();
		// Lighting/Planets Widgets
		ImGui::SetNextWindowPos(ImVec2(10, 140)/*, ImGuiSetCond_FirstUseEver*/);
		ImGui::SetNextWindowSize(ImVec2(250, 450)/*, ImGuiSetCond_FirstUseEver*/);
		ImGui::Begin("Planets");
		{
			const char* shadingModes[] = { 
				"Pivot",
				"MC MIS",
				"MC MIS Joint",
				"MC Cap",
				"MC GGX",
				"MC Cos",
				"MC H2",
				"MC S2",
				"Debug"
			};
			if (ImGui::Combo("Shading", &g_planets.shadingMode, shadingModes, BUFFER_SIZE(shadingModes))) {
				loadSphereProgram();
				g_framebuffer.flags.reset = true;
			}
			if (ImGui::CollapsingHeader("Flags", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Animate", &g_planets.flags.animate);
				ImGui::SameLine();
				if (ImGui::Checkbox("Wireframe", &g_planets.flags.showLines))
					g_framebuffer.flags.reset = true;
			}
			if (ImGui::CollapsingHeader("Geometry", ImGuiTreeNodeFlags_DefaultOpen)) {
				if (ImGui::SliderInt("xTess", &g_planets.sphere.xTess, 0, 128)) {
					loadSphereMeshBuffers();
					loadSphereVertexArray();
					g_framebuffer.flags.reset = true;
				}
				if (ImGui::SliderInt("yTess", &g_planets.sphere.yTess, 0, 128)) {
					loadSphereMeshBuffers();
					loadSphereVertexArray();
					g_framebuffer.flags.reset = true;
				}
			}
			if (ImGui::CollapsingHeader("Planet Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Combo("Id", &g_planets.activePlanet, "Sun\0Planet1\0Planet2\0Planet3\0\0");
				int id = g_planets.activePlanet;
				if (ImGui::SliderFloat("Radius", &g_planets.planets[id].scale, 0.0f, 0.5f))
					g_framebuffer.flags.reset = true;
				if (ImGui::SliderFloat("Emission Intensity", &g_planets.planets[id].emissionIntensity, 0.0f, 40.0f))
					g_framebuffer.flags.reset = true;
				if (ImGui::ColorEdit3("Emission Color", &g_planets.planets[id].emissionColor.r))
					g_framebuffer.flags.reset = true;
				if (id != 0) {
					if (ImGui::SliderFloat("Orbit Angle", &g_planets.planets[id].orbitAngle, 0.0f, 360.0f))
						g_framebuffer.flags.reset = true;
					if (ImGui::SliderFloat("Orbit Velocity", &g_planets.planets[id].orbitVelocity, 0.0f, 4.0f))
						g_framebuffer.flags.reset = true;
					if (ImGui::SliderFloat("Orbit Radius", &g_planets.planets[id].orbitRadius, 0.0f, 2.0f))
						g_framebuffer.flags.reset = true;

					if (ImGui::SliderFloat("Rotation Angle", &g_planets.planets[id].rotationAngle, 0.0f, 360.0f))
						g_framebuffer.flags.reset = true;
					if (ImGui::SliderFloat("Rotation Velocity", &g_planets.planets[id].rotationVelocity, 0.0f, 4.0f))
						g_framebuffer.flags.reset = true;
				}
			}
		}
		ImGui::End();

		ImGui::Render();
	}

	// screen recording
	if (g_app.recorder.on) {
		char name[64], path[1024];

		glBindFramebuffer(GL_READ_FRAMEBUFFER, g_gl.framebuffers[FRAMEBUFFER_BACK]);
		sprintf(name, "capture_%02i_%09i",
		        g_app.recorder.capture,
		        g_app.recorder.frame);
		strcat2(path, g_app.dir.output, name);
		djgt_save_glcolorbuffer_bmp(GL_COLOR_ATTACHMENT0, GL_RGB, path);
		++g_app.recorder.frame;
	}

	// restore state
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

// -----------------------------------------------------------------------------
/**
 * Blit the Composited Framebuffer to the Window Backbuffer
 *
 * Final drawing step: the composited framebuffer is blitted to the 
 * OpenGL window backbuffer
 */
void renderBack()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, g_gl.framebuffers[FRAMEBUFFER_BACK]);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// blit scene framebuffer
	glBlitFramebuffer(0, 0, g_app.viewer.w, g_app.viewer.h,
	                  0, 0, g_app.viewer.w, g_app.viewer.h,
	                  GL_COLOR_BUFFER_BIT,
	                  GL_NEAREST);
}

// -----------------------------------------------------------------------------
/**
 * Render Everything
 *
 */
void render()
{
	double cpuDt, gpuDt;

	djgc_start(g_gl.clocks[CLOCK_SPF]);
	renderScene();
	djgc_stop(g_gl.clocks[CLOCK_SPF]);
	djgc_ticks(g_gl.clocks[CLOCK_SPF], &cpuDt, &gpuDt);
	renderViewer(cpuDt, gpuDt);
	renderBack();
	++g_app.frame;
}
////////////////////////////////////////////////////////////////////////////////

void handleEvent(const SDL_Event *event)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard || io.WantCaptureMouse)
		return;

	switch(event->type) {
		case SDL_KEYDOWN:
			if (event->key.keysym.sym == SDLK_r) {
				loadPrograms();
				g_framebuffer.flags.reset = true;
			} else if (event->key.keysym.sym == SDLK_ESCAPE) {
				g_app.viewer.hud = !g_app.viewer.hud;
			}
		break;
		case SDL_MOUSEMOTION: {
			int x, y;
			unsigned int button= SDL_GetRelativeMouseState(&x, &y);
			const Uint8 *state = SDL_GetKeyboardState(NULL);

			if (button & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				dja::mat3 axis = dja::transpose(g_camera.axis);
				g_camera.axis = dja::mat3::rotation(dja::vec3(0, 0, 1), x * 5e-3)
				              * g_camera.axis;
				g_camera.axis = dja::mat3::rotation(axis[1], y * 5e-3)
				              * g_camera.axis;
				g_camera.axis[0] = dja::normalize(g_camera.axis[0]);
				g_camera.axis[1] = dja::normalize(g_camera.axis[1]);
				g_camera.axis[2] = dja::normalize(g_camera.axis[2]);
				g_framebuffer.flags.reset = true;
			} else if (button & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
				dja::mat3 axis = dja::transpose(g_camera.axis);
				g_camera.pos-= axis[1] * x * 5e-3 * norm(g_camera.pos);
				g_camera.pos+= axis[2] * y * 5e-3 * norm(g_camera.pos);
				g_framebuffer.flags.reset = true;
			}
		} break;
		case SDL_MOUSEWHEEL: {
			dja::mat3 axis = dja::transpose(g_camera.axis);
			g_camera.pos-= axis[0] * event->wheel.y * 5e-2 * norm(g_camera.pos);
			g_framebuffer.flags.reset = true;
		} break;
		default:
		break;
	}
}

int main(int argc, char **argv)
{
	Uint32 flags = SDL_WINDOW_OPENGL;
	SDL_Window *window;
	SDL_GLContext context;

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	// Create the Window
	LOG("Loading {Window-Main}\n");
	window = SDL_CreateWindow("OpenGL", 0, 0, g_app.viewer.w, 
	                          g_app.viewer.h, flags);
	if (!window) {
		LOG("=> Failure <=\n");
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Create the OpenGL context
	LOG("Loading {Window-GL-Context}\n")
	context = SDL_GL_CreateContext(window);
	if (!context) {
		LOG("=> Failure <=\n");
		LOG("-- Begin -- SDL Log\n" \
		    "%s\n"\
		    "-- End -- SDL Log\n", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Load OpenGL functions
	if (ogl_LoadFunctions() != ogl_LOAD_SUCCEEDED) {
		LOG("ogl_LoadFunctions failed\n");
		SDL_GL_DeleteContext(context);
		SDL_Quit();
		return EXIT_FAILURE;
	}

	LOG("-- Begin -- Demo\n");
	try {
		SDL_Event event;
		bool running = true;

		load();
		ImGui_ImplSdlGL3_Init(window);
		while (running) {
			while (SDL_PollEvent(&event) != 0) {
				ImGui_ImplSdlGL3_ProcessEvent(&event);
				if (event.type == SDL_QUIT) 
					running = false;
				handleEvent(&event);
			}
			ImGui_ImplSdlGL3_NewFrame(window);
			render();
			SDL_Delay(2);
			SDL_GL_SwapWindow(window);
		}
		ImGui_ImplSdlGL3_Shutdown();
		release();
		SDL_GL_DeleteContext(context);
	} catch (std::exception& e) {
		LOG("%s", e.what());
		SDL_GL_DeleteContext(context);
		SDL_Quit();
		LOG("(!) Demo Killed (!)\n");

		return EXIT_FAILURE;
	} catch (...) {
		SDL_GL_DeleteContext(context);
		SDL_Quit();
		LOG("(!) Demo Killed (!)\n");

		return EXIT_FAILURE;
	}
	LOG("-- End -- Demo\n");
	SDL_Quit();

	return EXIT_SUCCESS;
}
//
//
////////////////////////////////////////////////////////////////////////////////

