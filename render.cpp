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
int gInputBufferWritePointer = 0;			// can be taken from within it at any time

int gHopCounter = 0;					// Counter for hop size

string gFilename = "drumloop.wav"; // Name of the sound file (in project folder)
float *gSampleBuffer;			 // Buffer that holds the sound file
int gSampleBufferLength;		 // The length of the buffer in frames
int gReadPointer = 0;			 // Position of the last frame we played

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

    if(gSampleBuffer == 0) {
        rt_printf("Error allocating memory for the audio buffer.\n");
        return false;
    }

    // Load the sound into the file assuming mono
    getSamples(gFilename, gSampleBuffer, 0, 0, gSampleBufferLength);

    rt_printf("Loaded the audio file '%s' with %d frames (%.1f seconds)\n",
          gFilename.c_str(), gSampleBufferLength,
          gSampleBufferLength / context->audioSampleRate);

    for (int i = 0; i < 16; i++)
    {
        grains.push_back(unique_ptr<Grain>(new Grain(2200, 0, 1.0f)));
    }

    grains[0].get()->activate();

    sliderGui.setup(5432, "gui");
  	// Arguments: name, minimum, maximum, increment, default value
  	sliderGui.addSlider("Density", 0.5, 80, 0.5, 10);           // number of concurrent grains per second
  	sliderGui.addSlider("Duration", 5, 200, 1, 100);            // duration of a grain in ms
    sliderGui.addSlider("Speed", 0.1, 2, 0.1, 1);               // speed of grains, affecting the pitch
    sliderGui.addSlider("Position", 0, BUFFER_SIZE - 1, 1, 0);  // position to start from in the record buffer
    sliderGui.addSlider("Texture", 0, 1, 0.01, 0.5);
    sliderGui.addSlider("Blend", 0, 1, 0.01, 0.5);

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
    float position = sliderGui.getSliderValue(3);
    float blend    = sliderGui.getSliderValue(5);

    for(unsigned int n = 0; n < context->audioFrames; n++) {
        // Read the next sample from the buffer
        float in = gSampleBuffer[gReadPointer];
        if(++gReadPointer >= gSampleBufferLength)
        	gReadPointer = 0;

        gInputBuffer[gInputBufferWritePointer] = in;
        if (++gInputBufferWritePointer >= BUFFER_SIZE) gInputBufferWritePointer = 0;

        float out = 0.0f;

        for (int i = 0; i < grains.size(); i++)
        {
            if (!grains[i].get()->isInactive())
            {
                // grains[i].get()->setSize(duration / 1000 * context->audioSampleRate);
                grains[i].get()->setOnset(position);
                grains[i].get()->setSpeed(speed);
                grains[i].get()->setSize((duration / 1000) * context->audioSampleRate);
                grains[i].get()->processSample(gInputBuffer, BUFFER_SIZE, &out);
            }
        }

        if (++gGrainIntervalCounter >= context->audioSampleRate / density)
    	{
			startGrain();
    	}

		// Copy input to output
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{
    delete[] gSampleBuffer;
}
