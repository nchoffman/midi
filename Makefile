
miditest: miditest.c pmfunctions.c
	gcc $? -o miditest -lportmidi -lporttime

clean:
	-rm miditest