/* dj_opengl.h - v0.1 - public domain OpenGL3.3+ toolkit

   Do this:
      #define DJ_OPENGL_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.
   
   USAGE
   
   INTERFACING

   define DJG_ASSERT(x) to avoid using assert.h.
   define DJG_LOG(format, ...) to use your own logger (default prints in stdout)
   define DJG_MALLOC(x) to use your own memory allocator
   define DJG_FREE(x) to use your own memory deallocator

*/
#ifndef DJG_INCLUDE_DJ_OPENGL_H
#define DJG_INCLUDE_DJ_OPENGL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DJ_OPENGL_STATIC
#define DJGDEF static
#else
#define DJGDEF extern
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Debug Output API - Use OpenGL debug output
//

#ifdef GL_ARB_debug_output
DJGDEF void djg_log_debug_output(void);
#endif // GL_ARB_debug_output

//////////////////////////////////////////////////////////////////////////////
//
// Clock API - Get CPU and GPU ticks
//

typedef struct djg_clock djg_clock;

DJGDEF djg_clock *djgc_create(void);
DJGDEF void djgc_release(djg_clock *clock);

DJGDEF void djgc_start(djg_clock *clock);
DJGDEF void djgc_stop(djg_clock *clock);
DJGDEF void djgc_ticks(djg_clock *clock, double *cpu, double *gpu);

//////////////////////////////////////////////////////////////////////////////
//
// Program API - Load OpenGL programs quickly
//

typedef struct djg_program djg_program;

DJGDEF djg_program *djgp_create(void);
DJGDEF void djgp_release(djg_program *program);

DJGDEF bool djgp_push_file(djg_program *program, const char *filename);
DJGDEF bool djgp_push_string(djg_program *program, const char *format, ...);

DJGDEF bool djgp_gl_upload(const djg_program *program,
                           int version,
                           bool compatible,
                           bool link,
                           GLuint *gl);

//////////////////////////////////////////////////////////////////////////////
//
// Stream Buffer API - Stream data into a buffer asynchronously
//

typedef struct djg_buffer djg_buffer;

DJGDEF djg_buffer *djgb_create(int data_size);
DJGDEF void djgb_release(djg_buffer *buffer);

DJGDEF bool djgb_gl_upload(djg_buffer *buffer, const void *data, int *offset);

DJGDEF void djgb_glbind(const djg_buffer *buffer, GLenum target);
DJGDEF void djgb_glbindrange(const djg_buffer *buffer, 
                             GLenum target,
                             GLuint index);

//////////////////////////////////////////////////////////////////////////////
//
// Texture Loading API - Load OpenGL textures quickly
//    NOTE: requires Sean Barett's (http://nothings.org/) stbi library
//

#ifdef STBI_INCLUDE_STB_IMAGE_H

typedef struct djg_texture djg_texture;

DJGDEF djg_texture *djgt_create(int req_comp);
DJGDEF void djgt_release(djg_texture *texture);

DJGDEF bool djgt_push_image(djg_texture *texture,
                            const char *filename,
                            bool flipy);
#ifndef STBI_NO_HDR
DJGDEF bool djgt_push_hdrimage(djg_texture *texture,
                               const char *filename,
                               bool flipy);
#endif // STBI_NO_HDR
DJGDEF bool djgt_push_glcolorbuffer(djg_texture *texture,
                                    GLenum glbuffer,
                                    GLenum glformat,
                                    GLenum gltype,
                                    bool flipy);
#if 0
DJGDEF bool djgt_push_gltexture(djg_texture *texture,
                                GLenum target,
                                GLint level,
                                GLenum glformat,
                                GLenum gltype);
#endif

DJGDEF bool djgt_gl_upload(const djg_texture *texture,
                           GLint target,
                           GLint internalformat,
                           bool immutable,
                           bool mipmap,
                           GLuint *gl);

#endif // STBI_INCLUDE_STB_IMAGE_H

//////////////////////////////////////////////////////////////////////////////
//
// Texture Saving API - Save OpenGL textures / buffers as images
//    NOTE: requires Sean Barett's (http://nothings.org/) stbi and stbi_write 
//          library
//

#ifdef INCLUDE_STB_IMAGE_WRITE_H

// saves
DJGDEF bool djgt_save_bmp(const djg_texture *texture, const char *filename);
DJGDEF bool djgt_save_png(const djg_texture *texture, const char *filename);

// OpenGL color buffer saving
DJGDEF bool djgt_save_glcolorbuffer_bmp(GLenum glbuffer,
                                        GLenum glformat,
                                        const char *filename);
DJGDEF bool djgt_save_glcolorbuffer_png(GLenum glbuffer,
                                        GLenum glformat,
                                        const char *filename);

#endif // INCLUDE_STB_IMAGE_WRITE_H


//////////////////////////////////////////////////////////////////////////////
//
// Mesh API - Create meshed parametric surfaces
//

typedef struct djg_mesh djg_mesh;

typedef struct djgm_vertex {
	struct {float x, y, z, w;} p;    // position
	struct {float s, t, p, q;} st;   // texture coordinates
	struct {float x, y, z, w;} dpds; // tangent
	struct {float x, y, z, w;} dpdt; // bitangent
} djgm_vertex;

// destructor
DJGDEF void djgm_release(djg_mesh *mesh);

// factories
DJGDEF djg_mesh *djgm_load_plane(float width, float height, int slices, int stacks);
DJGDEF djg_mesh *djgm_load_disk(float radius, int slices, int stacks);
DJGDEF djg_mesh *djgm_load_sphere(float radius, int slices, int stacks);
DJGDEF djg_mesh *djgm_load_cylinder(float radius, float height,
                                    int slices, int stacks);
DJGDEF djg_mesh *djgm_load_torus(float ring_radius, int ring_segments,
                                 float pipe_radius, int pipe_segments);

// accessors
DJGDEF const uint16_t *djgm_get_triangles(const djg_mesh *mesh, GLint *count);
DJGDEF const uint16_t *djgm_get_quads(const djg_mesh *mesh, GLint *count);
DJGDEF const djgm_vertex *djgm_get_vertices(const djg_mesh *mesh, GLint *count);

// exports
DJGDEF bool djgm_export_obj_triangles(const djg_mesh *mesh, const char *filename);
DJGDEF bool djgm_export_obj_quads(const djg_mesh *mesh, const char *filename);


//////////////////////////////////////////////////////////////////////////////
//
// Font API - Simple font renderer for OpenGL3.3+
//

typedef struct djg_font djg_font;

enum {
	DJG_FONT_SMALL  = 1 << 3,
	DJG_FONT_BIG    = 1 << 4
};

// constructor / destructor
DJGDEF djg_font *djgf_create(int glunit);
DJGDEF void djgf_release(djg_font *font);

// manipulation
DJGDEF void djgf_set_color(djg_font *font,
                           GLubyte r, GLubyte g, GLubyte b, GLubyte a);
DJGDEF bool djgf_print(djg_font *font,
                       int size,
                       int x, int y,
                       const char *format,
                       ...);
// Experimental: does not restore the original OpenGL state
DJGDEF bool djgf_print_fast(djg_font *font,
                            int size,
                            int x, int y,
                            const char *format,
                            ...);

#ifdef __cplusplus
} // extern "C"
#endif

//
//
//// end header file /////////////////////////////////////////////////////
#endif // DJG_INCLUDE_DJ_OPENGL_H

#ifdef DJ_OPENGL_IMPLEMENTATION

#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef DJG_ASSERT
#	include <assert.h>
#	define DJG_ASSERT(x) assert(x)
#endif

#ifndef DJG_LOG
#	include <stdio.h>
#	define DJG_LOG(format, ...) fprintf(stdout, format, ##__VA_ARGS__)
#endif

#ifndef DJG_MALLOC
#	include <stdlib.h>
#	define DJG_MALLOC(x) (malloc(x))
#	define DJG_FREE(x) (free(x))
#else
#	ifndef DJG_FREE
#		error DJG_MALLOC defined without DJG_FREE
#	endif
#endif

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795 
#endif

// Internal constants
#define DJG__EPSILON 1e-4
#define DJG__CHAR_BUFFER_SIZE 4096

// Internal macros
#define DJG__BUFSIZE(x) sizeof(x) / sizeof(x[0])

// *************************************************************************************************
// Debug Output API Implementation

#ifdef GL_ARB_debug_output

static void
djg__debug_output_logger(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const GLvoid* userParam
) {
	char srcstr[32], typestr[32];

	switch(source) {
		case GL_DEBUG_SOURCE_API_ARB: strcpy(srcstr, "OpenGL"); break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: strcpy(srcstr, "Windows"); break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: strcpy(srcstr, "Shader Compiler"); break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: strcpy(srcstr, "Third Party"); break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB: strcpy(srcstr, "Application"); break;
		case GL_DEBUG_SOURCE_OTHER_ARB: strcpy(srcstr, "Other"); break;
		default: strcpy(srcstr, "???"); break;
	};

	switch(type) {
		case GL_DEBUG_TYPE_ERROR_ARB: strcpy(typestr, "Error"); break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: strcpy(typestr, "Deprecated Behavior"); break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: strcpy(typestr, "Undefined Behavior"); break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB: strcpy(typestr, "Portability"); break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB: strcpy(typestr, "Performance"); break;
		case GL_DEBUG_TYPE_OTHER_ARB: strcpy(typestr, "Message"); break;
#if 0
		case GL_DEBUG_TYPE_MARKER: strcpy(typestr, "Marker"); break;
		case GL_DEBUG_TYPE_PUSH_GROUP: strcpy(typestr, "Push Group"); break;
		case GL_DEBUG_TYPE_POP_GROUP: strcpy(typestr, "Pop Group"); break;
#endif
		default: strcpy(typestr, "???"); break;
	}

	if(severity == GL_DEBUG_SEVERITY_HIGH_ARB) {
		DJG_LOG("djg_error: %s %s\n"                \
		        "-- Begin -- GL_ARB_debug_output\n" \
		        "%s\n"                              \
		        "-- End -- GL_ARB_debug_output\n",
		        srcstr, typestr, message);
	} else if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB) {
		DJG_LOG("djg_warn: %s %s\n"                 \
		        "-- Begin -- GL_ARB_debug_output\n" \
		        "%s\n"                              \
		        "-- End -- GL_ARB_debug_output\n",
		        srcstr, typestr, message);
	}
#ifndef NVERBOSE
#if 0
	else {
		DJG_LOG("djg_verbose: %s %s\n"                 \
		        "-- Begin -- GL_ARB_debug_output\n" \
		        "%s\n"                              \
		        "-- End -- GL_ARB_debug_output\n",
		        srcstr, typestr, message);
	}
#endif
#endif
}

DJGDEF void djg_log_debug_output(void)
{
#ifndef _WIN32 // FIXME segfault on vs2010 ?!!
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageCallbackARB((GLDEBUGPROCARB)&djg__debug_output_logger, NULL);
#endif
}

#endif // GL_ARB_debug_output

// *************************************************************************************************
// Timer API Implementation

enum {DJGC__QUERY_START, DJGC__QUERY_STOP, DJGC__QUERY_COUNT};

typedef struct djg_clock {
	GLdouble cpu_ticks, gpu_ticks;
	GLint64 cpu_start_ticks;
	GLuint queries[DJGC__QUERY_COUNT];
	GLint is_gpu_ticking,
	      is_cpu_ticking,
	      is_gpu_ready;
} djg_clock;

