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

#define MAX_SHELLS 64
#define MAX_VERTICES (3 * MAX_SHELLS)

layout (triangles) in;
layout (triangle_strip, max_vertices = MAX_VERTICES) out;

layout(location = 0) in vec3 inNormal[];
layout(location = 1) in vec2 inUv[];

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outUvw;

layout(set = 0, binding = 0) uniform Camera
{
	vec4 eyePos;
	mat4 view;
	mat4 perspective;

} camera;

layout(set = 1, binding = 0) uniform Shells
{
    float extrusion;
    int count; // This is the number of steps which divide the total extrusion. It is forced to be between 1 and MAX_SHELLS.

} shells; 

void main()
{
	int validCount = clamp(shells.count, 1, MAX_SHELLS);
	for (int i = 0; i < validCount; i++)
	{
		float normalizedDistance = float(i) / float(validCount);
		float distance = shells.extrusion * normalizedDistance;

		for (int j = 0; j < 3; j++)
		{
			outNormal = inNormal[j]; 
			vec4 worldPos = gl_in[j].gl_Position + vec4(outNormal * distance, 0.0);
			gl_Position = camera.perspective * camera.view * worldPos;
			outUvw = vec3(inUv[j], normalizedDistance);
			EmitVertex();
		}
		EndPrimitive();
	}
}