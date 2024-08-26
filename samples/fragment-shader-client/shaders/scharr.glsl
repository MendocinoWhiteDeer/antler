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
  
/*
Scharr filter

References:

Jähne, Bernd, Hanno Scharr, Stefan Körkel, Bernd Jähne, Horst Haußecker, and Peter Geißler.
"Principles of filter design." Handbook of computer vision and applications 2 (1999): 125-151.

Scharr, Hanno, and Joachim Weickert. "An anisotropic diffusion algorithm with optimized rotation invariance." In Mustererkennung 2000: 22. DAGM-Symposium. Kiel, 13.–15. September 2000, pp. 460-467. Springer Berlin Heidelberg, 2000.
*/

void atlrFragment(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy / ubo.resolution.xy;
    float stepX = 1.0f / ubo.resolution.x;
    float stepY = 1.0f / ubo.resolution.y;

    float umvm = length(texture(textureSampler, uv + vec2(-stepX, -stepY)));
    float uvm  = length(texture(textureSampler, uv + vec2(     0, -stepY)));
    float upvm = length(texture(textureSampler, uv + vec2( stepX, -stepY)));
    float umv  = length(texture(textureSampler, uv + vec2(-stepX,      0)));
    float upv  = length(texture(textureSampler, uv + vec2( stepX,      0)));
    float umvp = length(texture(textureSampler, uv + vec2(-stepX,  stepY)));
    float uvp  = length(texture(textureSampler, uv + vec2(     0,  stepY)));
    float upvp = length(texture(textureSampler, uv + vec2( stepX,  stepY)));

    float r = length(vec2(
    	 3.0f * (umvm - upvm) + 10.0f * (umv - upv) + 3.0f * (umvp - upvp),
    	 3.0f * (umvp - umvm) + 10.0f * (uvp - uvm) + 3.0f * (upvp - upvm)));
    fragColor = vec4(r, r, r, 1.0f);
}