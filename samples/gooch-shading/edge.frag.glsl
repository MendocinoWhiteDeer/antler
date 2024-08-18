/*

This file is part of antler.
Copyright (C) 2024 Taylor Wampler 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
  
*/

#version 460

layout (set= 0, binding = 0) uniform sampler2D samplerColor;
layout (set= 0, binding = 1) uniform sampler2D samplerDepth;

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

void main() 
{
	vec2 uv = inUv;
	vec2 texelSize = 1.0 / textureSize(samplerDepth, 0);

	float depth       = texture(samplerDepth, uv).r;
    	float depthLeft   = texture(samplerDepth, uv + vec2(-texelSize.x, 0.0f        )).r;
    	float depthRight  = texture(samplerDepth, uv + vec2( texelSize.x, 0.0f        )).r;
    	float depthUp     = texture(samplerDepth, uv + vec2(0.0f        , texelSize.y )).r;
    	float depthDown   = texture(samplerDepth, uv + vec2(0.0f        , -texelSize.y)).r;

	float weight = abs(depth - depthLeft) + abs(depth - depthRight) + abs(depth - depthUp) + abs(depth - depthRight);
	float threshold = 0.01f;
	float edge = step(0.0f, weight - threshold);

	vec3 color = (1.0f - edge) * texture(samplerColor, uv).rgb;
	outColor = vec4(color, 1.0f);
}