DJGDEF djg_clock *djgc_create(void) 
{
	djg_clock *clock = (djg_clock *)DJG_MALLOC(sizeof(*clock));

	glGenQueries(DJGC__QUERY_COUNT, clock->queries);
	clock->cpu_ticks = 0.0;
	clock->gpu_ticks = 0.0;
	clock->cpu_start_ticks = 0.0;
	clock->is_cpu_ticking = GL_FALSE;
	clock->is_gpu_ticking = GL_FALSE;
	clock->is_gpu_ready = GL_TRUE;
	glQueryCounter(clock->queries[DJGC__QUERY_START], GL_TIMESTAMP);
	glQueryCounter(clock->queries[DJGC__QUERY_STOP], GL_TIMESTAMP);

	return clock;
}

DJGDEF void djgc_release(djg_clock *clock)
{
	DJG_ASSERT(clock);
	glDeleteQueries(DJGC__QUERY_COUNT, clock->queries);
	DJG_FREE(clock);
}

DJGDEF void djgc_start(djg_clock *clock)
{
	DJG_ASSERT(clock);
	if (!clock->is_cpu_ticking) {
		clock->is_cpu_ticking = GL_TRUE;
		glGetInteger64v(GL_TIMESTAMP, &clock->cpu_start_ticks);
	}
	if (!clock->is_gpu_ticking && clock->is_gpu_ready) {
		glQueryCounter(clock->queries[DJGC__QUERY_START], GL_TIMESTAMP);
		clock->is_gpu_ticking = GL_TRUE;
	}
}

DJGDEF void djgc_stop(djg_clock *clock)
{
	DJG_ASSERT(clock);
	if (clock->is_cpu_ticking) {
		GLint64 now = 0;

		glGetInteger64v(GL_TIMESTAMP, &now);
		clock->cpu_ticks = (now - clock->cpu_start_ticks) / 1e9;
		clock->is_cpu_ticking = GL_FALSE;
	}
	if (clock->is_gpu_ticking && clock->is_gpu_ready) {
		glQueryCounter(clock->queries[DJGC__QUERY_STOP], GL_TIMESTAMP);
		clock->is_gpu_ticking = GL_FALSE;
	}
}

// lazy GPU evaluation
DJGDEF void djgc_ticks(djg_clock *clock, double *tcpu, double *tgpu)
{
	DJG_ASSERT(clock);
	if (!clock->is_gpu_ticking) {
		glGetQueryObjectiv(clock->queries[DJGC__QUERY_STOP],
		                   GL_QUERY_RESULT_AVAILABLE,
		                   &clock->is_gpu_ready);
		if (clock->is_gpu_ready) {
			GLuint64 start, stop;

			glGetQueryObjectui64v(clock->queries[DJGC__QUERY_STOP],
			                      GL_QUERY_RESULT,
			                      &stop);
			glGetQueryObjectui64v(clock->queries[DJGC__QUERY_START],
			                      GL_QUERY_RESULT,
			                      &start);
			clock->gpu_ticks = (stop - start) / 1e9;
		}
	}
	if (tcpu) *tcpu = clock->cpu_ticks;
	if (tgpu) *tgpu = clock->gpu_ticks;
}

// *************************************************************************************************
// Program API Implementation

enum {
	DJGP__STAGE_VERTEX_BIT          = 1,
	DJGP__STAGE_FRAGMENT_BIT        = 1 << 1,
	DJGP__STAGE_GEOMETRY_BIT        = 1 << 2,
	DJGP__STAGE_TESS_CONTROL_BIT    = 1 << 3,
	DJGP__STAGE_TESS_EVALUATION_BIT = 1 << 4,
	DJGP__STAGE_COMPUTE_BIT         = 1 << 5
};

typedef struct djg_program {
	char *src; // null terminated string holding the source code
	struct djg_program *next;
} djg_program;

static bool
djgp__attach_shader(
	GLuint program,
	GLenum shader_t,
	GLsizei count,
	const GLchar **source
) {
	GLint compiled;
	GLuint shader = glCreateShader(shader_t);

	// set source and compile
	glShaderSource(shader, count, source, NULL);
	glCompileShader(shader);

	// check compilation
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint logc = 0;
		GLchar *logv = NULL;

		// retrieve GLSL compiler information and log 
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logc);
		logv = (GLchar *)DJG_MALLOC(logc);
		glGetShaderInfoLog(shader, logc, NULL, logv);
		DJG_LOG("djg_error: Shader compilation failed\n"\
		        "-- Begin -- GLSL Compiler Info Log\n"          \
		        "%s\n"                                          \
		        "-- End -- GLSL Compiler Info Log\n", logv);
		DJG_FREE(logv);

		glDeleteShader(shader);
		return false;
	}

	// attach shader and flag for deletion
	glAttachShader(program, shader);
	glDeleteShader(shader);
	return true;
}

static bool djgp__push_src(djg_program *program, char *src)
{
	djg_program *tail = program;

	while (tail->next) tail = tail->next;
	tail->next = djgp_create();
	tail->next->src = src;
	
	return true;
}

static int djgp__count(const djg_program *program)
{
	const djg_program *it = program;
	int i = 0;

	while (it->next) {
		it = it->next;
		++i;
	}

	return i;
}

DJGDEF djg_program *djgp_create(void)
{
	djg_program *head = (djg_program*)DJG_MALLOC(sizeof(*head));

	head->next = NULL;
	head->src = NULL;

	return head;
}

DJGDEF void djgp_release(djg_program *program)
{
	djg_program *tmp;

	DJG_ASSERT(program);
	while (program) {
		tmp = program;
		program = program->next;
		DJG_FREE(tmp->src);
		DJG_FREE(tmp);
	}
}

DJGDEF bool djgp_push_file(djg_program *program, const char *file)
{
	FILE *pf;
	char *src, letter;
	int n = 0;

	DJG_ASSERT(program);
	pf = fopen(file, "r");
	if (!pf) {
		DJG_LOG("djg_error: Function fopen() failed\n");
		return false;
	}

	// get file length
#if 0 // nonportable (does not work with vs2010)
	fseek(pf, 0, SEEK_END);
	n = (int)ftell(pf) + /* '\0' */1;
#else
	do {++n;} while (fread(&letter, 1, 1, pf));
	++n; // add an extra char for an endline
#endif
	rewind(pf);

	// read and push to source
	src = (char *)DJG_MALLOC(sizeof(char) * n);
	fread(src, 1, n-1, pf);
	fclose(pf);
	src[n-2] = '\n'; // add endline
	src[n-1] = '\0'; // add EOF

	return djgp__push_src(program, src);
}

DJGDEF bool djgp_push_string(djg_program *program, const char *str, ...)
{
	char *buf;
	va_list vl;
	int n;

	DJG_ASSERT(program);
	DJG_ASSERT(strlen(str) < DJG__CHAR_BUFFER_SIZE);
	buf = (char *)DJG_MALLOC(sizeof(char) * DJG__CHAR_BUFFER_SIZE);
	va_start(vl, str);
	n = vsnprintf(buf, DJG__CHAR_BUFFER_SIZE, str, vl);
	va_end(vl);
	if (n < 0 || n > DJG__CHAR_BUFFER_SIZE) {
		DJG_LOG("djg_error: string too long\n");
		return false;
	}

	return djgp__push_src(program, buf);
}

DJGDEF bool
djgp_gl_upload(
	const djg_program *program,
	int version,
	bool compatible,
	bool link,
	GLuint *gl
) {
	const GLchar **srcv;
	djg_program *it;
	int i, srcc, stages = 0;
	GLuint glprogram;

	DJG_ASSERT(program && gl);

	// prepare sources
	srcc = djgp__count(program) + /* head */1;
	srcv = (const GLchar **)DJG_MALLOC(sizeof(GLchar *) * srcc);

	// set source pointers
	it = program->next;
	i = 1;
	while (it) {
		srcv[i] = it->src;
		it = it->next; 
		++i;
	}

	// look for stages in the last source file
	if (strstr(srcv[srcc-1], "VERTEX_SHADER"))
		stages|= DJGP__STAGE_VERTEX_BIT;
	if (strstr(srcv[srcc-1], "FRAGMENT_SHADER"))
		stages|= DJGP__STAGE_FRAGMENT_BIT;
	if (strstr(srcv[srcc-1], "GEOMETRY_SHADER"))
		stages|= DJGP__STAGE_GEOMETRY_BIT;
	if (strstr(srcv[srcc-1], "TESS_CONTROL_SHADER"))
		stages|= DJGP__STAGE_TESS_CONTROL_BIT;
	if (strstr(srcv[srcc-1], "TESS_EVALUATION_SHADER"))
		stages|= DJGP__STAGE_TESS_EVALUATION_BIT;
	if (strstr(srcv[srcc-1], "COMPUTE_SHADER"))
		stages|= DJGP__STAGE_COMPUTE_BIT;

	if (!stages) {
		DJG_LOG("djg_error: no shader stage found in source\n");
		DJG_FREE(srcv);
		return false;
	}
	// load program
	glprogram = glCreateProgram();
	if (!glIsProgram(glprogram)) {
		fprintf(stderr, "djg_error: glCreateProgram failed\n");
		DJG_FREE(srcv);
		return false;
	}

#define DJGP__ATTACH_SHADER(glstage, str, bit)                        \
	if (stages & bit) {                                               \
		char head[DJG__CHAR_BUFFER_SIZE];                             \
		head[0] = '\0';                                               \
		                                                              \
		sprintf(head,                                                 \
		        "#version %d %s\n",                                   \
		        version,                                              \
		        compatible ? "compatibility" : "");                   \
		strcat(head, "#define " str " 1\n");                          \
		srcv[0] = head;                                               \
		if (!djgp__attach_shader(glprogram, glstage, srcc, srcv)) {   \
			glDeleteProgram(glprogram);                               \
			DJG_FREE(srcv);                                           \
			return false;                                             \
		}                                                             \
	}
	DJGP__ATTACH_SHADER(GL_VERTEX_SHADER, "VERTEX_SHADER", DJGP__STAGE_VERTEX_BIT);
	DJGP__ATTACH_SHADER(GL_FRAGMENT_SHADER, "FRAGMENT_SHADER", DJGP__STAGE_FRAGMENT_BIT);
#if defined(GL_GEOMETRY_SHADER)
	DJGP__ATTACH_SHADER(GL_GEOMETRY_SHADER, "GEOMETRY_SHADER", DJGP__STAGE_GEOMETRY_BIT);
#endif
#if defined(GL_TESS_CONTROL_SHADER)
	DJGP__ATTACH_SHADER(GL_TESS_CONTROL_SHADER, "TESS_CONTROL_SHADER", DJGP__STAGE_TESS_CONTROL_BIT);
#endif
#if defined(GL_TESS_EVALUATION_SHADER)
	DJGP__ATTACH_SHADER(GL_TESS_EVALUATION_SHADER, "TESS_EVALUATION_SHADER", DJGP__STAGE_TESS_EVALUATION_BIT);
#endif
#if defined(GL_COMPUTE_SHADER)
	DJGP__ATTACH_SHADER(GL_COMPUTE_SHADER, "COMPUTE_SHADER", DJGP__STAGE_COMPUTE_BIT);
#endif

#undef DJGP__ATTACH_SHADER

	// link if requested
	if (link) {
		GLint link_status = 0;

		glLinkProgram(glprogram);
		glGetProgramiv(glprogram, GL_LINK_STATUS, &link_status);
		if (!link_status) {
			GLint logc = 0;
			GLchar *logv = NULL;

			glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &logc);
			logv = (GLchar *)DJG_MALLOC(logc);
			glGetProgramInfoLog(glprogram, logc, NULL, logv);
			fprintf(stderr, "djg_error: GLSL linker failure\n\
			                 -- Begin -- GLSL Linker Info Log\n\
			                 %s\n\
			                 -- End -- GLSL Linker Info Log\n", logv);
			glDeleteProgram(glprogram);
			DJG_FREE(logv);
			DJG_FREE(srcv);
			return false;
		}
	}

	// cleanup
	DJG_FREE(srcv);
	// affect program
	if (glIsProgram (*gl)) glDeleteProgram(*gl);
	*gl = glprogram;

	return true;
}

