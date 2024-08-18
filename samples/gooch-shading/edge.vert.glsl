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

layout (location = 0) out vec2 outUv;

const vec2 positions[4] = vec2[4](vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f), vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f));

void main()
{
	vec2 pos = positions[gl_VertexIndex];
	outUv = 0.5f * (pos + 1.0f);
	gl_Position = vec4(pos, 0.0f, 1.0f);
}