#define FLUIDSYNTH_URI       "http://www.joebutton.co.uk/software/lv2/fluidsynth"
#define FLUIDSYNTH_UI_URI       "http://www.joebutton.co.uk/software/lv2/fluidsynth/gui"
#define MIDI         0
#define CHAN1PROGRAM 1
#define LEFT         2
#define RIGHT        3

#define FLUID_TIME_SCALE 1000

// MIDI opcodes
#define NOTE_OFF                0x80
#define NOTE_ON                 0x90
#define KEY_PRESSURE            0xA0
#define CONTROL_CHANGE          0xB0
#define PROGRAM_CHANGE          0xC0
#define CHANNEL_PRESSURE        0xD0
#define PITCH_BEND              0xE0
#define MIDI_SYSTEM_RESET       0xFF
