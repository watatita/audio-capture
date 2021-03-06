// AudioCaptureSDL.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <complex>
#include <math.h>
#include <SDL.h>

using namespace std;

SDL_Window *gWindow = NULL;
SDL_Renderer *gRender = NULL;
SDL_Surface *gSurface = NULL;
SDL_AudioDeviceID gRecDevice;
SDL_AudioSpec gRecordSpec, gOutRecSpec;

Uint8 gRecordingBuffer[8192];

void audioRecordCallback(void* userdata, Uint8* stream, int len);
float hannWindow(float input, int _n, int _nsample);
void separate(complex<float>* a, int n);
void fft2(complex<float>* X, int N);

int main(int argc, char** argv)
{
	bool done = false;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	gWindow = SDL_CreateWindow("yolo", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
	gRender = SDL_CreateRenderer(gWindow, 0, SDL_RENDERER_SOFTWARE);

	gRecordSpec.freq = 44100;
	gRecordSpec.format = AUDIO_F32;
	gRecordSpec.channels = 1;
	gRecordSpec.samples = 256;
	gRecordSpec.callback = audioRecordCallback;

	cout << "number of record device : " << SDL_GetNumAudioDevices(SDL_TRUE) << endl;
	
	int audioDeviceIndex = 0;
	gRecDevice = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(audioDeviceIndex, SDL_TRUE), SDL_TRUE, &gRecordSpec, &gOutRecSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	SDL_PauseAudioDevice(gRecDevice, SDL_FALSE);
	
	int nsample = gOutRecSpec.samples;
	Uint32 lastTick = SDL_GetTicks();
	SDL_Event _e;
	complex<float> xsig[512];
	while (!done) {
		
		Uint32 deltaTime = SDL_GetTicks() - lastTick;
		lastTick = SDL_GetTicks();
		
		SDL_SetRenderDrawColor(gRender, 0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE);

		SDL_RenderClear(gRender);
		float* bPointer = (float*)gRecordingBuffer;
		float lastMagn = 0, lastOutMagn = 0;
		for (int i = 0; i < nsample * 2; i++) {
			float sigval = bPointer[i] * 100;
			float wOut = hannWindow(sigval, i, nsample * 2);	//hann window
			xsig[i] = complex<float>(wOut, 0);
			SDL_SetRenderDrawColor(gRender, 0x10, 0x10, 0xff, SDL_ALPHA_OPAQUE);
			SDL_RenderDrawLine(gRender, i - 1, 200 - (int)lastMagn, i, 200 - (int)sigval);
			SDL_SetRenderDrawColor(gRender, 0xff, 0x10, 0x10, SDL_ALPHA_OPAQUE);
			SDL_RenderDrawLine(gRender, i - 1, 200 - (int)lastOutMagn, i, 200 - (int)wOut);
			lastMagn = sigval;
			lastOutMagn = wOut;
		}
		fft2(xsig, nsample * 2);
		for (int i = 0; i < nsample; i++) {
			SDL_SetRenderDrawColor(gRender, 0x10, 0xff, 0xff, SDL_ALPHA_OPAQUE);
			SDL_RenderDrawLine(gRender, nsample + i, 300, nsample + i, 300 - (int)xsig[i].real());
			SDL_RenderDrawLine(gRender, i, 300, i, 300 - (int)xsig[nsample + i].real());
			SDL_SetRenderDrawColor(gRender, 0xff, 0x10, 0xff, SDL_ALPHA_OPAQUE);
			SDL_RenderDrawLine(gRender, nsample + i, 300, nsample + i, 300 - (int)xsig[i].imag());
			SDL_RenderDrawLine(gRender, i, 300, i, 300 - (int)xsig[nsample + i].imag());
		}
		SDL_RenderPresent(gRender);

		while (SDL_PollEvent(&_e))
		{
			if (_e.type == SDL_QUIT) {
				done = true;
			}
			if (_e.type == SDL_KEYDOWN) {
				if (_e.key.keysym.sym == SDLK_ESCAPE) {
					done = true;
				}
			}
		}

	}
	SDL_Quit();
	return 0;
}

void audioRecordCallback(void* userdata, Uint8* stream, int len) {
	memcpy(&gRecordingBuffer[0], &gRecordingBuffer[len], len);
	memcpy(&gRecordingBuffer[len], stream, len);
}

float hannWindow(float input, int _n, int _nsample)
{
	float nm;
	nm = 0.5 - 0.5*(cos(2 * M_PI*_n / _nsample));
	return nm * input;
}

void separate(complex<float>* a, int n) {
	complex<float>* b = new complex<float>[n / 2];  // get temp heap storage
	for (int i = 0; i < n / 2; i++)    // copy all odd elements to heap storage
		b[i] = a[i * 2 + 1];
	for (int i = 0; i < n / 2; i++)    // copy all even elements to lower-half of a[]
		a[i] = a[i * 2];
	for (int i = 0; i < n / 2; i++)    // copy all odd (from heap) to upper-half of a[]
		a[i + n / 2] = b[i];
	delete[] b;                 // delete heap storage
}

// N must be a power-of-2, or bad things will happen.
// Currently no check for this condition.
//
// N input samples in X[] are FFT'd and results left in X[].
// Because of Nyquist theorem, N samples means 
// only first N/2 FFT results in X[] are the answer.
// (upper half of X[] is a reflection with no new information).
void fft2(complex<float>* X, int N) {
	if (N < 2) {
		// bottom of recursion.
		// Do nothing here, because already X[0] = x[0]
	}
	else {
		separate(X, N);      // all evens to lower half, all odds to upper half
		fft2(X, N / 2);   // recurse even items
		fft2(X + N / 2, N / 2);   // recurse odd  items
		// combine results of two half recursions
		for (int k = 0; k < N / 2; k++) {
			complex<float> e = X[k];   // even
			complex<float> o = X[k + N / 2];   // odd
						 // w is the "twiddle-factor"
			complex<float> w = exp(complex<float>(0, -2.*M_PI*k / N));
			X[k] = e + w * o;
			X[k + N / 2] = e - w * o;
		}
	}
}