// *************************************************************************************************
// Buffer Streaming API Implementation

typedef struct djg_buffer {
	GLuint gl; // OpenGL name
	int capacity; // total buffer capacity
	int size;     // size of streamed data
	int offset;   // current offset inside the buffer
} djg_buffer;

DJGDEF djg_buffer *djgb_create(int data_size)
{
	const int buf_capacity = (1 << 20); // capacity in Bytes
	djg_buffer *buffer = (djg_buffer*)DJG_MALLOC(sizeof(*buffer));

	DJG_ASSERT(data_size > 0 && buf_capacity > 8 * data_size);
	glGenBuffers(1, &buffer->gl);
	buffer->capacity = buf_capacity;
	buffer->size = data_size;
	buffer->offset = buffer->capacity;

	return buffer;
}

DJGDEF void djgb_release(djg_buffer *buffer)
{
	DJG_ASSERT(buffer);
	glDeleteBuffers(1, &buffer->gl);
	DJG_FREE(buffer);
}

DJGDEF bool djgb_gl_upload(djg_buffer *buffer, const void *data, int *offset)
{
	GLint buf = 0;
	void *ptr = NULL;

	// save GL state
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buf);

	// orphaning
	if (buffer->offset + buffer->size > buffer->capacity) {
#ifndef NDEBUG
		DJG_LOG("djg_debug: Reached font buffer capacity (buffer orphaned)\n");
#endif
		// allocate GL memory
		glBindBuffer(GL_ARRAY_BUFFER, buffer->gl);
		glBufferData(GL_ARRAY_BUFFER, buffer->capacity, NULL, GL_STREAM_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		buffer->offset = 0;
	}

	// stream data asynchronously
	glBindBuffer(GL_ARRAY_BUFFER, buffer->gl);
	ptr = glMapBufferRange(
		GL_ARRAY_BUFFER,
		buffer->offset,
		buffer->size,
		GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT
	);
	if (!ptr) {
		DJG_LOG("djg_error: Buffer mapping failed\n");

		return false;
	}
	memcpy(ptr, data, buffer->size);
	glUnmapBuffer(GL_ARRAY_BUFFER);

	// update buffer offset
	if (offset) (*offset) = buffer->offset;
	buffer->offset+= buffer->size;

	return true;
}

DJGDEF void djgb_glbindrange(djg_buffer *buffer, GLenum target, GLuint index)
{
	int offset = buffer->offset - buffer->size;
	glBindBufferRange(target, index, buffer->gl, offset, buffer->size);
}

DJGDEF void djgb_glbind(const djg_buffer *buffer, GLenum target)
{
	glBindBuffer(target, buffer->gl);
}

// *************************************************************************************************
// Texture Loading API Implementation

#ifdef STBI_INCLUDE_STB_IMAGE_H

typedef struct djg_texture {
	struct djg_texture *next;
	char *texels;   // pixel data
	int x, y, comp, hdr;  // width, height, format, and hdr flag
} djg_texture;

static void djgt__flipy(djg_texture *texture)
{
	int x, y, bpp = texture->comp * (texture->hdr ? 4 : 1);

	for (x = 0; x < texture->x; ++x)
	for (y = 0; y < texture->y / 2; ++y) {
		char *tx1 = &texture->texels[bpp * (texture->x * y + x)];
		char *tx2 = &texture->texels[bpp * (texture->x * (texture->y - 1 - y) + x)];
		char tmp[64];

		memcpy(tmp, tx1, bpp);
		memcpy(tx1, tx2, bpp);
		memcpy(tx2, tmp, bpp);
	}
}

static bool
djgt__push_texture(djg_texture *head, djg_texture *texture, bool flipy)
{
	djg_texture *tail = head;

	if (flipy) djgt__flipy(texture);
	while (tail->next) tail = tail->next;
	tail->next = texture;

	return true;
}

static int djgt__count(const djg_texture *texture)
{
	const djg_texture *it = texture;
	int i = 0;

	while (it->next) {
		it = it->next;
		++i;
	}

	return i;
}


DJGDEF djg_texture *djgt_create(int req_comp)
{
	djg_texture *head = (djg_texture*)DJG_MALLOC(sizeof(*head));

	DJG_ASSERT(req_comp >= 0 && req_comp <= 4);
	head->next = NULL;
	head->texels = NULL;
	head->x = 0;
	head->y = 0;
	head->comp = req_comp;
	head->hdr = false;

	return head;
}

DJGDEF void djgt_release(djg_texture *texture)
{
	djg_texture *tmp;

	DJG_ASSERT(texture);
	while (texture) {
		tmp = texture;
		texture = texture->next;
		DJG_FREE(tmp->texels);
		DJG_FREE(tmp);
	}
}

DJGDEF bool
djgt_push_image(djg_texture *texture, const char *filename, bool flipy)
{
	djg_texture *tail = djgt_create(texture->comp);

	tail->hdr = false;
	tail->texels = (char *)stbi_load(
		filename, &tail->x, &tail->y, &tail->comp, texture->comp
	);
	if (!tail->texels) {
		DJG_LOG("djg_error: Image loading failed\n");
#ifndef STBI_NO_FAILURE_STRINGS
		DJG_LOG("-- Begin -- STBI Log\n");
		DJG_LOG("%s\n", stbi_failure_reason());
		DJG_LOG("-- End -- STBI Log\n");
#endif // STBI_NO_FAILURE_STRINGS
		djgt_release(tail);

		return false;
	}
	djgt__push_texture(texture, tail, flipy);

	return true;
}

#ifndef STBI_NO_HDR
DJGDEF bool
djgt_push_hdrimage(djg_texture *texture, const char *filename, bool flipy)
{
	djg_texture *tail = djgt_create(texture->comp);

	tail->hdr = true;
	tail->texels = (char *)stbi_loadf(
		filename, &tail->x, &tail->y, &tail->comp, texture->comp
	);
	if (!tail->texels) {
		DJG_LOG("djg_error: Image loading failed\n");
#ifndef STBI_NO_FAILURE_STRINGS
		DJG_LOG("-- Begin -- STBI Log\n");
		DJG_LOG("%s\n", stbi_failure_reason());
		DJG_LOG("-- End -- STBI Log\n");
#endif // STBI_NO_FAILURE_STRINGS
		djgt_release(tail);

		return false;
	}
	djgt__push_texture(texture, tail, flipy);

	return true;
}
#endif // STBI_NO_HDR

#ifndef NGL_ARB_texture_storage
/**
 * Nearest Power of Two Value
 */
static int djgt__exp2(int x)
{
	size_t i;

	--x;
	for (i = 1; i < sizeof(int) * 8; i <<= 1) {
		x = x | x >> i;
	}

	return (x + 1);
}

/**
 * Nearest Power of Two Exponent
 */
static int djgt__log2(int x)
{
	int p = djgt__exp2(x);
	int e = 0;

	while (! (p & 1)) {
		p >>= 1;
		++e;
	}
	return e;
}

/**
 * Maximum Value Between 3 Numbers
 *
 * This algorithm takes advantage of short circuiting.
 */
static int djgt__max3(int x, int y, int z)
{
	int m = x;

	(void)((m < y) && (m = y));
	(void)((m < z) && (m = z));

	return m;
}

/**
 * Mip Count
 */
static int djgt__mipcnt(int x, int y, int z)
{
	return (djgt__log2(djgt__max3(x, y, z)) + /* base MIP */1);
}
#endif // NGL_ARB_texture_storage

/**
 * OpenGL State management for Texture uploads.
 * The upload is GL_PIXEL_STORE state safe. Note that
 * the texture is created in the active channel, so 
 * previous bindings are overwritten. You can safely bind 
 * the texture by setting glActiveTexture to a free channel
 *
 */

enum {
	DJGT__GLPSS_BUFFER,
	DJGT__GLPSS_SWAPBYTES,
	DJGT__GLPSS_LSBFIRST,
	DJGT__GLPSS_ROWLENGTH,
	DJGT__GLPSS_IMAGEHEIGHT,
	DJGT__GLPSS_SKIPROWS,
	DJGT__GLPSS_SKIPPIXELS,
	DJGT__GLPSS_SKIPIMAGES,
	DJGT__GLPSS_ALIGNMENT,
	DJGT__GLPSS_COUNT
};
// OpenGL pixel store state
typedef GLint djgt__glpss[DJGT__GLPSS_COUNT]; 

static void djgt__get_glpps(djgt__glpss state)
{
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, state    );
	glGetIntegerv(GL_PACK_SWAP_BYTES          , state + 1);
	glGetIntegerv(GL_PACK_LSB_FIRST           , state + 2);
	glGetIntegerv(GL_PACK_ROW_LENGTH          , state + 3);
	glGetIntegerv(GL_PACK_IMAGE_HEIGHT        , state + 4);
	glGetIntegerv(GL_PACK_SKIP_ROWS           , state + 5);
	glGetIntegerv(GL_PACK_SKIP_PIXELS         , state + 6);
	glGetIntegerv(GL_PACK_SKIP_IMAGES         , state + 7);
	glGetIntegerv(GL_PACK_ALIGNMENT           , state + 8);
}

static void djgt__get_glpus(djgt__glpss state)
{
	glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, state    );
	glGetIntegerv(GL_UNPACK_SWAP_BYTES          , state + 1);
	glGetIntegerv(GL_UNPACK_LSB_FIRST           , state + 2);
	glGetIntegerv(GL_UNPACK_ROW_LENGTH          , state + 3);
	glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT        , state + 4);
	glGetIntegerv(GL_UNPACK_SKIP_ROWS           , state + 5);
	glGetIntegerv(GL_UNPACK_SKIP_PIXELS         , state + 6);
	glGetIntegerv(GL_UNPACK_SKIP_IMAGES         , state + 7);
	glGetIntegerv(GL_UNPACK_ALIGNMENT           , state + 8);
}

static void djgt__set_glpps(const djgt__glpss state)
{
	glBindBuffer(GL_PIXEL_PACK_BUFFER , state[0]);
	glPixelStorei(GL_PACK_SWAP_BYTES  , state[1]);
	glPixelStorei(GL_PACK_LSB_FIRST   , state[2]);
	glPixelStorei(GL_PACK_ROW_LENGTH  , state[3]);
	glPixelStorei(GL_PACK_IMAGE_HEIGHT, state[4]);
	glPixelStorei(GL_PACK_SKIP_ROWS   , state[5]);
	glPixelStorei(GL_PACK_SKIP_PIXELS , state[6]);
	glPixelStorei(GL_PACK_SKIP_IMAGES , state[7]);
	glPixelStorei(GL_PACK_ALIGNMENT   , state[8]);
}

