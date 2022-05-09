#include <jni.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <android/log.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>

#include "types.h"
#include "math.h"

#define LOG(...) ((void)__android_log_print(ANDROID_LOG_INFO, "NativeExample", __VA_ARGS__))

v2 GlobalRes      = {0};
v2 GlobalTouchPos = {0};

struct engine
{
  struct android_app* App;
  
  int Active;
  EGLDisplay Display;
  EGLSurface Surface;
  EGLContext Context;
  int32_t Width;
  int32_t Height;
  
  GLuint Buffer;
  GLuint Shader;
};

static int EngineInitDisplay(struct engine* Engine)
{
  const EGLint Attribs[] =
  {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_NONE,
  };
  
  EGLDisplay Display;
  if ((Display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY)
  {
    LOG("error with eglGetDisplay");
    return -1;
  }
  
  if (!eglInitialize(Display, 0, 0))
  {
    LOG("error with eglInitialize");
    return -1;
  }
  
  EGLConfig Config;
  EGLint NumConfigs;
  if (!eglChooseConfig(Display, Attribs, &Config, 1, &NumConfigs))
  {
    LOG("error with eglChooseConfig");
    return -1;
  }
  
  EGLint Format;
  if (!eglGetConfigAttrib(Display, Config, EGL_NATIVE_VISUAL_ID, &Format))
  {
    LOG("error with eglGetConfigAttrib");
    return -1;
  }
  
  ANativeWindow_setBuffersGeometry(Engine->App->window, 0, 0, Format);
  
  EGLSurface Surface;
  if (!(Surface = eglCreateWindowSurface(Display, Config, Engine->App->window, NULL)))
  {
    LOG("error with eglCreateWindowSurface");
    return -1;
  }
  
  const EGLint CtxAttrib[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
  EGLContext Context;
  if (!(Context = eglCreateContext(Display, Config, NULL, CtxAttrib)))
  {
    LOG("error with eglCreateContext");
    return -1;
  }
  
  if (eglMakeCurrent(Display, Surface, Surface, Context) == EGL_FALSE)
  {
    LOG("error with eglMakeCurrent");
    return -1;
  }
  
  LOG("GL_VENDOR = %s", glGetString(GL_VENDOR));
  LOG("GL_RENDERER = %s", glGetString(GL_RENDERER));
  LOG("GL_VERSION = %s", glGetString(GL_VERSION));
  
  EGLint Width, Height;
  eglQuerySurface(Display, Surface, EGL_WIDTH, &Width);
  eglQuerySurface(Display, Surface, EGL_HEIGHT, &Height);
  
  Engine->Display = Display;
  Engine->Context = Context;
  Engine->Surface = Surface;
  Engine->Width = Width;
  Engine->Height = Height;
  
  AAsset* VAsset = AAssetManager_open(Engine->App->activity->assetManager, "vertex.glsl", AASSET_MODE_BUFFER);
  if (!VAsset)
  {
    LOG("error opening vertex.glsl");
    return -1;
  }
  const GLchar* VSrc = AAsset_getBuffer(VAsset);
  GLint VLen = AAsset_getLength(VAsset);
  
  GLuint V = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(V, 1, &VSrc, &VLen);
  glCompileShader(V);
  
  AAsset_close(VAsset);
  
  AAsset* FAsset = AAssetManager_open(Engine->App->activity->assetManager, "fragment.glsl", AASSET_MODE_BUFFER);
  if (!FAsset)
  {
    LOG("error opening fragment.glsl");
    return -1;
  }
  const GLchar* FSrc = AAsset_getBuffer(FAsset);
  GLint FLen = AAsset_getLength(FAsset);
  
  GLuint F = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(F, 1, &FSrc, &FLen);
  glCompileShader(F);
  
  AAsset_close(FAsset);
  
  GLuint ShaderProgram = glCreateProgram();
  glAttachShader(ShaderProgram, V);
  glAttachShader(ShaderProgram, F);
  
  glBindAttribLocation(ShaderProgram, 0, "vPosition");
  glBindAttribLocation(ShaderProgram, 1, "vColor");
  glLinkProgram(ShaderProgram);
  
  glDeleteShader(V);
  glDeleteShader(F);
  glUseProgram(ShaderProgram);
  
  const float TriData[] =
  {
    0.0f,  0.5f, 1.f, 0.f, 0.f,
    -0.5f, -0.5f, 0.f, 1.f, 0.f,
    0.5f, -0.5f, 0.f, 0.f, 1.f,
  };
  
  GLuint VertBuffer;
  glGenBuffers(1, &VertBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, VertBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(TriData), TriData, GL_STATIC_DRAW);
  
  Engine->Buffer = VertBuffer;
  Engine->Shader = ShaderProgram;
  
  return 0;
}

static void EngineDrawFrame(struct engine* Engine)
{
  if (Engine->Display == NULL)
  {
    return;
  }
  
  glClearColor(0.258824f, 0.258824f, 0.435294f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glUseProgram(Engine->Shader);
  
  glBindBuffer(GL_ARRAY_BUFFER, Engine->Buffer);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2+3)*sizeof(float), NULL);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (2+3)*sizeof(float), (void*)(2*sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  
  glDrawArrays(GL_TRIANGLES, 0, 3);
  
  eglSwapBuffers(Engine->Display, Engine->Surface);
  return;
}

static void EngineTermDisplay(struct engine* Engine)
{
  if (Engine->Display != EGL_NO_DISPLAY)
  {
    glDeleteProgram(Engine->Shader);
    glDeleteBuffers(1, &Engine->Buffer);
    
    eglMakeCurrent(Engine->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (Engine->Context != EGL_NO_CONTEXT)
    {
      eglDestroyContext(Engine->Display, Engine->Context);
    }
    if (Engine->Surface != EGL_NO_SURFACE)
    {
      eglDestroySurface(Engine->Display, Engine->Surface);
    }
    eglTerminate(Engine->Display);
  }
  Engine->Active = 0;
  Engine->Display = EGL_NO_DISPLAY;
  Engine->Context = EGL_NO_CONTEXT;
  Engine->Surface = EGL_NO_SURFACE;
}

static int32_t EngineHandleInput(struct android_app* App, AInputEvent* Event)
{
  // NOTE(MIGUEL): do i even need to take input in this function?????
  if((AInputEvent_getSource(Event) == AINPUT_SOURCE_TOUCHSCREEN) &&
     (AInputEvent_getType  (Event) == AINPUT_EVENT_TYPE_MOTION))
  {
    u32 Action   = AMotionEvent_getAction(Event);
    u32 PtrIndex = ((Action >>  AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT));
    
    
    GlobalTouchPos.x = AMotionEvent_getRawX(Event,
                                            PtrIndex);
    
    GlobalTouchPos.y = AMotionEvent_getRawY(Event,
                                            PtrIndex);
    
    LOG("input| x: %f, y: %f", GlobalTouchPos.x, GlobalTouchPos.y);
    switch(Action)
    {
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
      {
        GlobalTouchPos.x = AMotionEvent_getRawX(Event,
                                                PtrIndex) /  AINPUT_MOTION_RANGE_X;
        
        GlobalTouchPos.y = AMotionEvent_getRawY(Event,
                                                PtrIndex) /  AINPUT_MOTION_RANGE_Y;
        
        
      } break;
      default:
      {
        
      } break;
    }
    
  }
  return 0;
}

static void EngineHandleCmd(struct android_app* App, int32_t Cmd)
{
  struct engine* Engine = (struct engine*)App->userData;
  switch (Cmd)
  {
    case APP_CMD_INIT_WINDOW:
    if (Engine->App->window != NULL)
    {
      EngineInitDisplay(Engine);
      EngineDrawFrame(Engine);
    }
    break;
    
    case APP_CMD_TERM_WINDOW:
    EngineTermDisplay(Engine);
    break;
    
    case APP_CMD_GAINED_FOCUS:
    Engine->Active = 1;
    break;
    
    case APP_CMD_LOST_FOCUS:
    Engine->Active = 0;
    EngineDrawFrame(Engine);
    break;
  }
}

void android_main(struct android_app* State)
{
  struct engine Engine;
  memset(&Engine, 0, sizeof(Engine));
  
  State->userData = &Engine;
  State->onAppCmd = EngineHandleCmd;
  State->onInputEvent = EngineHandleInput;
  Engine.App = State;
  
  while (1)
  {
    int Ident;
    int Events;
    struct android_poll_source* Source;
    
    while ((Ident=ALooper_pollAll(Engine.Active ? 0 : -1, NULL, &Events, (void**)&Source)) >= 0)
    {
      if (Source != NULL)
      {
        Source->process(State, Source);
      }
      
      if (State->destroyRequested != 0)
      {
        EngineTermDisplay(&Engine);
        return;
      }
    }
    
    if (Engine.Active)
    {
      GlobalRes.x = Engine.Width;
      GlobalRes.y = Engine.Height;
      EngineDrawFrame(&Engine);
    }
  }
}
