# tcRackModules
## Blur
Usage Hints: 

**Buffer Length** - sets the number of seconds worth of FFT frames to buffer up. Values are 0 .. 10 seconds. (0 seconds is keeping just a single frame in the buffer)

**Playback Pos** - the position in the frame buffer where the output is generated from. Think of the frame buffer as a kind of delay line and the position being the delay time. Modulation the position plays the frames forward and backward. 
Modulate slower/faster than real time to get slowed down and sped up playback.

**Blur** - sets the amount of bin scrambling. The bin picker selects bin values randomly within a range of frames on either side of the playback position. The Blur control sets the distance away from the position that the picker can pick from. A 0 value means use only local bins. A 1 value means pick from any frame in the buffer.

**Freq Span** - defines a width of frequencies to apply the blurring to. A vaue of 0 applies blurring to no frequeny bins. A value of 1 applies blurring to all frequency bins.

**Freq Center** - the center frequency of the Freq Span.

**Blur Mix** - controls the wet/dry mix of the blurred and non-blurred output. Try high blur with low mix and vice-versa.

**Robot** - simulates classic robot effect by setting all phases to 0. Blur Mix also controls the wet/dry ratio for this effect. Watch your levels with this effect, robotizing seems to increase or decrease the overall level of the output by a few dB depending on the signal being affected.

**Pitch / Semitone** - increase or decrease pitch. When Semitone is active, the pitch can swing from +/- 3 octaves in semitone steps. When semitone is not active the pitch switch is smooth - CCW motion takes the pitch down to very low, barely audible, good for tape-stop type effects. CW motion on the right of the dial raises the pitch up to 2 octaves higher.

**Frame Drop** - sets the probability of discarding new audio input. Smaller values let more frames in, larger values let fewer frames in. At 0, all incoming audio is framed up and passed into the history buffer. At 1, no new audio is added to the frame buffer, effectively freezing the contents of the buffer. NOTE: This caught me a few times where I saved a rack with Frame Dropper set to 1 while manipulating captured audio. On re-opening the rack, Blur made no output. It took me a few minutes to realize that the Dropper was full on, suppressing incoming audio. Backing off the Dropper opened the pipe for new audio. (Note to self: add a light or something indicating that audio is suppressed)  

**Gain** - output gain +/- 6dB to adjust as needed.

**Popup Menu** has options to set the FFT size and the Oversample amount. Low FFT sizes are good for distorting the sound. Low oversample amounts also tend to introduce noise and artifacts. Larger FFT sizes with oversamlple of 4 or 8 provide better clarity. FFT Size 2048 with oversample 4 is a good starting point. Experimentation is the key!

