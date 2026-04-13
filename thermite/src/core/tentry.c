#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include "../../../glfw/include/GLFW/glfw3.h"
#include "../../../glfw/include/GLFW/glfw3native.h"
#endif

#include "tentry.h"

int tentry(int argc, char** argv, size_t usr_size) {
  tenv env;

  env.config.argc = argc;
  env.config.argv = argv;
  env.config.vsync = true;
  env.config.running = true;
  env.config.fullscreen = false;
  env.config.resizable = true;
  env.config.aspect_ratio = 16 / 9.0f;
  env.config.fif = 3;
  env.config.title = "app";
  env.usr = malloc(usr_size);

  tlaunch(&env);

  env.wnd = twindow_create(&env, trender, tresize);
  
  // Set window icon from embedded resource
  #ifdef _WIN32
  {
    HWND hwnd = glfwGetWin32Window(env.wnd->handle);
    HICON hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
    if (hicon) {
      SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);
      SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
      // Also set class icon for taskbar
      SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)hicon);
      SetClassLongPtr(hwnd, GCLP_HICONSM, (LONG_PTR)hicon);
    }
  }
  #endif
  
  env.kb = tkeyboard_create(env.wnd);
  env.ms = tmouse_create(env.wnd);
  if ((env.ctx = tcontext_create(env.wnd, env.config.vsync, env.config.fif)) == NULL) {
    printf("Error creating context.\n");
    exit(1);
  }
  tinit(&env);

  while (env.config.running) {
    if (env.ctx->swapchain_ok) {
      twindow_poll_input(env.wnd);
    } else {
      twindow_wait_input(env.wnd);
      continue;
    }

    tinput(&env);
    trender(&env);

    tkeyboard_update(env.kb);
    tmouse_update(env.ms);
  }
  tcontext_wait_idle(env.ctx);

  tdestroy(&env);
  tcontext_destroy(env.ctx);
  tmouse_destroy(env.ms);
  tkeyboard_destroy(env.kb);
  twindow_destroy(env.wnd);
  free(env.usr);

  return 0;
}