static void djgt__set_glpus(const djgt__glpss state)
{
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER , state[0]);
	glPixelStorei(GL_UNPACK_SWAP_BYTES  , state[1]);
	glPixelStorei(GL_UNPACK_LSB_FIRST   , state[2]);
	glPixelStorei(GL_UNPACK_ROW_LENGTH  , state[3]);
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, state[4]);
	glPixelStorei(GL_UNPACK_SKIP_ROWS   , state[5]);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS , state[6]);
	glPixelStorei(GL_UNPACK_SKIP_IMAGES , state[7]);
	glPixelStorei(GL_UNPACK_ALIGNMENT   , state[8]);
}

static bool djgt__validate(void)
{
	bool nerr = (glGetError() == GL_NO_ERROR);

	do {} while (glGetError() != GL_NO_ERROR);

	return nerr;
}

#define djgt__glformat(T) \
	(T->comp == 1 ? GL_RED :\
		T->comp == 2 ? GL_RG :\
			T->comp == 3 ? GL_RGB :\
				GL_RGBA)

#define djgt__gltype(T) \
	(T->hdr ? GL_FLOAT : GL_UNSIGNED_BYTE)

static bool
djgt__glTexSubImage1D(const djg_texture *texture, GLenum target, GLint xoffset)
{
	djgt__glpss pus = {0, 0, 0, 0, 0, 0 ,0, 0, 2};
	djgt__glpss backup;
	bool reset;

	djgt__get_glpus(backup);
	reset = memcmp(pus, backup, sizeof(pus)) != 0;
	if (reset) djgt__set_glpus(pus);
	glTexSubImage1D(target,
	       /*level*/0,
	                xoffset,
	                texture->x,
	                djgt__glformat(texture),
	                djgt__gltype(texture),
	                texture->texels);
	if (reset) djgt__set_glpus(backup);

	return djgt__validate();
}

static bool
djgt__glTexSubImage2D(
	const djg_texture *texture,
	GLenum target,
	GLint xoffset,
	GLint yoffset
) {
	djgt__glpss pus = {0, 0, 0, 0, 0, 0 ,0, 0, 2};
	djgt__glpss backup;
	bool reset;

	djgt__get_glpus(backup);
	reset = memcmp(pus, backup, sizeof(djgt__glpss)) != 0;
	if (reset) djgt__set_glpus(pus);
	glTexSubImage2D(target,
	       /*level*/0,
	                xoffset,
	                yoffset,
	                texture->x,
	                texture->y,
	                djgt__glformat(texture),
	                djgt__gltype(texture),
	                texture->texels);
	if (reset) djgt__set_glpus(backup);

	return djgt__validate();
}

static bool
djgt__glTexSubImage3D(
	const djg_texture *texture,
	GLenum target,
	GLint xoffset,
	GLint yoffset,
	GLint zoffset
) {
	djgt__glpss pus = {0, 0, 0, 0, 0, 0 ,0, 0, 2};
	djgt__glpss backup;
	bool reset;

	djgt__get_glpus(backup);
	reset = memcmp(pus, backup, sizeof(djgt__glpss)) != 0;
	if (reset) djgt__set_glpus(pus);
	glTexSubImage3D(target,
	       /*level*/0,
	                xoffset,
	                yoffset,
	                zoffset,
	                texture->x,
	                texture->y,
	       /*depth*/1,
	                djgt__glformat(texture),
	                djgt__gltype(texture),
	                texture->texels);
	if (reset) djgt__set_glpus(backup);

	return djgt__validate();
}

DJGDEF bool
djgt_gl_upload(
	const djg_texture *texture,
	GLint target,
	GLint internalformat,
	bool immutable,
	bool mipmap,
	GLuint *gl
) {
	bool v = true;
	GLuint glt;

	DJG_ASSERT(texture && gl);
	djgt__validate(); // flush previous OpenGL errors

	if (!djgt__count(texture)) return false;
	glGenTextures(1, &glt);
	glBindTexture(target, glt);
	switch (target) {
		/*************************************************************/
		/***/
		/***/
		// 1D
		case GL_TEXTURE_1D:
		case GL_PROXY_TEXTURE_1D: {
			int x = texture->next->x;

#ifndef NGL_ARB_texture_storage
			if (immutable) {
				glTexStorage1D(target,
				               djgt__mipcnt(x, 0, 0),
				               internalformat,
				               x);
			} else
#endif // NGL_ARB_texture_storage
			{
				glTexImage1D(target,
				    /*level*/0,
				             internalformat,
				             x,
				   /*border*/0,
				    /*dummy*/GL_RED, GL_UNSIGNED_BYTE, NULL);
			}
			djgt__glTexSubImage1D(texture->next, target, /*xoffset*/0);
		} break;

		/*************************************************************/
		/***/
		/***/
		// 1D_ARRAY
		case GL_TEXTURE_1D_ARRAY:
		case GL_PROXY_TEXTURE_1D_ARRAY: {
			int x = texture->next->x;
			int i = 0;
			const djg_texture *it = texture->next;

#ifndef NGL_ARB_texture_storage
			if (immutable) {
				glTexStorage2D(target,
				               djgt__mipcnt(x, 0, 0),
				               internalformat,
				               x,
				               djgt__count(texture));
			} else
#endif // NGL_ARB_texture_storage
			{
				glTexImage2D(target,
				    /*level*/0,
				             internalformat,
				             x,
				             djgt__count(texture),
				   /*border*/0,
				    /*dummy*/GL_RED, GL_UNSIGNED_BYTE, NULL);
			}
			while (it) {
				it = it->next;
				v&= djgt__glTexSubImage2D(texture->next,
				                          target,
				               /*offsets*/0, i);
				++i;
			}
		} break;

		/*************************************************************/
		/***/
		/***/
		// 2D
		case GL_TEXTURE_2D:
		case GL_TEXTURE_RECTANGLE:
		case GL_PROXY_TEXTURE_2D:
		case GL_PROXY_TEXTURE_RECTANGLE: {
			int x = texture->next->x;
			int y = texture->next->y;
#ifndef NGL_ARB_texture_storage
			if (immutable) {
				glTexStorage2D(target,
				               djgt__mipcnt(x, y, 0),
				               internalformat,
				               x,
				               y);
			} else
#endif // NGL_ARB_texture_storage
			{
				glTexImage2D(target,
				    /*level*/0,
				             internalformat,
				             x,
				             y,
				   /*border*/0,
				    /*dummy*/GL_RED, GL_UNSIGNED_BYTE, NULL);
			}
			v&= djgt__glTexSubImage2D(texture->next, target, /*offsets*/0, 0);
		} break;

		/*************************************************************/
		/***/
		/***/
		// CUBEMAP
		case GL_TEXTURE_CUBE_MAP: {
			int x = texture->next->x;
			int y = texture->next->y;
			const djg_texture *it = texture->next;
			int i;

#ifndef NGL_ARB_texture_storage
			if (immutable) {
				glTexStorage2D(target,
				               djgt__mipcnt(x, y, 0),
				               internalformat,
				               x,
				               y);
			} else
#endif // NGL_ARB_texture_storage
			{
				for (i = 0; i < 6; ++i)
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					             0, internalformat, x, y, 0,
					    /*dummy*/GL_RED, GL_UNSIGNED_BYTE, NULL);
			}
			for (i = 0; i < 6 && it && v; ++i) {
				v&= djgt__glTexSubImage2D(it,
				                          GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				               /*offsets*/0, 0);
				it = it->next;
			}
		} break;

		/*************************************************************/
		/***/
		/***/
		// 2D_ARRAY
		case GL_TEXTURE_2D_ARRAY:
		case GL_PROXY_TEXTURE_2D_ARRAY: {
			int x = texture->next->x;
			int y = texture->next->y;
			int z = djgt__count(texture);
			const djg_texture *it = texture->next;
			int i;

#ifndef NGL_ARB_texture_storage
			if (immutable) {
				glTexStorage3D(target,
				               djgt__mipcnt(x, y, 0),
				               internalformat,
				               x,
				               y,
				               z);
			} else
#endif // NGL_ARB_texture_storage
			{
				glTexImage3D(target, 0, internalformat, x, y, z, 0,
				    /*dummy*/GL_RED, GL_UNSIGNED_BYTE, NULL);
			}
			for (i = 0; i < z && v; ++i) {
				v&= djgt__glTexSubImage3D(it,
				                          target,
				               /*offsets*/0, 0, i);
				it = it->next;
			}
		} break;

		/*************************************************************/
		/***/
		/***/
		// 3D
		case GL_TEXTURE_3D:
		case GL_PROXY_TEXTURE_3D: {
			int x = texture->next->x;
			int y = texture->next->y;
			int z = djgt__count(texture);
			const djg_texture *it = texture->next;
			int i;

#ifndef NGL_ARB_texture_storage
			if (immutable) {
				glTexStorage3D(target,
				               djgt__mipcnt(x, y, z),
				               internalformat,
				               x,
				               y,
				               z);
			} else
#endif // NGL_ARB_texture_storage
			{
				glTexImage3D(target, 0, internalformat, x, y, z, 0,
				    /*dummy*/GL_RED, GL_UNSIGNED_BYTE, NULL);
			}
			for (i = 0; i < z && v; ++i) {
				v&= djgt__glTexSubImage3D(it,
				                          target,
				               /*offsets*/0, 0, z);
				it = it->next;
			}
		} break;

		/*************************************************************/
		/***/
		/***/
		// Error
		default:
			DJG_LOG("djg_error: Unsupported GL texture target\n");

			return false;
			break;
	}

	if (mipmap && v) glGenerateMipmap(target);

	if (!(v&= djgt__validate())) {
		DJG_LOG("djg_error: Caught OpenGL error\n");

		return false;
	}

	if (glIsTexture(*gl)) glDeleteTextures(1, gl);
	*gl = glt;

	return true;
}


DJGDEF bool
djgt_push_glcolorbuffer(
	djg_texture *texture,
	GLenum glbuffer,
	GLenum glformat,
	GLenum gltype,
	bool flipy
) {
	DJG_ASSERT(texture);
	djgt__glpss pus = {0, 0, 0, 0, 0, 0 ,0, 0, 2};
	djg_texture *tail = djgt_create(0);
	djgt__glpss pss;
	GLint read_buffer;
	GLint v[4];

	// init members
	glGetIntegerv(GL_VIEWPORT, v);
	glGetIntegerv(GL_READ_BUFFER, &read_buffer);
	tail->x = v[2];
	tail->y = v[3];
	switch (glformat) {
		case GL_RED : tail->comp = 1; break;
		case GL_RG  : tail->comp = 2; break;
		case GL_RGB : tail->comp = 3; break;
		case GL_RGBA: tail->comp = 4; break;
		default:
			DJG_LOG("djg_error: Unsupported OpenGL format\n");
			djgt_release(tail);

			return false;
	}
	switch (gltype) {
		case GL_UNSIGNED_BYTE:
			tail->texels = (char *)DJG_MALLOC(tail->x * tail->y * tail->comp);
			break;
		case GL_FLOAT:
			tail->texels = (char *)DJG_MALLOC(sizeof(float) * tail->x * tail->y * tail->comp);
			tail->hdr = true;
			break;
		default:
			DJG_LOG("djg_error: Unsupported OpenGL type\n");
			djgt_release(tail);

			return false;
	}

	// fill texels and push texture
	djgt__get_glpps(pss);
	djgt__set_glpps(pus);
	glReadBuffer(glbuffer);
	glReadPixels(v[0], v[1], v[2], v[3], glformat, gltype, tail->texels);
	glReadBuffer(read_buffer);
	djgt__set_glpps(pss);

	if (djgt__validate())
		djgt__push_texture(texture, tail, flipy);
	else
		return false;

	return true;
}

