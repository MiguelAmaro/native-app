//A: Attributes
//U: Uniforms

attribute vec2 APosition;
attribute vec3 AColor;
attribute vec4 AUIRect;
attribute vec4 AUIColor;

varying vec4 Color;
varying vec2 UV;
varying vec2 Dim;

#define UI_ELEMENT_MAX_COUNT (64)
uniform vec2  UWinRes;
//uniform float URect [4*UI_ELEMENT_MAX_COUNT];
//uniform vec4  UColor[1*UI_ELEMENT_MAX_COUNT];

void main()
{
  Color = vec4(AColor,1.0)*0.0+AUIColor*1.0;
  UV = APosition;
  vec2 Min = vec2(AUIRect[0], AUIRect[1]);
  vec2 Max = vec2(AUIRect[2], AUIRect[3]);
  vec2 FullDim = vec2((Max.x-Min.x)*1.0f,
                      (Max.y-Min.y)*1.0f);
  vec2 HalfDim   = vec2((Max.x-Min.x)*0.5f,
                        (Max.y-Min.y)*0.5f);
  
  //position is counting as uv also
  vec2 PosCenter    = Min+HalfDim;
  PosCenter.y = (UWinRes.y-PosCenter.y);
  vec2 NDCHalfDim = 2.0*HalfDim.xy/UWinRes.xy;
  vec2 NDCPosCenter = 2.0f*(PosCenter.xy/UWinRes.xy)-1.0f;
  
  float AspectRatio = 1.0f;
  Dim = FullDim;
  //if     (Dim.y>Dim.x) { UVDim = vec2(1.0f, (Dim.y/Dim.x));}
  //else if(Dim.x>Dim.y) { UVDim = vec2((Dim.x/Dim.y), 1.0f);}
  
  
  
  gl_Position = vec4(APosition*NDCHalfDim+NDCPosCenter, 0.0, 1.0);
}
