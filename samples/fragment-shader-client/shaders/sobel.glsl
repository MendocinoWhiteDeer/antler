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
Sobel filter

References:

Sobel, Irwin, R. Duda, P. Hart, and John Wiley.
"Sobel-feldman operator." https://www.researchgate.net/profile/Irwin-Sobel/publication/285159837.
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
    	 umvm - upvm + 2.0f * (umv - upv) + umvp - upvp,
    	 umvp - umvm + 2.0f * (uvp - uvm) + upvp - upvm));
    fragColor = vec4(r, r, r, 1.0f);
}