#ifndef _GINPUT_PI_H_
#define _GINPUT_PI_H_

#include <gglobal.h>

#ifdef __cplusplus
extern "C" {
#endif

G_API void ginputp_mouseDown(int x, int y, int button, int mod);
G_API void ginputp_mouseMove(int x, int y, int button, int mod);
G_API void ginputp_mouseHover(int x, int y, int button, int mod);
G_API void ginputp_mouseUp(int x, int y, int button, int mod);
G_API void ginputp_mouseWheel(int x, int y, int button, int delta, int mod);
G_API void ginputp_mouseEnter(int x, int y, int buttons, int mod);
G_API void ginputp_mouseLeave(int x, int y, int mod);
G_API void ginputp_touchesBegin(int x, int y, int id, float pressure, int touchType, int touches, int xs[], int ys[], int ids[], float pressures[], int touchTypes[], int mod, int button);
G_API void ginputp_touchesMove(int x, int y, int id, float pressure, int touchType, int touches, int xs[], int ys[], int ids[], float pressures[], int touchTypes[], int mod, int button);
G_API void ginputp_touchesEnd(int x, int y, int id, float pressure, int touchType, int touches, int xs[], int ys[], int ids[], float pressures[], int touchTypes[], int mod, int button);
G_API void ginputp_touchesCancel(int x, int y, int id, float pressure, int touchType, int touches, int xs[], int ys[], int ids[], float pressures[], int touchTypes[], int mod, int button);
G_API void ginputp_keyDown(int keyCode,int modifiers);
G_API void ginputp_keyUp(int keyCode,int modifiers);
G_API void ginputp_keyChar(const char *keyChar);


#ifdef __cplusplus
}
#endif

#endif
