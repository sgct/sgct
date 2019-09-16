#include <vrpn/vrpn_Analog.h>
#include <vrpn/vrpn_Button.h>
#include <vrpn/vrpn_Tracker.h>
#include <memory>
#include <vector>

std::unique_ptr<vrpn_Tracker_Remote> mTracker;
vrpn_Analog_Remote* mAnalogDevice = nullptr;
vrpn_Button_Remote* mButtonDevice = nullptr;

void VRPN_CALLBACK update_tracker_cb(void*, vrpn_TRACKERCB info) {
    fprintf(
        stderr,
        "Sensor: %d, Pos: x=%lf\ty=%lf\tz=%lf\n",
        info.sensor, info.pos[0], info.pos[1], info.pos[2]
    );
}

void VRPN_CALLBACK update_button_cb(void*, vrpn_BUTTONCB b) {
    fprintf(stderr, "Button: %d, state: %d\n", b.button, b.state);
}

void VRPN_CALLBACK update_analog_cb(void*, vrpn_ANALOGCB a) {
    for (int i = 0; i < a.num_channel; i++ ) {
        fprintf(stderr, "Analog: %d, value: %lf\n", i, a.channel[i]);
    }
}


int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);

    fprintf(stderr, "Tracking test.\n");

    fprintf(stderr, "connecting to positional device...\n");
    mTracker = std::make_unique<vrpn_Tracker_Remote>("IS900@130.236.142.25");

    fprintf(stderr, "connecting to analog device...\n");
    mAnalogDevice = new vrpn_Analog_Remote("Wand@130.236.142.25");

    fprintf(stderr, "connecting to button device...\n");
    mButtonDevice = new vrpn_Button_Remote("Wand@130.236.142.25");

    fprintf(stderr, "Setting callback to positional device...\n");
    mTracker->register_change_handler(nullptr, update_tracker_cb);

    fprintf(stderr, "Setting callback to analog device...\n");
    mAnalogDevice->register_change_handler(nullptr, update_analog_cb);

    fprintf(stderr, "Setting callback to button device...\n");
    mButtonDevice->register_change_handler(nullptr, update_button_cb);

    fprintf(stderr, "Starting loop.\n");

    while (true) {
        mTracker->mainloop();
        mAnalogDevice->mainloop();
        mButtonDevice->mainloop();

        vrpn_SleepMsecs(1);
    }

    exit(EXIT_SUCCESS);
}
