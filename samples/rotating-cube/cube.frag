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

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inLightDir;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 baseColor = vec3(0.8f, 0.2f, 0.2f);
     	vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
     	vec3 lightDir = normalize(inLightDir);
	vec3 normal = normalize(inNormal);
	
	float ambient = 0.2f;
	float diffuse = max(-dot(normal, lightDir), 0.0f);
	vec3 color = (ambient + diffuse) * lightColor * baseColor;
	outColor = vec4(color, 1.0f);
}