#endif // STBI_INCLUDE_STB_IMAGE_H

// *************************************************************************************************
// Texture Save API Implementation

#ifdef INCLUDE_STB_IMAGE_WRITE_H

#define DJGT__CAT(x, y) x##y

#define DJGT__SAVE_GL_COLOR_BUFFER_IMPL(type)                               \
DJGDEF bool                                                                 \
DJGT__CAT(djgt_save_glcolorbuffer_, type)(                                  \
    GLenum buffer,                                                          \
    GLenum format,                                                          \
    const char *filename                                                    \
) {                                                                         \
    djg_texture *head = djgt_create(0);                                     \
    int r;                                                                  \
                                                                            \
    r = djgt_push_glcolorbuffer(head, buffer, format, GL_UNSIGNED_BYTE, 1); \
    if (!r) {                                                               \
        djgt_release(head);                                                 \
                                                                            \
        return false;                                                       \
    }                                                                       \
                                                                            \
    r = DJGT__CAT(djgt_save_,type)(head, filename);                         \
    djgt_release(head);                                                     \
                                                                            \
    return r;                                                               \
}

DJGT__SAVE_GL_COLOR_BUFFER_IMPL(bmp)
DJGT__SAVE_GL_COLOR_BUFFER_IMPL(png)

DJGDEF bool djgt_save_bmp(const djg_texture *texture, const char *filename)
{
	const djg_texture* it = texture->next;
	int cnt = djgt__count(texture);
	char buf[1024];

	if (cnt == 1) {
		sprintf(buf, "%s.bmp", filename);

		return stbi_write_bmp(buf, it->x, it->y, it->comp, it->texels);
	} else {
		int i = 1;

		while (it) {
			sprintf(buf, "%s_layer%03i.bmp", filename, i);

			++i;
			if (!stbi_write_bmp(buf, it->x, it->y, it->comp, it->texels))
				return false;
		}
	}

	return true;
}

DJGDEF bool djgt_save_png(const djg_texture *texture, const char *filename)
{
	const djg_texture* it = texture->next;
	int cnt = djgt__count(texture);
	char buf[1024];

	if (cnt == 1) {
		sprintf(buf, "%s.png", filename);

		return stbi_write_png(buf, it->x, it->y, it->comp, it->texels, 0);
	} else {
		int i = 1;

		while (it) {
			sprintf(buf, "%s_layer%03i.png", filename, i);

			++i;
			if (!stbi_write_png(buf, it->x, it->y, it->comp, it->texels, 0))
				return false;
		}
	}

	return true;
}

#endif // INCLUDE_STB_IMAGE_WRITE_H

// *************************************************************************************************
// Mesh API Implementation

typedef struct djg_mesh {
	djgm_vertex *vertexv;
	uint16_t *poly3v; // triangles
	uint16_t *poly4v; // quads
	int32_t vertexc, poly3c, poly4c;
} djg_mesh;

static djg_mesh *djgm__create(void)
{
	djg_mesh *mesh = (djg_mesh *)DJG_MALLOC(sizeof(*mesh));

	DJG_ASSERT(mesh);
	mesh->vertexv = NULL;
	mesh->poly3v = NULL;
	mesh->poly4v = NULL;
	mesh->vertexc = 0;
	mesh->poly3c = 0;
	mesh->poly4c = 0;

	return mesh;
}

static bool
djgm__load_plane_vertices(
	djg_mesh *mesh,
	float width, float height,
	int slices, int stacks
) {
	djgm_vertex *vv;
	int32_t x, z, vc;

	slices+= 2;
	stacks+= 2;
	vc = slices * stacks;

	if (vc > 0xFFFF) {
		DJG_LOG("djg_error: Too many vertices\n");

		return false;
	}

	vv = (djgm_vertex *)DJG_MALLOC(sizeof(*vv) * vc);

	for (x = 0; x < slices; ++x)
	for (z = 0; z < stacks; ++z) {
		djgm_vertex *v = &vv[x * stacks + z];

		v->st.s = (float)x / (slices - 1);
		v->st.t = (float)z / (stacks - 1);
		v->st.p = 0;
		v->st.q = 0;

		v->p.x = (v->st.s - 0.5f) * width;
		v->p.y = (v->st.t - 0.5f) * height;
		v->p.z = 0;
		v->p.w = 1.0;

		v->dpds.x = 1.0;
		v->dpds.y = 0.0;
		v->dpds.z = 0.0;
		v->dpds.w = 0.0;

		v->dpdt.x = 0.0;
		v->dpdt.y = 1.0;
		v->dpdt.z = 0.0;
		v->dpdt.w = 0.0;
	}
	
	mesh->vertexv = vv;
	mesh->vertexc = vc;

	return true;
}

static bool djgm__load_plane_polygons(djg_mesh *mesh, int slices, int stacks)
{
	uint16_t *p3v, *p4v;
	int32_t i, j, p3c, p4c;

	++slices;
	++stacks;

	p3c = /* quad count */slices * stacks 
	    * /* triangles per quad */2 
	    * /* indexes per triangle*/3;
	p4c = /* quad count */slices * stacks 
	    * /* indexes per quad */4;
	p3v = (uint16_t *)DJG_MALLOC(sizeof(*p3v) * p3c);
	p4v = (uint16_t *)DJG_MALLOC(sizeof(*p4v) * p4c);

	// build triangles
	for (j = 0; j < stacks; ++j)
	for (i = 0; i < slices; ++i) {
		uint16_t *p3 = &p3v[2 * 3 * (j * slices + i)];

		// upper triangle
		p3[0] = j     + (stacks + 1) *  i;
		p3[1] = j     + (stacks + 1) * (i + 1);
		p3[2] = j + 1 + (stacks + 1) *  i;

		p3+= 3;

		// lower triangle
		p3[0] = j + 1 + (stacks + 1) *  i;
		p3[1] = j     + (stacks + 1) * (i + 1);
		p3[2] = j + 1 + (stacks + 1) * (i + 1);
	}
	
	// build quads
	for (j = 0; j < stacks; ++j)
	for (i = 0; i < slices; ++i) {
		uint16_t *p4 = &p4v[4 * (j * slices + i)];

		p4[0] = j     + (stacks + 1) *  i;
		p4[1] = j     + (stacks + 1) * (i + 1);
		p4[2] = j + 1 + (stacks + 1) * (i + 1);
		p4[3] = j + 1 + (stacks + 1) *  i;
	}

	mesh->poly3c = p3c;
	mesh->poly4c = p4c;
	mesh->poly3v = p3v;
	mesh->poly4v = p4v;

	return true;
}

DJGDEF djg_mesh *
djgm_load_plane(float width, float height, int slices, int stacks)
{
	djg_mesh *mesh = djgm__create();

	DJG_ASSERT(slices >= 0 && stacks >= 0);
	DJG_ASSERT(width > 0.0 && height > 0.0);
	if (!djgm__load_plane_vertices(mesh, width, height, slices, stacks)) {
		DJG_FREE(mesh);

		return NULL;
	}
	if (!djgm__load_plane_polygons(mesh, slices, stacks)) {
		DJG_FREE(mesh->vertexv);
		DJG_FREE(mesh);

		return NULL;
	}

	return mesh;
}

DJGDEF djg_mesh *djgm_load_disk(float radius, int slices, int stacks)
{
	djg_mesh *mesh = djgm_load_plane(2.f, 2.f, slices, stacks);
	int i, j;

	DJG_ASSERT(radius > 0.0);
	slices+= 2;
	stacks+= 2;

	for (i = 0; i < slices; ++i)
	for (j = 0; j < stacks; ++j) {
		djgm_vertex *v = &mesh->vertexv[i * stacks + j];
		float r1 = 2.f * v->st.s - 1.f;
		float r2 = 2.f * v->st.t - 1.f;
		float r, phi;

		/* Modified concencric map code with less branching (by Dave Cline), see
		   http://psgraphics.blogspot.ch/2011/01/improved-code-for-concentric-map.html */
		if (r1 == 0 && r2 == 0) {
			r = phi = 0;
		} else if (r1 * r1 > r2 * r2) {
			r = r1;
			phi = (M_PI / 4.0f) * (r2 / r1);
		} else {
			r = r2;
			phi = (M_PI / 2.0f) - (r1 / r2) * (M_PI / 4.0f);
		}
		v->p.x = radius * r * cos(phi);
		v->p.y = radius * r * sin(phi);
		v->p.z = 0.f;

		v->dpds.x = cos(phi);
		v->dpds.y = sin(phi);
		v->dpds.z = 0.f;

		v->dpdt.x = -r * sin(phi);
		v->dpdt.y =  r * cos(phi);
		v->dpdt.z = 0.f;
	}

	return mesh;
}

DJGDEF djg_mesh *djgm_load_sphere(float radius, int slices, int stacks)
{
	djg_mesh *mesh = djgm_load_plane(1.f, 1.f, slices, stacks);
	int i, j;

	DJG_ASSERT(radius > 0.0);
	slices+= 2;
	stacks+= 2;

	for (i = 0; i < slices; ++i)
	for (j = 0; j < stacks; ++j) {
		djgm_vertex *v = &mesh->vertexv[i * stacks + j];
		float theta = v->st.s * M_PI;
		float phi = v->st.t * 2.f * M_PI;
		float tmp = v->st.s;

		v->st.s = v->st.t;
		v->st.t = tmp;
		v->st.p = 0;
		v->st.q = 0;

		v->p.x = radius * sin(theta) * cos(phi);
		v->p.y = radius * sin(theta) * sin(phi);
		v->p.z = radius * cos(theta);

		v->dpds.x = cos(theta) * cos(phi);
		v->dpds.y = cos(theta) * sin(phi);
		v->dpds.z = -sin(theta);

		v->dpdt.x = -sin(phi);
		v->dpdt.y = cos(phi);
		v->dpdt.z = 0.f;

	}

	return mesh;
}

DJGDEF djg_mesh *
djgm_load_cylinder(float radius, float height, int slices, int stacks)
{
	djg_mesh *mesh = djgm_load_plane(1.f, 1.f, slices, stacks);;
	int i, j;

	DJG_ASSERT(radius > 0.0 && height > 0.0);
	slices+= 2;
	stacks+= 2;
	
	for (i = 0; i < slices; ++i)
	for (j = 0; j < stacks; ++j) {
		djgm_vertex *v = &mesh->vertexv[i * stacks + j];
		float z = (v->st.t - 0.5) * height;
		float phi = v->st.s * 2.f * M_PI;

		v->p.x = radius * cos(phi);
		v->p.y = z;
		v->p.z = -radius * sin(phi);

		v->dpds.x = -sin(phi);
		v->dpds.y = 0.0;
		v->dpds.z = -cos(phi);

		v->dpdt.x = 0.0;
		v->dpdt.y = 1.0;
		v->dpdt.z = 0.0;
	}

	return mesh;
}

