vec3 a_position  : POSITION;
vec3 a_normal    : NORMAL;
vec4 a_tangent   : TANGENT;
vec2 a_texcoord0 : TEXCOORD0;
vec2 a_texcoord1 : TEXCOORD1;
vec2 a_texcoord2 : TEXCOORD2;
float a_texcoord3 : TEXCOORD3;
ivec4 a_indices : BLENDINDICES;
vec4 a_weight   : BLENDWEIGHT;

vec4 i_data0 : TEXCOORD7;
vec4 i_data1 : TEXCOORD6;
vec4 i_data2 : TEXCOORD5;
vec4 i_data3 : TEXCOORD4;

vec2 v_texcoord0 : TEXCOORD0;
vec2 v_lmCoord   : TEXCOORD1;
vec2 v_lmSize    : TEXCOORD2;
float v_alpha    : TEXCOORD3;
vec3 v_fragPos   : TEXCOORD4;
vec3 v_tbn0      : TEXCOORD5;
vec3 v_tbn1      : TEXCOORD6;
vec3 v_tbn2      : TEXCOORD7;
vec3 v_dir       : TEXCOORD8;