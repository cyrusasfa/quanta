#include <Bela.h>
#include <cstring>
#include <SampleLoader.h>
#include <SampleData.h>
#include <Grain.h>
#include <Gui.h>        // Need this to use the GUI

#define BUFFER_SIZE 16384				// Save a longer history

Gui sliderGui;

int gWindowSize = 1024;					// Size of the window

int gHopSize = 512;						// How often we calculate a window

float gInputBuffer[BUFFER_SIZE];		// This is the circular buffer; the window
int gInputBufferPointer = 0;			// can be taken from within it at any time

int gHopCounter = 0;					// Counter for hop size

string gFilename = "drumloop.wav"; // Name of the sound file (in project folder)
float *gSampleBuffer;			 // Buffer that holds the sound file
int gSampleBufferLength;		 // The length of the buffer in frames
int gReadPointer = 0;			 // Position of the last frame we played

// Output circular buffer
float gOutputBuffer[BUFFER_SIZE] = {0};
int gOutputBufferWritePointer = 512;
int gOutputBufferReadPointer = 0;

unique_ptr<Grain> grain;
vector<unique_ptr<Grain>> grains;

int gGrainIntervalCounter = 0;

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

    // Make sure the memory allocated properly
    if(gSampleBuffer == 0) {
        rt_printf("Error allocating memory for the audio buffer.\n");
        return false;
    }

    // Load the sound into the file (note: this example assumes a mono audio file)
    getSamples(gFilename, gSampleBuffer, 0, 0, gSampleBufferLength);

    rt_printf("Loaded the audio file '%s' with %d frames (%.1f seconds)\n",
          gFilename.c_str(), gSampleBufferLength,
          gSampleBufferLength / context->audioSampleRate);


    grain = unique_ptr<Grain>(new Grain(gSampleBufferLength, 0, 2.0f));
    for (int i = 0; i < 16; i++)
    {
        grains.push_back(unique_ptr<Grain>(new Grain(gSampleBufferLength, 0, 0.5f)));
    }

    grains[0].get()->activate();

    sliderGui.setup(5432, "gui");
  	// Arguments: name, minimum, maximum, increment, default value
  	sliderGui.addSlider("Density", 0.5, 16, 0.5, 2);
  	sliderGui.addSlider("Duration", 5, 100, 1, 50);
    sliderGui.addSlider("Speed", 0.1, 2, 0.1, 1);
    sliderGui.addSlider("Position", 0, BUFFER_SIZE - 1, 1, BUFFER_SIZE / 2);
    sliderGui.addSlider("Texture", 0, 1, 0.01, 0.5);

    return true;
}

void startGrain() {
    gGrainIntervalCounter = 0;
    for (int i = 0; i < grains.size(); i++)
    {
        if (grains[i].get()->isInactive())
        {
            grains[i].get()->activate();
            break;
        }
    }
}

void render(BelaContext *context, void *userData)
{
    float density  = sliderGui.getSliderValue(0);
    float duration = sliderGui.getSliderValue(1);
    float speed    = sliderGui.getSliderValue(2);

    for(unsigned int n = 0; n < context->audioFrames; n++) {
        // Read the next sample from the buffer
        float in = gSampleBuffer[gReadPointer];
        if(++gReadPointer >= gSampleBufferLength)
        	gReadPointer = 0;

        gInputBuffer[gInputBufferPointer] = in;
        if (++gInputBufferPointer >= BUFFER_SIZE) gInputBufferPointer = 0;

        float out = 0.0f;
        // grain.get()->processSample(gSampleBuffer, gSampleBufferLength, gReadPointer, &out);

        for (int i = 0; i < grains.size(); i++)
        {
            if (!grains[i].get()->isInactive())
            {
                // grains[i].get()->setSize(duration / 1000 * context->audioSampleRate);
                // grains[i].get()->setDuration(duration / 1000 * context->audioSampleRate);
                // grains[i].get()->setDuration(gSampleBufferLength);
                // grains[i].get()->setSpeed(speed);
                grains[i].get()->processSample(gSampleBuffer, gSampleBufferLength, gReadPointer, &out);
            }
        }

        // if (++gGrainIntervalCounter >= context->audioSampleRate / density)
    	// {
		// 	startGrain();
    	// }

		// Copy input to output
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}
	}
}

// for (int i = 0; i < 2048; i++) {
//     double multiplier = 0.5 * (1 - cos(2*PI*i/2047));
//     dataOut[i] = multiplier * dataIn[i];
// }

void cleanup(BelaContext *context, void *userData)
{
    delete[] gSampleBuffer;
}