DJGDEF djg_mesh *
djgm_load_torus(
	float ring_radius, int ring_segments,
	float pipe_radius, int pipe_segments
) {
	djg_mesh *mesh = djgm_load_plane(1.f, 1.f, ring_segments, pipe_segments);
	int i, j;

	DJG_ASSERT(ring_radius > 0.0 && pipe_radius > 0.0);
	ring_segments+= 2;
	pipe_segments+= 2;
	
	for (i = 0; i < ring_segments; ++i)
	for (j = 0; j < pipe_segments; ++j) {
		djgm_vertex *v = &mesh->vertexv[i * pipe_segments + j];
		float theta = (1.f - v->st.s) * 2.f * M_PI;
		float phi = (1.f - v->st.t) * 2.f * M_PI;
		float tx, ty, tn;

		v->p.x = cos(theta) * (pipe_radius * cos(phi) + ring_radius);
		v->p.y = sin(theta) * (pipe_radius * cos(phi) + ring_radius);
		v->p.z = pipe_radius * sin(phi);

		// tangent (requires normalization)
		tx = -sin(theta) * (pipe_radius * cos(phi) + ring_radius);
		ty = cos(theta) * (pipe_radius * cos(phi) + ring_radius);
		tn = 1.f / sqrtf(tx * tx + ty * ty);
		v->dpds.x = tx * tn;
		v->dpds.y = ty * tn;
		v->dpds.z = 0.f;

		// bitangent
		v->dpdt.x = -cos(theta) * sin(phi);
		v->dpdt.y = -sin(theta) * sin(phi);
		v->dpdt.z = cos(phi);
	}

	return mesh;
}

DJGDEF void djgm_release(djg_mesh *mesh)
{
	DJG_ASSERT(mesh);
	DJG_FREE(mesh->vertexv);
	DJG_FREE(mesh->poly3v);
	DJG_FREE(mesh->poly4v);
	DJG_FREE(mesh);
}

DJGDEF const uint16_t *djgm_get_triangles(const djg_mesh *mesh, GLint *count)
{
	DJG_ASSERT(mesh);
	if (count) *count = mesh->poly3c;

	return mesh->poly3v;
}

DJGDEF const uint16_t *djgm_get_quads(const djg_mesh *mesh, GLint *count)
{
	DJG_ASSERT(mesh);
	if (count) *count = mesh->poly4c;

	return mesh->poly4v;
}

DJGDEF const djgm_vertex *djgm_get_vertices(const djg_mesh *mesh, GLint *count)
{
	DJG_ASSERT(mesh);
	if (count) *count = mesh->vertexc;

	return mesh->vertexv;
}

DJGDEF bool
djgm_export_obj_triangles(const djg_mesh *mesh, const char *filename)
{
	DJG_ASSERT(mesh);
	int i;
	FILE *pf;

	pf = fopen(filename, "w");
	if (!pf) {
		DJG_LOG("djg_error: fopen failed\n");
		return false;
	}
	fprintf(pf, "# Created by the DJGM library (see dj_opengl.h)\n\n");

	// write vertices
	fprintf(pf, "# Vertices\n");
	for (i = 0; i < mesh->vertexc; ++i) {
		const djgm_vertex *v = &mesh->vertexv[i];
		float n[3], nrm;

		// compute the normal from tangent cross product
		n[0] = v->dpds.y * v->dpdt.z - v->dpds.z * v->dpdt.y;
		n[1] = v->dpds.z * v->dpdt.x - v->dpds.x * v->dpdt.z;
		n[2] = v->dpds.x * v->dpdt.y - v->dpds.y * v->dpdt.x;
		nrm = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);

		DJG_ASSERT(nrm > 0.0f && "Invalid normal");
		fprintf(pf, "v %f %f %f\n", v->p.x, v->p.y, v->p.z);
		fprintf(pf, "vn %f %f %f\n", n[0] / nrm, n[1] / nrm, n[2] / nrm);
		fprintf(pf, "vt %f %f\n", v->st.s, v->st.t);
	}

	// write topology
	fprintf(pf, "# Topology\n");
	for (i = 0; i < mesh->poly3c / 3; ++i) {
		fprintf(pf, "f %i/%i/%i ", (int32_t)mesh->poly3v[3*i  ] + 1,
		                           (int32_t)mesh->poly3v[3*i  ] + 1,
		                           (int32_t)mesh->poly3v[3*i  ] + 1);
		fprintf(pf, "%i/%i/%i ", (int32_t)mesh->poly3v[3*i+1] + 1,
		                         (int32_t)mesh->poly3v[3*i+1] + 1,
		                         (int32_t)mesh->poly3v[3*i+1] + 1);
		fprintf(pf, "%i/%i/%i\n", (int32_t)mesh->poly3v[3*i+2] + 1,
		                          (int32_t)mesh->poly3v[3*i+2] + 1,
		                          (int32_t)mesh->poly3v[3*i+2] + 1);
	}

	fclose(pf);

	return true;
}

// *************************************************************************************************
// Font API Implementation
enum {
	DJGF__UNIFORM_SIZE,
	DJGF__UNIFORM_COLOR,
	DJGF__UNIFORM_COUNT
};

