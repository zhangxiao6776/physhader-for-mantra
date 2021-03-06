// This may look like -*- C -*- code, but it is really Houdini Vex
//
//	volume.h - volumetric routines,
//	This is part of phyShader for Mantra.
//
//
// Copyright (c) 2013-2015 Roman Saldygashev <sldg.roman@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#ifndef __phy_volume__
#define __phy_volume__


#include <pbr.h>
#include <phy/utils.h>


// Direct lighting for volume
vector
illum_volume(vector p, v;
	     bsdf f;
	     int sid;
	     int depth;
	     float depthimp)
{
    vector eval = .0;
    vector illum = .0;

    START_ILLUMINANCE;

    vector accum = .0;

    START_SAMPLING("nextpixel");
    SET_SAMPLE;

    SAMPLE_LIGHT(p, v);

    if (mask & PBR_VOLUME_MASK)
	{
	    cl *= EVAL_BSDF(f);

	    accum += cl;
	}

    END_LOOP; 	// SAMPLING

    illum = accum / samples;

    eval += illum;

    END_LOOP; 	// ILLUMINANCE

    return eval;
}


// Constant density stachaostic raymarching routine
struct RayMarcher
{
    vector _ca;
    bsdf _f;
    int _sid;
    int _samples;
    int _depth;
    float _depthimp;

    float _sigma;

    void
    init(vector ca;
	 bsdf f;
	 int sid, samples, depth;
	 float depthimp)
    {
	this._ca = ca;
	this._f = f;
	this._sid = sid;
	this._samples = samples;
	this._depth = depth;
	this._depthimp = depthimp;

	this._sigma = luminance(ca);
    }

    // Without all "this"'s Mantra get segfault
    vector
    eval(vector p, v;
	 float raylength)
    {
	vector ca = this._ca;
	bsdf f = this._f;
	int sid = this._sid;
	int samples = this._samples;
	int depth = this._depth;
	float depthimp = this._depthimp;
	float sigma = this._sigma;
	float pdf = .0;

	vector
	    result = .0,
	    pp, cl, l;

	START_SAMPLING("nexpixel");
	sx *= raylength;

	pp = p + v * sx;

	cl = illum_volume(pp, v, f, sid,
			  depth, depthimp)
	    * exp(-ca * sx);

	float weight = exp(-sigma * sx);
	pdf += weight;
	result += cl * weight;
	
	END_LOOP;

	if (pdf > .0)
	    result /= pdf;

	return result;
    }
};


// Volume model
void
phyvolume(vector p;
	  vector i;
	  float density;
	  vector color;
	  float g;
	  int constantshadow;
	  float shadowdensity;
	  float depthimp;
	  export vector beauty;
	  export float opacity;
	  export bsdf f)
{
    beauty = 0;
    opacity = 0;

    f = g == .0 ? isotropic() : henyeygreenstein(g);

    vector v = -i;
    int depth = getraylevel() + getglobalraylevel();

    string renderengine;
    renderstate("renderer:renderengine", renderengine);

    int sid = renderengine == "micropoly" ? newsampler() : SID;

    if(isshadowray() && constantshadow)
	opacity = shadowdensity;
    else
	opacity = 1. - exp(-density * dPdz);

    beauty = illum_volume(p, v, f, sid, depth, depthimp);

    beauty *= opacity * color;
}


#endif // __phy_volume__
