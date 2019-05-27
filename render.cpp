#include <Bela.h>
#include <cstring>
#include <stdio.h>
#include <math.h>
#include <SampleLoader.h>
#include <SampleData.h>
#include <Gui.h>
#include <Compressor.h>
#include <Grain.h>
#include <Button.h>

#define BUFFER_SIZE 16384
#define WINDOW_SIZE 1024

Gui sliderGui;

const int gLedPulseTime = 20;	    // time for led pulse to stay on
int gLedState = LOW;		        // led on or off
const int gLedPin = 0;

const int gButtonPin = 2; // Freeze button
Button gButton(gButtonPin);

int kDensity = 0;  // Analog Pin of pot controlling density
int kDuration = 1;  // Analog Pin of pot controlling duration
int kSpeed = 2;  // Analog Pin of pot controlling speed
int kPosition = 3;  // Analog Pin of pot controlling position
int kBlend = 4;  // Analog Pin of pot controlling blend
int kFeedback = 5;  // Analog Pin of pot controlling feedback

float gInputBuffer[BUFFER_SIZE];   // Circular audio input buffer
int gInputBufferWritePointer = 0;  // write pointer into the buffer

float gWindow[WINDOW_SIZE];

string gFilename = "time-ended.wav";     // Name of the sound file to be read
float *gSampleBuffer;			    // Buffer holding sound file
int gSampleBufferLength;		    // The length of the buffer in frames
int gReadPointer = 0;			    // Read poisition into the sound file buffer

vector<unique_ptr<Grain>> grains;  // pool of grain objects that can be scheduled to run
const int NUM_GRAINS = 60;         // number of grain objects to initialise

int gGrainIntervalCounter = 0;    // frames since last grain onset
int nextGrainOnset;               // number of frames interval until next grain onset
bool gFrozen;

float gAudioSampleRate;           // global audio sample rate

Compressor compressor;          // compressor to perform audio limiting

using namespace std;

bool setup(BelaContext *context, void *userData)
{
    // Check the length of the audio file and allocate memory
    gSampleBufferLength = getNumFrames(gFilename);

    if(gSampleBufferLength <= 0) {
      rt_printf("Error loading audio file '%s'\n", gFilename.c_str());
      return false;
    }

    gSampleBuffer = new float[gSampleBufferLength];

    if(gSampleBuffer == 0) {
        rt_printf("Error allocating memory for the audio buffer.\n");
        return false;
    }

    // Load the sound into the file assuming mono
    getSamples(gFilename, gSampleBuffer, 0, 0, gSampleBufferLength);

    rt_printf("Loaded the audio file '%s' with %d frames (%.1f seconds)\n",
          gFilename.c_str(), gSampleBufferLength,
          gSampleBufferLength / context->audioSampleRate);

    sliderGui.setup(5432, "gui");
  	// Arguments: name, minimum, maximum, increment, default value
  	sliderGui.addSlider("Density", -NUM_GRAINS, NUM_GRAINS, 1, -10);           // number of grains per second
  	sliderGui.addSlider("Duration", 15, 250, 1, 50);            // duration of a grain in ms
    sliderGui.addSlider("Speed", 0.5, 2, 0.1, 1);               // speed of grains, affecting the pitch
    sliderGui.addSlider("Position", 0, BUFFER_SIZE - 1, 1, 0);  // position to start from in the record buffer
    sliderGui.addSlider("Blend", 0, 1, 0.01, 0.5);
    sliderGui.addSlider("Feedback", 0, 0.99, 0.01, 0.5);

    pinMode(context, 0, gLedPin, OUTPUT);
    pinMode(context, 0, gButtonPin, INPUT);

    // Set the first grain's onset
    float density  = sliderGui.getSliderValue(0);
    nextGrainOnset = context->audioSampleRate / abs(density); // set the first grain's onset

    // Initialise pool of inactive grain objects
    for (int i = 0; i < NUM_GRAINS; i++)
    {
        grains.push_back(unique_ptr<Grain>(new Grain(2200, 0, 1.0f)));
    }

    // Initialise cosine window wavetable
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        if (0 <= i && i < WINDOW_SIZE / 2)
        {
            gWindow[i] = 0.5f * (1.0f + cosf((float) 2.0f * M_PI / WINDOW_SIZE * (i - WINDOW_SIZE / 2.0f)));
        }
        else if (WINDOW_SIZE - WINDOW_SIZE / 2.0f <= i && i < WINDOW_SIZE)
        {
            gWindow[i] = 0.5f * (1.0f + cosf((float) 2.0f * M_PI / WINDOW_SIZE * (i - 1.0f - WINDOW_SIZE / 2.0f)));
        } else {
            gWindow[i] = 0.0f;
        }
    }

    // Activate the first grain
    grains[0].get()->activate();

    // Set global sample rate
    gAudioSampleRate = context->audioSampleRate;
    compressor.prepareToPlay(gAudioSampleRate);

    return true;
}