// Internal constants
#define DJGF__BUFFER_CAPACITY (1<<23)
#define DJGF__BUFFER_SIZE   1024
#define DJGF__BITMAP_WIDTH  1024
#define DJGF__BITMAP_HEIGHT    8
#define DJGF__BITMAP_CHAR_SIZE 7
#define DJGF__BITMAP_TAB_SIZE  (1<<4)
#define DJGF__BITMAP_DATA { \
	68, 3, 64, 3, 10, 3, 46, 3, 46, 3, 190, 3, 80, 5, 254, 16, 3, 156, 5, 16, \
	3, 8, 5, 26, 13, 120, 5, 42, 5, 92, 3, 20, 3, 118, 5, 34, 3, 12, 3, 12,  \
	3, 32, 5, 254, 254, 32, 3, 44, 7, 10, 3, 4, 3, 8, 7, 2, 3, 24, 3, 14,  \
	3, 46, 3, 30, 3, 12, 3, 14, 7, 12, 3, 10, 11, 8, 7, 14, 3, 10, 7, 10,  \
	7, 12, 3, 12, 7, 10, 5, 14, 3, 14, 3, 62, 3, 12, 3, 12, 3, 6, 3, 6,  \
	9, 10, 7, 8, 7, 12, 9, 8, 3, 14, 7, 8, 3, 6, 3, 10, 3, 12, 7, 8,  \
	3, 6, 3, 8, 9, 6, 3, 6, 3, 6, 3, 6, 3, 8, 7, 8, 3, 16, 7, 8,  \
	3, 6, 3, 8, 7, 12, 3, 12, 7, 12, 3, 12, 3, 2, 3, 8, 3, 6, 3, 10,  \
	3, 12, 9, 10, 3, 18, 3, 10, 3, 42, 9, 12, 7, 8, 7, 12, 5, 12, 7, 10,  \
	5, 12, 3, 18, 3, 8, 3, 4, 3, 10, 3, 16, 3, 10, 3, 4, 3, 12, 3, 8,  \
	3, 2, 3, 2, 3, 8, 3, 4, 3, 10, 5, 10, 3, 20, 3, 8, 3, 14, 7, 14,  \
	3, 12, 5, 12, 3, 12, 3, 2, 3, 8, 3, 6, 3, 10, 3, 12, 9, 10, 3, 14,  \
	3, 14, 3, 28, 3, 4, 3, 254, 254, 74, 3, 2, 3, 2, 3, 8, 3, 2, 3, 2,  \
	3, 4, 3, 6, 3, 24, 3, 18, 3, 28, 3, 14, 3, 46, 3, 10, 3, 6, 3, 10,  \
	3, 12, 3, 12, 3, 6, 3, 12, 3, 8, 3, 6, 3, 6, 3, 6, 3, 10, 3, 10,  \
	3, 6, 3, 12, 3, 28, 3, 16, 3, 26, 3, 28, 3, 2, 7, 6, 3, 6, 3, 6,  \
	3, 6, 3, 6, 3, 6, 3, 6, 3, 4, 3, 10, 3, 14, 3, 12, 3, 6, 3, 6,  \
	3, 6, 3, 10, 3, 10, 3, 6, 3, 6, 3, 4, 3, 10, 3, 12, 3, 6, 3, 6,  \
	3, 4, 5, 6, 3, 6, 3, 6, 3, 14, 3, 2, 3, 2, 3, 6, 3, 4, 3, 8,  \
	3, 6, 3, 10, 3, 10, 3, 6, 3, 10, 3, 12, 3, 2, 3, 8, 3, 6, 3, 10,  \
	3, 12, 3, 16, 3, 16, 3, 12, 3, 44, 3, 4, 3, 8, 3, 4, 3, 8, 3, 4,  \
	3, 8, 3, 4, 3, 8, 3, 4, 3, 8, 3, 16, 3, 14, 7, 8, 3, 4, 3, 10,  \
	3, 16, 3, 10, 3, 2, 3, 12, 3, 10, 3, 2, 3, 2, 3, 8, 3, 4, 3, 8,  \
	3, 4, 3, 8, 7, 12, 7, 8, 3, 20, 3, 10, 3, 12, 3, 4, 3, 10, 3, 12,  \
	3, 2, 3, 10, 3, 2, 3, 12, 3, 12, 3, 16, 3, 14, 3, 14, 3, 26, 3, 2,  \
	5, 2, 3, 254, 254, 28, 3, 26, 3, 2, 3, 14, 3, 2, 3, 10, 5, 2, 3, 4,  \
	3, 6, 3, 24, 3, 18, 3, 8, 3, 6, 3, 10, 3, 62, 3, 10, 5, 4, 3, 10,  \
	3, 14, 3, 18, 3, 6, 11, 14, 3, 6, 3, 6, 3, 10, 3, 10, 3, 6, 3, 14,  \
	3, 42, 3, 10, 11, 10, 3, 14, 3, 10, 5, 2, 3, 2, 3, 4, 11, 6, 3, 6,  \
	3, 6, 3, 14, 3, 6, 3, 8, 3, 14, 3, 12, 3, 6, 3, 6, 3, 6, 3, 10,  \
	3, 10, 3, 6, 3, 6, 3, 2, 3, 12, 3, 12, 3, 6, 3, 6, 3, 4, 5, 6,  \
	3, 6, 3, 6, 3, 14, 3, 6, 3, 6, 3, 2, 3, 18, 3, 10, 3, 10, 3, 6,  \
	3, 8, 3, 2, 3, 10, 3, 2, 3, 10, 3, 2, 3, 12, 3, 12, 3, 16, 3, 16,  \
	3, 12, 3, 44, 3, 16, 7, 8, 3, 4, 3, 8, 3, 14, 3, 4, 3, 8, 9, 10,  \
	3, 12, 3, 4, 3, 8, 3, 4, 3, 10, 3, 16, 3, 10, 5, 14, 3, 10, 3, 2,  \
	3, 2, 3, 8, 3, 4, 3, 8, 3, 4, 3, 8, 3, 4, 3, 8, 3, 4, 3, 8,  \
	3, 16, 5, 12, 3, 12, 3, 4, 3, 8, 3, 2, 3, 8, 3, 2, 3, 2, 3, 10,  \
	3, 12, 3, 2, 3, 12, 3, 14, 3, 30, 3, 26, 5, 6, 3, 254, 254, 28, 3, 26,  \
	11, 8, 7, 10, 9, 8, 3, 2, 3, 2, 3, 22, 3, 18, 3, 10, 3, 2, 3, 8,  \
	11, 24, 9, 28, 3, 8, 3, 2, 3, 2, 3, 10, 3, 16, 3, 10, 7, 8, 3, 4,  \
	3, 16, 3, 6, 9, 14, 3, 10, 7, 10, 9, 40, 3, 34, 3, 14, 3, 8, 5, 2,  \
	3, 2, 3, 6, 3, 2, 3, 8, 9, 8, 3, 14, 3, 6, 3, 8, 7, 10, 7, 8,  \
	3, 4, 5, 6, 11, 10, 3, 18, 3, 6, 5, 14, 3, 12, 3, 6, 3, 6, 3, 2,  \
	3, 2, 3, 6, 3, 6, 3, 6, 9, 8, 3, 6, 3, 6, 9, 10, 7, 12, 3, 10,  \
	3, 6, 3, 8, 3, 2, 3, 8, 3, 2, 3, 2, 3, 10, 3, 14, 3, 14, 3, 14,  \
	3, 14, 3, 14, 3, 42, 7, 18, 3, 8, 3, 4, 3, 8, 3, 4, 3, 8, 3, 4,  \
	3, 8, 3, 4, 3, 10, 3, 12, 3, 4, 3, 8, 3, 4, 3, 10, 3, 16, 3, 10,  \
	3, 2, 3, 12, 3, 10, 3, 2, 3, 2, 3, 8, 3, 4, 3, 8, 3, 4, 3, 8,  \
	3, 4, 3, 8, 3, 4, 3, 8, 5, 12, 3, 16, 3, 12, 3, 4, 3, 8, 3, 2,  \
	3, 8, 3, 6, 3, 8, 3, 2, 3, 10, 3, 2, 3, 14, 3, 10, 3, 34, 3, 24,  \
	5, 6, 3, 254, 254, 28, 3, 28, 3, 2, 3, 8, 3, 2, 3, 10, 3, 2, 5, 12,  \
	3, 28, 3, 18, 3, 10, 7, 12, 3, 64, 3, 8, 3, 4, 5, 10, 3, 18, 3, 10,  \
	3, 12, 3, 2, 3, 8, 9, 8, 3, 20, 3, 8, 3, 6, 3, 6, 3, 6, 3, 10,  \
	3, 14, 3, 14, 3, 10, 11, 10, 3, 18, 3, 6, 3, 2, 5, 2, 3, 6, 3, 2,  \
	3, 8, 3, 6, 3, 6, 3, 14, 3, 6, 3, 8, 3, 14, 3, 12, 3, 14, 3, 6,  \
	3, 10, 3, 18, 3, 6, 3, 2, 3, 12, 3, 12, 3, 2, 3, 2, 3, 6, 3, 2,  \
	3, 2, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6,  \
	3, 18, 3, 10, 3, 6, 3, 6, 3, 6, 3, 6, 3, 2, 3, 2, 3, 8, 3, 2,  \
	3, 10, 3, 2, 3, 14, 3, 12, 3, 14, 3, 14, 3, 44, 3, 16, 5, 10, 7, 12,  \
	5, 12, 7, 10, 5, 10, 7, 12, 7, 8, 7, 12, 3, 16, 3, 10, 3, 4, 3, 10,  \
	3, 10, 5, 2, 3, 10, 7, 12, 5, 10, 7, 12, 7, 8, 3, 2, 5, 10, 7, 8,  \
	7, 10, 3, 4, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6,  \
	3, 8, 9, 10, 3, 14, 3, 14, 3, 26, 3, 2, 5, 2, 3, 254, 254, 28, 3, 12,  \
	3, 2, 3, 8, 11, 6, 3, 2, 3, 2, 3, 6, 3, 2, 3, 2, 3, 8, 3, 2,  \
	3, 12, 3, 14, 3, 14, 3, 10, 3, 2, 3, 2, 3, 10, 3, 66, 3, 6, 3, 6,  \
	3, 8, 5, 10, 3, 6, 3, 12, 3, 12, 5, 8, 3, 16, 3, 20, 3, 6, 3, 6,  \
	3, 6, 3, 6, 3, 44, 3, 26, 3, 14, 3, 4, 3, 8, 3, 4, 3, 10, 3, 10,  \
	3, 6, 3, 6, 3, 6, 3, 6, 3, 4, 3, 10, 3, 14, 3, 12, 3, 6, 3, 6,  \
	3, 6, 3, 10, 3, 18, 3, 6, 3, 4, 3, 10, 3, 12, 5, 2, 5, 6, 5, 4,  \
	3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6,  \
	3, 10, 3, 10, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6,  \
	3, 6, 3, 14, 3, 10, 3, 12, 3, 16, 3, 12, 3, 2, 3, 26, 3, 30, 3, 36,  \
	3, 26, 3, 28, 3, 46, 3, 16, 3, 126, 3, 110, 3, 14, 3, 14, 3, 10, 3, 2,  \
	5, 10, 3, 4, 3, 254, 254, 30, 3, 12, 3, 2, 3, 12, 3, 2, 3, 8, 7, 10,  \
	9, 10, 3, 14, 3, 16, 3, 10, 3, 16, 3, 82, 3, 8, 7, 12, 3, 12, 7, 8,  \
	11, 12, 3, 8, 11, 10, 5, 8, 11, 8, 7, 10, 7, 92, 5, 12, 5, 12, 3, 10,  \
	9, 10, 7, 8, 7, 12, 9, 8, 9, 8, 7, 8, 3, 6, 3, 10, 3, 18, 3, 6,  \
	3, 6, 3, 8, 3, 12, 3, 6, 3, 6, 5, 4, 3, 8, 7, 8, 9, 10, 7, 8,  \
	9, 10, 7, 8, 11, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6, 3, 6,  \
	3, 6, 3, 6, 3, 8, 9, 10, 5, 10, 3, 14, 5, 14, 3, 30, 5, 26, 3, 36,  \
	3, 28, 5, 24, 3, 16, 3, 16, 3, 10, 3, 16, 3, 126, 3, 112, 3, 12, 3, 12,  \
	3, 14, 5, 2, 3, 10, 5, 254, 254, 12                                \
}

typedef struct djg_font {
	// OpenGL resources
	struct {
		GLint uniforms[DJGF__UNIFORM_COUNT]; // uniform locations
		GLint unit;      // texture unit e.g., GL_TEXTURE0
		GLuint buffer;
		GLuint texture;
		GLuint vertexarray;
		GLuint program;
	} gl;
	// buffer streaming
	struct {
		size_t capacity;
		size_t size;
		size_t offset;
	} buffer;
	// font bitmap sheet information
	struct {
		size_t width;
		size_t height;
		size_t char_size;
		size_t tab_size;
	} bitmap;
	// color
	struct {
		float r, g, b, a;
	} color;
} djg_font;

typedef struct {
	unsigned char letter;
	unsigned char xy[3]; // x,y in NDC
} djgf__vertex; // 4 Bytes

static bool djgf__load_shader(
	djg_font *font,
	GLenum shader,
	GLsizei count,
	const GLchar **srcv
) {
	// To reduce code redundancy, I've accepted a dependence on
	// an internal function of the Program API.
	return djgp__attach_shader(font->gl.program, shader, count, srcv);
}

static void djgf__load_texels(GLubyte *txv)
{
	const unsigned char bitv[] = DJGF__BITMAP_DATA;
	size_t cnt = sizeof(bitv);
	size_t i;

	for(i = 0; i < cnt; ++i) {
		size_t rle = (bitv[i] >> 1u) & 0x7Fu;
		GLint color = (bitv[i] & 1) * 255;

		memset(txv, color, rle);
		txv+= rle;
	}
}

static bool djgf__load_buffer(djg_font *font)
{
	glGenBuffers(1, &font->gl.buffer);
	glBindBuffer(GL_ARRAY_BUFFER, font->gl.buffer);
	glBufferData(GL_ARRAY_BUFFER,
	             font->buffer.capacity,
	             NULL,
	             GL_STREAM_DRAW);
	return (glGetError() == GL_NO_ERROR);
}

static bool djgf__load_vertexarray(djg_font *font)
{
	glGenVertexArrays(1, &font->gl.vertexarray);
	glBindVertexArray(font->gl.vertexarray);
	glBindBuffer(GL_ARRAY_BUFFER, font->gl.buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 0, 0);
	glBindVertexArray(0);
	return (glGetError() == GL_NO_ERROR);
}

static bool djgf__load_texture(djg_font *font)
{
	GLubyte *texelv;
	GLint active;

	texelv = (GLubyte *)DJG_MALLOC(font->bitmap.width * font->bitmap.height);
	djgf__load_texels(texelv);

	glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
	glPixelStorei(GL_UNPACK_LSB_FIRST, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glActiveTexture(GL_TEXTURE0 + font->gl.unit);
	glGenTextures(1, &font->gl.texture);
	glBindTexture(GL_TEXTURE_RECTANGLE, font->gl.texture);
#ifndef NGL_ARB_texture_storage
	glTexStorage2D(GL_TEXTURE_RECTANGLE, 1, GL_R8,
	               font->bitmap.width,
	               font->bitmap.height);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0,
	                font->bitmap.width, font->bitmap.height,
	                GL_RED,GL_UNSIGNED_BYTE, texelv);
#else
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R8,
	             font->bitmap.width, font->bitmap.height,
	             0, GL_RED, GL_UNSIGNED_BYTE, texelv);
#endif
	glTexParameteri(GL_TEXTURE_RECTANGLE,
	                GL_TEXTURE_MAG_FILTER,
	                GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE,
	                GL_TEXTURE_MIN_FILTER,
	                GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE,
	                GL_TEXTURE_WRAP_S,
	                GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE,
	                GL_TEXTURE_WRAP_T,
	                GL_CLAMP_TO_BORDER);
	glActiveTexture(active);
	DJG_FREE(texelv);

	return (glGetError() == GL_NO_ERROR);
}

