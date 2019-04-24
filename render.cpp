#include <Bela.h>
#include <cstring>
#include <SampleLoader.h>
#include <SampleData.h>
#include <Grain.h>
#include <Gui.h>
#include <Scope.h>

#define BUFFER_SIZE 16384				// Save a longer history
#define WINDOW_SIZE 1024

Gui sliderGui;

const int gLedPulseTime = 40;	    // time for led pulse to stay on
int gLedState = LOW;		// led on or off
const int gLedPin = 0;

float gInputBuffer[BUFFER_SIZE];		// This is the circular buffer; the window
int gInputBufferWritePointer = 0;			// can be taken from within it at any time

float gWindow[WINDOW_SIZE];


string gFilename = "drumloop.wav"; // Name of the sound file (in project folder)
float *gSampleBuffer;			 // Buffer that holds the sound file
int gSampleBufferLength;		 // The length of the buffer in frames
int gReadPointer = 0;			 // Position of the last frame we played

vector<unique_ptr<Grain>> grains;
const int NUM_GRAINS = 32;

int gGrainIntervalCounter = 0;
int nextGrainOnset;

Scope gScope;
float gAudioSampleRate;

// Thread for grain scheduling
AuxiliaryTask gScheduleGrainTask;

using namespace std;

void schedule_grain_background(void *);

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
  	sliderGui.addSlider("Density", -15, 15, 1, -10);           // number of grains per second
  	sliderGui.addSlider("Duration", 15, 250, 1, 50);            // duration of a grain in ms
    sliderGui.addSlider("Speed", 0.1, 2, 0.1, 1);               // speed of grains, affecting the pitch
    sliderGui.addSlider("Position", 0, BUFFER_SIZE - 1, 1, 0);  // position to start from in the record buffer
    sliderGui.addSlider("Texture", 0.5, 1, 0.01, 1);
    sliderGui.addSlider("Blend", 0, 1, 0.01, 0.5);
    sliderGui.addSlider("Freeze", 0, 1, 1, 0.0f);
    sliderGui.addSlider("Feedback", 0, 0.99, 0.01, 0.5);

    gScope.setup(1, context->digitalSampleRate);

    pinMode(context, 0, gLedPin, OUTPUT);

    // Set the first grain's onset
    float density  = sliderGui.getSliderValue(0);
    nextGrainOnset = context->audioSampleRate / abs(density);

    // Initialise pool of inactive grain objects
    for (int i = 0; i < NUM_GRAINS; i++)
    {
        grains.push_back(unique_ptr<Grain>(new Grain(2200, 0, 1.0f, 1.0)));
    }

    // Initialise cosine window wavetable
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        if (0 <= i && i < WINDOW_SIZE / 2)
        {
            return 0.5f * (1.0f + cosf((float) 2.0f * M_PI / WINDOW_SIZE * (i - WINDOW_SIZE / 2.0f)));
        }
        else if (WINDOW_SIZE - WINDOW_SIZE / 2.0f <= i && i <= WINDOW_SIZE)
        {
            return 0.5f * (1.0f + cosf((float) 2.0f * M_PI / WINDOW_SIZE * (i - 1.0f - WINDOW_SIZE / 2.0f)));
        }
    }

    // Activate the first grain
    grains[0].get()->activate();

    // Set global sample rate
    gAudioSampleRate = context->audioSampleRate;

    gScheduleGrainTask = Bela_createAuxiliaryTask(schedule_grain_background, 50, "bela-schedule-grain");

    return true;
}

void startGrain(float sampleRate) {
    float density = sliderGui.getSliderValue(0);
    float duration = sliderGui.getSliderValue(1);
    float speed    = sliderGui.getSliderValue(2);
    float position = sliderGui.getSliderValue(3);
    float texture   = sliderGui.getSliderValue(4);

    // Activate a grain object with the current params with onset at current read position
    for (int i = 0; i < grains.size(); i++)
    {
        if (grains[i].get()->isInactive())
        {
            // TODO: randomly set onset? vary duration?
            // Set the grains params
            grains[i].get()->setSpeed(speed);
            grains[i].get()->setSize((duration / 1000) * sampleRate);
            // Set onset of grain to current offset from write pointer
            grains[i].get()->setOnset((gInputBufferWritePointer + (int) position) % BUFFER_SIZE);
            grains[i].get()->setTexture(texture);
            grains[i].get()->activate();
            break;
        }
    }

    // Schedule the next grain's onset
    if (density < 0) {
        // constant spaced interval between grains, according to the density
        nextGrainOnset = sampleRate / abs(density);
    } else if (density > 0) {
        // generate a stochastic next onset time averaged by the density
        float r = (float)random() / (float) RAND_MAX;
        nextGrainOnset = ((-log( r )) * sampleRate) / density;
    }

    // Start LED pulse
    gLedState = HIGH;
    // Reset inter-grain onset counter
    gGrainIntervalCounter = 0;
}

void schedule_grain_background(void * arg)
{
	startGrain(gAudioSampleRate);
}

void render(BelaContext *context, void *userData)
{
    float blend    = sliderGui.getSliderValue(5);
    float freeze   = sliderGui.getSliderValue(6);
    float feedback = sliderGui.getSliderValue(7);

    for(unsigned int n = 0; n < context->audioFrames; n++) {
        // Read the next sample from the buffer
        float in = gSampleBuffer[gReadPointer];
        if(++gReadPointer >= gSampleBufferLength)
        	gReadPointer = 0;

        // Stop the led pulse if pulse time has elapsed since last grain began
        if (gGrainIntervalCounter >= gLedPulseTime * context->audioSampleRate / 1000.0)
    	{
    		gLedState = LOW;	// led off
    	}

        if (++gGrainIntervalCounter >= nextGrainOnset)
        {
            Bela_scheduleAuxiliaryTask(gScheduleGrainTask);
        }

        // Read from all the active grains
        float out = 0.0f;
        for (int i = 0; i < grains.size(); i++)
        {
            if (!grains[i].get()->isInactive())
            {
                // Process sample for this active grain
                // Grain passed a reference to out, which will accumulate grain values
                grains[i].get()->processSample(gInputBuffer, BUFFER_SIZE, &out);
            }
        }

        // If not in freeze mode then write incoming sample and feedback to the buffer
        if (!freeze) {
            gInputBuffer[gInputBufferWritePointer] = in + out * feedback;
            if (++gInputBufferWritePointer >= BUFFER_SIZE) gInputBufferWritePointer = 0;
        }

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
}