void startGrain(float density, float duration, float speed, float position)
{
    // Activate a grain object with the current params with onset at current read position
    for (int i = 0; i < grains.size(); i++)
    {
        if (grains[i].get()->isInactive())
        {
            // Initialise grain parameters
            grains[i].get()->setSpeed(speed);
            // Set the size of the grain in sample according to the current grain duration
            grains[i].get()->setSize((duration / 1000) * gAudioSampleRate);
            // Set onset of grain to current offset from write pointer
            grains[i].get()->setOnset((gInputBufferWritePointer + (int) position) % BUFFER_SIZE);

            grains[i].get()->activate(); // set grain's state to active
            break;
        }
    }

    // Schedule the next grain's onset
    if (density < 0) {
        // constant spaced interval between grains, according to the density
        nextGrainOnset = gAudioSampleRate / abs(density);
    } else if (density > 0) {
        // generate a stochastic next onset time, averaged by the density
        float r = (float)random() / (float) RAND_MAX;
        nextGrainOnset = ((-log( r )) * gAudioSampleRate) / density;
    }
    // Start LED pulse
    gLedState = HIGH;
    // Reset inter-grain onset counter
    gGrainIntervalCounter = 0;
}

void readButton(BelaContext* context, int frame) {
	/* Debounce the button
	   If the button is closed and was not closed before, then invert freeze state
	*/
	gButton.debounce(context, frame);
	if (gButton.isClosed() && !gButton.wasClosed())
	{
        // Invert the frozen state
		gFrozen = !gFrozen;
	}
}

void render(BelaContext *context, void *userData)
{
    float blend = sliderGui.getSliderValue(4);
    float feedback    = sliderGui.getSliderValue(5);

    for (unsigned int n = 0; n < context->audioFrames; n++)
    {
        // Read the next sample from the buffer
        readButton(context, n);

        float in = gSampleBuffer[gReadPointer];
        // float in = 0;
        // for(unsigned int ch = 0; ch < context->audioInChannels; ch++) {
        //     in += audioRead(context, n, ch);
        // }
        // in /= (float)context->audioInChannels;
        if(++gReadPointer >= gSampleBufferLength)
        	gReadPointer = 0;

        // Stop the led pulse if pulse time has elapsed since last grain began
        if (gGrainIntervalCounter >= gLedPulseTime * context->audioSampleRate / 1000.0)
    	{
    		gLedState = LOW;	// led off
    	}

        // Activate a grain if the current inter-grain onset interval since
        // the last grain's onset has elapsed
        if (++gGrainIntervalCounter >= nextGrainOnset)
        {
            float density = sliderGui.getSliderValue(0);
            float duration = sliderGui.getSliderValue(1);
            float speed    = sliderGui.getSliderValue(2);
            float position = sliderGui.getSliderValue(3);

            startGrain(density, duration, speed, position);
        }

        // Read from all active grains
        float out = 0.0f;
        for (int i = 0; i < grains.size(); i++)
        {
            if (!grains[i].get()->isInactive())
            {
                // Process sample for this active grain
                // Grain passed a reference to out, which will accumulate grain values
                grains[i].get()->processSample(gInputBuffer, BUFFER_SIZE, &out, gWindow, WINDOW_SIZE);
            }
        }

        compressor.processSample(&out);

        // If not in freeze mode then write incoming sample and feedback to the buffer
        if (!gFrozen) {
            // Mix audio input and feedback granulated output from the record buffer back into the record buffer
            gInputBuffer[gInputBufferWritePointer] = in + out * feedback;
            if (++gInputBufferWritePointer >= BUFFER_SIZE) gInputBufferWritePointer = 0;
        }

        // Write LED state to LED digital pin
        digitalWrite(context, n, gLedPin, gLedState);

        // Write to output the dry/wet input sample and sample output from the grains
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, in * (1 - blend) + out * blend);
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{
    delete[] gSampleBuffer;
    grains.clear();
}
