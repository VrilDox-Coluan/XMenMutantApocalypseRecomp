#include <stddef.h>

struct RendererFuncs;

void OpenGLRenderer_Create(struct RendererFuncs *funcs) { (void)funcs; }
void snesrecomp_opengl_set_vsync(int interval) { (void)interval; }
void snesrecomp_opengl_set_viewport(void (*compute)(int, int, int, int, int *, int *, int *, int *)) { (void)compute; }
void OpenGLRenderer_RequestScreenshot(void) {}
