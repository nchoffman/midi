#include "portmidi.h"
#include "porttime.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"

#define INPUT_BUFFER_SIZE 100
#define OUTPUT_BUFFER_SIZE 0
#define DRIVER_INFO NULL
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define TIME_START Pt_Start(1, 0, 0) /* timer started w/millisecond accuracy */

#define STRING_MAX 80 /* used for console input */

int32_t latency_ = 0;

/* crash the program to test whether midi ports are closed */
/**/




void main_test_both()
{
    int i = 0;
    int in, out;
    PmStream * midi, * midiOut;
    PmEvent buffer[1];
    PmEvent outputBuffer[4];
    PmError status, length;
    int num = 10;
    
    int midi_num;
    int midi_vel;
    int midi_status;

    float tempo, time1 = -1, time2;


    in = get_number("Type input number: ");
    out = get_number("Type output number: ");

    /* In is recommended to start timer before PortMidi */
    TIME_START;

    Pm_OpenOutput(&midiOut, 
                  out, 
                  DRIVER_INFO,
                  OUTPUT_BUFFER_SIZE, 
                  TIME_PROC,
                  TIME_INFO, 
                  latency_);
    printf("Midi Output opened with %ld ms latency_.\n", (long) latency_);
    /* open input device */
    Pm_OpenInput(&midi, 
                 in,
                 DRIVER_INFO, 
                 INPUT_BUFFER_SIZE, 
                 TIME_PROC, 
                 TIME_INFO);
    printf("Midi Input opened. Reading %d Midi messages...\n",num);
    Pm_SetFilter(midi, PM_FILT_ACTIVE | PM_FILT_CLOCK);
    /* empty the buffer after setting filter, just in case anything
       got through */
    while (Pm_Poll(midi)) {
        Pm_Read(midi, buffer, 1);
    }
    i = 0;

    while (1) {
        status = Pm_Poll(midi);

	
        if (status == TRUE) {
            length = Pm_Read(midi,buffer,1);

            if (length > 0) {

		time2 = buffer[0].timestamp;

		midi_status = Pm_MessageStatus(buffer[0].message);
		midi_num = Pm_MessageData1(buffer[0].message);
		midi_vel = Pm_MessageData2(buffer[0].message);


		if(midi_status == 144) {
			tempo = time2 - time1;
			if(tempo > 5000) {
				time1 = -1;
			}
		}	



		outputBuffer[0].timestamp = buffer[0].timestamp;
		outputBuffer[0].message = Pm_Message(midi_status, midi_num, midi_vel);
		Pm_Write(midiOut, outputBuffer, 1);

		if (time1 < 0) {
			time1 = time2;
			continue;
		}

		outputBuffer[0].message = Pm_Message(midi_status, midi_num, midi_vel);
		outputBuffer[1].message = Pm_Message(midi_status, midi_num+5, midi_vel);
		outputBuffer[2].message = Pm_Message(midi_status, midi_num+7, midi_vel);
		outputBuffer[0].timestamp = tempo + buffer[0].timestamp;
		outputBuffer[1].timestamp = tempo + buffer[0].timestamp;
		outputBuffer[2].timestamp = tempo + buffer[0].timestamp;
                Pm_Write(midiOut, outputBuffer, 3);



                printf("Got message %d: time %ld, %d %d %d\n",
                       i,
                       (long) buffer[0].timestamp,
                        Pm_MessageStatus(buffer[0].message), /*NOTE ON(90)/OFF(80), b0 - knobs*/
                        Pm_MessageData1(buffer[0].message),  /*MIDI NOTE NUMBER*/
                        Pm_MessageData2(buffer[0].message)); /*VELOCITY, 0-127*/
                i++;

		if(Pm_MessageData1(buffer[0].message) == 36) {
			printf("Closing program...\n");
			break;
		}
            } else {
                assert(0);
            }
        }
		

    }

    /* close midi devices */
    Pm_Close(midi);
    Pm_Close(midiOut);
    Pm_Terminate(); 
}



int main(int argc, char *argv[])
{
    int default_in;
    int default_out;
    int i = 0, n = 0;
    char line[STRING_MAX];
    int test_input = 0, test_output = 0, test_both = 0, somethingStupid = 0;
    int stream_test = 0;
    int latency_valid = FALSE;
    
    if (sizeof(void *) == 8) 
        printf("Apparently this is a 64-bit machine.\n");
    else if (sizeof(void *) == 4) 
        printf ("Apparently this is a 32-bit machine.\n");
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            show_usage();
        } else if (strcmp(argv[i], "-l") == 0 && (i + 1 < argc)) {
            i = i + 1;
            latency_ = atoi(argv[i]);
            printf("Latency will be %ld\n", (long) latency_);
            latency_valid = TRUE;
        } else {
            show_usage();
        }
    }

    while (!latency_valid) {
        int lat; // declared int to match "%d"
        printf("Latency in ms: ");
        latency_ = 10; // coerce from "%d" to known size
  	latency_valid = TRUE;
    }

    /* determine what type of test to run */
    printf("begin portMidi test...\n");
    
    
    /* list device information */
    default_in = Pm_GetDefaultInputDeviceID();
    default_out = Pm_GetDefaultOutputDeviceID();
    for (i = 0; i < Pm_CountDevices(); i++) {
        char *deflt;
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (((test_input  | test_both) & info->input) |
            ((test_output | test_both | stream_test) & info->output)) {
            printf("%d: %s, %s", i, info->interf, info->name);
            if (info->input) {
                deflt = (i == default_in ? "default " : "");
                printf(" (%sinput)", deflt);
            }
            if (info->output) {
                deflt = (i == default_out ? "default " : "");
                printf(" (%soutput)", deflt);
            }
            printf("\n");
        }
    }
    
    /* run test */
    main_test_both();
    
    printf("finished portMidi test...type ENTER to quit...");
    fgets(line, STRING_MAX, stdin);
    return 0;
}
