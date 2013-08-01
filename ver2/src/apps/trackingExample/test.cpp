#include <stdio.h>
#include <vector>

#include "vrpn/vrpn_Tracker.h"
#include "vrpn/vrpn_Button.h"
#include "vrpn/vrpn_Analog.h"

std::vector<vrpn_Tracker_Remote *> mTrackers;
vrpn_Analog_Remote * mAnalogDevice = NULL;
vrpn_Button_Remote * mButtonDevice = NULL;

void VRPN_CALLBACK update_tracker_cb(void *userdata, const vrpn_TRACKERCB t );
void VRPN_CALLBACK update_button_cb(void *userdata, const vrpn_BUTTONCB b );
void VRPN_CALLBACK update_analog_cb(void * userdata, const vrpn_ANALOGCB a );

int main( int argc, char* argv[] )
{

    fprintf(stderr, "Tracking test.\n");

    fprintf(stderr, "connecting to positional device...\n");
    mTrackers.push_back(NULL);
    mTrackers[0] = new vrpn_Tracker_Remote( "IS900@130.236.142.25" );
    mTrackers.push_back(NULL);

    fprintf(stderr, "connecting to analog device...\n");
    mAnalogDevice = new vrpn_Analog_Remote( "Wand@130.236.142.25" );

    fprintf(stderr, "connecting to button device...\n");
    mButtonDevice = new vrpn_Button_Remote( "Wand@130.236.142.25" );

    fprintf(stderr, "Setting callback to positional device...\n");
    mTrackers[0]->register_change_handler(NULL, update_tracker_cb);

    fprintf(stderr, "Setting callback to analog device...\n");
    mAnalogDevice->register_change_handler(NULL, update_analog_cb);

    fprintf(stderr, "Setting callback to button device...\n");
    mButtonDevice->register_change_handler(NULL, update_button_cb);

    fprintf(stderr, "Starting loop.\n");

    while(true)
    {
        mTrackers[0]->mainloop();
        mAnalogDevice->mainloop();
        mButtonDevice->mainloop();

        vrpn_SleepMsecs(1);
    }

	// Exit program
	exit( EXIT_SUCCESS );
}


void VRPN_CALLBACK update_tracker_cb(void *userdata, const vrpn_TRACKERCB info)
{
	fprintf(stderr, "Sensor: %d, Pos: x=%lf\ty=%lf\tz=%lf\n", info.sensor, info.pos[0], info.pos[1], info.pos[2]);
}

void VRPN_CALLBACK update_button_cb(void *userdata, const vrpn_BUTTONCB b )
{
	fprintf(stderr, "Button: %d, state: %d\n", b.button, b.state);
}

void VRPN_CALLBACK update_analog_cb(void* userdata, const vrpn_ANALOGCB a )
{
	for( int i=0; i < a.num_channel; i++ )
	{
		fprintf(stderr, "Analog: %d, value: %lf\n", i, a.channel[i]);
	}
}