static bool djgf__load_program(djg_font *font)
{
	const char *srcv_v[] = {
		"#version 330\n",
		"layout(location = 0) in uint i_Data;\n\n",

		"void main()\n",
		"{\n",
		"	vec2 ndc = vec2(i_Data >> 8u & 0xFFFu, i_Data >> 20u & 0xFFFu);\n",
		"	gl_Position.xy = ndc / 4095.0 * 2.0 - 1.0;\n",
		"	gl_Position.zw = vec2(i_Data & 0xFFu, 0.0);\n",
		"}\n"
	};
	const char *srcv_g[] = {
		"#version 330\n",
		"uniform vec2 u_FontSize;\n", // size of the letter in NDC space
		"layout(points) in;\n",
		"layout(triangle_strip, max_vertices = 4) out;\n",
		"out vec2 o_TexCoord;\n\n",

		"void main()\n",
		"{\n",
		"	vec2 fs = u_FontSize.xy;\n",
		"	vec4 d = gl_in[0].gl_Position;\n", // letter aabb
		"	o_TexCoord = vec2(d.z * 8.0, -1);\n", // lower left
		"	gl_Position = vec4(d.xy - vec2(0, fs.y + fs.y/8.0), -1, 1);\n",
		"	EmitVertex();\n",
		"	o_TexCoord.s+= 8.0;\n", // lower right
		"	gl_Position = vec4(d.xy + vec2(fs.x, -fs.y-fs.y/8.0), -1, 1);\n",
		"	EmitVertex();\n",
		"	o_TexCoord+= vec2(-8,9);\n", // upper left
		"	gl_Position = vec4(d.xy, -1, 1);\n",
		"	EmitVertex();\n",
		"	o_TexCoord.s+= 8.0;\n", // upper right
		"	gl_Position = vec4(d.xy + vec2(fs.x, 0), -1, 1);\n",
		"	EmitVertex();\n",
		"	EndPrimitive();\n",
		"}\n"
	};
	const char *srcv_f[] = {
		"#version 330\n",
		"uniform sampler2DRect u_FontSampler;\n",
		"uniform vec4 u_FontColor;\n",
		"in vec2 o_TexCoord;\n",
		"#define i_TexCoord o_TexCoord\n",
		"layout(location = 0) out vec4 o_FragColor;\n\n",

		"void main()\n",
		"{\n",
			"vec4 t;\n",
			"t.x = texture(u_FontSampler, i_TexCoord).r;\n",
			"t.y = texture(u_FontSampler, i_TexCoord + vec2(-1, 0)).r;\n",
			"t.z = texture(u_FontSampler, i_TexCoord + vec2(-1,+1)).r;\n",
			"t.w = texture(u_FontSampler, i_TexCoord + vec2( 0,+1)).r;\n",
			"if (all(lessThan(t, vec4(.1)))) discard;\n",
			"o_FragColor = t.x * u_FontColor;\n",
		"}\n"
	};

	font->gl.program = glCreateProgram();

	if (!djgf__load_shader(font, GL_VERTEX_SHADER, DJG__BUFSIZE(srcv_v), srcv_v))
		return false;

	if (!djgf__load_shader(font, GL_GEOMETRY_SHADER, DJG__BUFSIZE(srcv_g), srcv_g))
		return false;

	if (!djgf__load_shader(font, GL_FRAGMENT_SHADER, DJG__BUFSIZE(srcv_f), srcv_f))
		return false;

	glLinkProgram(font->gl.program);
	if (true) {
		GLint link_status = 0;

		glGetProgramiv(font->gl.program, GL_LINK_STATUS, &link_status);
		if (!link_status) {
			GLint logc = 0;
			GLchar *logv = NULL;

			glGetProgramiv(font->gl.program, GL_INFO_LOG_LENGTH, &logc);
			logv = (GLchar *)DJG_MALLOC(logc);
			glGetProgramInfoLog(font->gl.program, logc, NULL, logv);
			fprintf(stderr, "djg_error: GLSL linker failure\n\
			                 -- Begin -- GLSL Linker Info Log\n\
			                 %s\n\
			                 -- End -- GLSL Linker Info Log\n", logv);
			return false;
		}
	}

	glUseProgram(font->gl.program);
	font->gl.uniforms[DJGF__UNIFORM_SIZE] = 
		glGetUniformLocation(font->gl.program, "u_FontSize");
	font->gl.uniforms[DJGF__UNIFORM_COLOR] = 
		glGetUniformLocation(font->gl.program, "u_FontColor");
	glUniform1i(glGetUniformLocation(font->gl.program, "u_FontSampler"),
	            font->gl.unit);

	return (glGetError() == GL_NO_ERROR);
}

static void
djgf__stream_vertex(djgf__vertex* vertex, char letter, int x, int y)
{
	int x0 = x < 0xFFF ? x : 0xFFF;  // clamp to 4095
	int y0 = y < 0xFFF ? y : 0xFFF;  // clamp to 4095
	vertex->letter = letter - ' ';   // llll llll ---- ---- ---- ---- ---- ----
	vertex->xy[0] = x0 & 0xFFu;      // llll llll xxxx xxxx ---- ---- ---- ----
	vertex->xy[1] = x0 >> 8 & 0x0Fu; // llll llll xxxx xxxx xxxx ---- ---- ----
	vertex->xy[1]|= y0 << 4 & 0xF0u; // llll llll xxxx xxxx xxxx yyyy ---- ----
	vertex->xy[2] = y0 >> 4 & 0xFFu; // llll llll xxxx xxxx xxxx yyyy yyyy yyyy
}

static GLint
djgf__stream_vertices(
	const djg_font *font,
	const char *buffer,
	int font_size,
	int glviewport_width, int glviewport_height,
	int y,
	djgf__vertex *data
) {
	int count = 0;
	int yc = glviewport_height - y; // invert coord system
	int xc = 0;
	int i = 0;

	for(; buffer[i] != '\0' && yc > 0; ++i) {
		if (buffer[i]=='\n') {
			xc = 0;
			yc-= font_size * 2;
		} else if (buffer[i] == '\t') {
			xc+= (font->bitmap.tab_size - (xc % font->bitmap.tab_size)) 
			   * (font_size >> 3);
		} else if (xc < glviewport_width - font_size) {
			djgf__stream_vertex(data, buffer[i], xc, yc);
			xc+= font->bitmap.char_size * (font_size >> 3);
			++count;
			++data;
		}
	}

	return count;
}

static bool
djgf__print(djg_font *font, int size, int x, int y, const char *buffer)
{
	djgf__vertex *vertices = NULL;
	GLint glviewport[4] = {0, 0, 0, 0};
	GLint count = 0;

	// get viewport and check coords
	glGetIntegerv(GL_VIEWPORT, glviewport);
	if(x + size > glviewport[2] || y + size > glviewport[3])
		return true; // text is out of viewport

	// map buffer
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, font->gl.buffer);
	vertices = (djgf__vertex *)glMapBufferRange(
		GL_ARRAY_BUFFER,
		sizeof(djgf__vertex) * font->buffer.offset,
		sizeof(djgf__vertex) * font->buffer.size,
		GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT
	);
	// check memory
	if(!vertices) {
		DJG_LOG("djg_error: glMapBufferRange() failed\n");
		return false;
	}
	// set data
	count = djgf__stream_vertices(font, buffer, size,
	                              glviewport[2], glviewport[3], y,
	                              vertices);
	// unmap buffer
	glUnmapBuffer(GL_ARRAY_BUFFER);

	// draw if data has been generated
	if(count > 0) {
		glViewport(x, 0, 4096, 4096);
		glUseProgram(font->gl.program);
		glUniform2f(font->gl.uniforms[DJGF__UNIFORM_SIZE],
		            2.f * size / 4096.f, 2.f * size / 4096.f);
		glUniform4f(font->gl.uniforms[DJGF__UNIFORM_COLOR],
		            font->color.r, font->color.g, font->color.b, font->color.a);
		glBindVertexArray(font->gl.vertexarray);
		glDrawArrays(GL_POINTS, font->buffer.offset, count);
		glViewport(glviewport[0], glviewport[1],
		           glviewport[2], glviewport[3]);

		// compute new buffer_offset
		font->buffer.offset+= font->buffer.size;
		if((font->buffer.offset + font->buffer.size)
		     * sizeof(djgf__vertex) >= font->buffer.capacity) {
			font->buffer.offset = 0; // loop
#ifndef NDEBUG
		DJG_LOG("djg_debug: Reached font buffer capacity (offset reset)\n");
#endif
		}
	}

	return true;
}

DJGDEF void
djgf_set_color(djg_font *font, GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
	DJG_ASSERT(font);
	font->color.r = (float)r / 255.f;
	font->color.g = (float)g / 255.f;
	font->color.b = (float)b / 255.f;
	font->color.a = (float)a / 255.f;
}

DJGDEF void djgf_release(djg_font *font)
{
	DJG_ASSERT(font);
	glDeleteTextures(1, &font->gl.texture);
	glDeleteBuffers(1, &font->gl.buffer);
	glDeleteVertexArrays(1, &font->gl.vertexarray);
	glDeleteProgram(font->gl.program);
	DJG_FREE(font);
}

DJGDEF djg_font *djgf_create(GLint unit)
{
	DJG_ASSERT(unit >= GL_TEXTURE0);
	djg_font *font;

	font = (djg_font *)DJG_MALLOC(sizeof(*font));
	font->buffer.capacity = DJGF__BUFFER_CAPACITY;
	font->buffer.size = DJGF__BUFFER_SIZE;
	font->buffer.offset = 0;
	font->bitmap.width = DJGF__BITMAP_WIDTH;
	font->bitmap.height = DJGF__BITMAP_HEIGHT;
	font->bitmap.char_size = DJGF__BITMAP_CHAR_SIZE;
	font->bitmap.tab_size = DJGF__BITMAP_TAB_SIZE;
	font->gl.unit = unit - GL_TEXTURE0;
	djgf_set_color(font, 255, 255, 255, 255);

	if(!djgf__load_buffer(font)) {
		DJG_FREE(font);
		return NULL;
	}
	if(!djgf__load_texture(font)) {
		glDeleteBuffers(1, &font->gl.buffer);
		DJG_FREE(font);
		return NULL;
	}
	if(!djgf__load_vertexarray(font)) {
		glDeleteBuffers(1, &font->gl.buffer);
		glDeleteTextures(1, &font->gl.texture);
		DJG_FREE(font);
		return NULL;
	}
	if(!djgf__load_program(font)) {
		glDeleteBuffers(1, &font->gl.buffer);
		glDeleteTextures(1, &font->gl.texture);
		glDeleteVertexArrays(1, &font->gl.vertexarray);
		DJG_FREE(font);
		return NULL;
	}

	return font;
}

static bool
djgf__print_fast(djg_font *font, int size, int x, int y, const char *format)
{
	DJG_ASSERT(size == DJG_FONT_SMALL || size == DJG_FONT_BIG);
	DJG_ASSERT(x >= 0 && y >= 0);
	DJG_ASSERT(format);

	if (format[0] != '\0')
		return djgf__print(font, size, x, y, format);

	return true;
}

DJGDEF bool
djgf_print_fast(djg_font *font, int size, int x, int y, const char *format, ...)
{
	DJG_ASSERT(font);
	va_list vl;
	char *buffer = (char *)DJG_MALLOC(font->buffer.size);

	va_start(vl, format);
	vsnprintf(buffer, font->buffer.size, format, vl);
	va_end(vl);

	return djgf__print_fast(font, size, x, y, buffer);
}

DJGDEF bool
djgf_print(djg_font *font, int size, int x, int y, const char *format, ...)
{
	DJG_ASSERT(font);
	enum {VERTEXARRAY, BUFFER, PROGRAM, COUNT};
	char *buffer = (char *)DJG_MALLOC(font->buffer.size);
	GLint state[COUNT];
	va_list vl;
	bool v;

	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, state + VERTEXARRAY);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, state + BUFFER);
	glGetIntegerv(GL_CURRENT_PROGRAM, state + PROGRAM);

	va_start(vl, format);
	vsnprintf(buffer, font->buffer.size, format, vl);
	va_end(vl);

	v = djgf__print_fast(font, size, x, y, buffer);

	glBindBuffer(GL_ARRAY_BUFFER, state[BUFFER]);
	glBindVertexArray(state[VERTEXARRAY]);
	glUseProgram(state[PROGRAM]);

	free(buffer);

	return v;
}

#endif // DJ_OPENGL_IMPLEMENTATION

