$input v_normal, v_tangent, v_bitangent, v_texcoord0, v_texcoord1, v_color0, v_wpos, v_view

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2021 Fragcolor Pte. Ltd. */

#include <bgfx_shader.h>
#include <shaderlib.h>

SAMPLER2D(s_texColor,  0);
SAMPLER2D(s_texNormal, 1);
uniform vec4 u_lightPosRadius[4];
uniform vec4 u_lightRgbInnerR[4];

vec2 blinn(vec3 _lightDir, vec3 _normal, vec3 _viewDir) {
	float ndotl = dot(_normal, _lightDir);
	//vec3 reflected = _lightDir - 2.0*ndotl*_normal; // reflect(_lightDir, _normal);
	vec3 reflected = 2.0*ndotl*_normal - _lightDir;
	float rdotv = dot(reflected, _viewDir);
	return vec2(ndotl, rdotv);
}

float fresnel(float _ndotl, float _bias, float _pow) {
	float facing = (1.0 - _ndotl);
	return max(_bias + (1.0 - _bias) * pow(facing, _pow), 0.0);
}

vec4 lit(float _ndotl, float _rdotv, float _m) {
	float diff = max(0.0, _ndotl);
	float spec = step(0.0, _ndotl) * max(0.0, _rdotv * _m);
	return vec4(1.0, diff, spec, 1.0);
}

vec4 powRgba(vec4 _rgba, float _pow) {
	vec4 result;
	result.xyz = pow(_rgba.xyz, vec3_splat(_pow) );
	result.w = _rgba.w;
	return result;
}

vec3 calcLight(int _idx, mat3 _tbn, vec3 _wpos, vec3 _normal, vec3 _view) {
	vec3 lp = u_lightPosRadius[_idx].xyz - _wpos;
	float attn = 1.0 - smoothstep(u_lightRgbInnerR[_idx].w, 1.0, length(lp) / u_lightPosRadius[_idx].w);
	vec3 lightDir = mul( normalize(lp), _tbn );
	vec2 bln = blinn(lightDir, _normal, _view);
	vec4 lc = lit(bln.x, bln.y, 1.0);
	vec3 rgb = u_lightRgbInnerR[_idx].xyz * saturate(lc.y) * attn;
	return rgb;
}

mat3 mtx3FromCols(vec3 c0, vec3 c1, vec3 c2) {
#if BGFX_SHADER_LANGUAGE_GLSL
	return mat3(c0, c1, c2);
#else
	return transpose(mat3(c0, c1, c2));
#endif
}

void main() {
#ifdef CB_HAS_NORMAL
#ifdef CB_NORMAL_TEXTURE
	vec3 normal;
	normal.xy = texture2D(s_texNormal, v_texcoord0).xy * 2.0 - 1.0;
	normal.z = sqrt(1.0 - dot(normal.xy, normal.xy) );
#else
	vec3 normal = v_normal;
#endif
#else
	vec3 normal = vec3(0.0, 0.0, 1.0);
#endif

#ifdef CB_HAS_TANGENT
	vec3 tangent = v_tangent;
	vec3 bitangent = v_bitangent;
	vec3 view = normalize(v_view);
#else
	vec3 tangent = vec3(1.0, 0.0, 0.0);
	vec3 bitangent = vec3(0.0, 1.0, 0.0);
	vec3 view = vec3(0.0, 0.0, 0.0);
#endif

#ifndef CB_UNLIT
	mat3 tbn = mtx3FromCols(tangent, bitangent, normal);

	vec3 lightColor;
	lightColor =  calcLight(0, tbn, v_wpos, normal, view);
	lightColor += calcLight(1, tbn, v_wpos, normal, view);
	lightColor += calcLight(2, tbn, v_wpos, normal, view);
	lightColor += calcLight(3, tbn, v_wpos, normal, view);
#endif

#ifndef CB_HAS_COLOR_0
	vec4 _color = vec4(1.0, 1.0, 1.0, 1.0);
#else
	vec4 _color = v_color0;
#endif

#ifdef CB_PBR_COLOR_TEXTURE
	vec4 color = toLinear(texture2D(s_texColor, v_texcoord0) * _color);
#else
	vec4 color = toLinear(_color);
#endif

#ifndef CB_UNLIT
	gl_FragColor.xyz = max(vec3_splat(0.05), lightColor.xyz)*color.xyz;
#else
	gl_FragColor.xyz = color.xyz;
#endif
	gl_FragColor.w = 1.0;
	gl_FragColor = toGamma(gl_FragColor);
}
