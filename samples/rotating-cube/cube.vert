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

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outLightDir;

layout(binding = 0) uniform Camera
{
	vec4 eye;
	mat4 view;
	mat4 perspective;

} camera;

layout(push_constant) uniform World
{
	mat4 transform;
	mat4 normalTransform;
	
} world;

void main()
{
	outNormal = mat3(world.normalTransform) * inNormal;
	vec4 worldPos = world.transform * vec4(inPos, 1.0f);
	outLightDir = normalize(vec3(worldPos - camera.eye));
	gl_Position = camera.perspective * camera.view * worldPos;
}