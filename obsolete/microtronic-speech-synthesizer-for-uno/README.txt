This is a simple speech synthesizer for the Microtronic.  It uses the
Emic-2 speech module, connected to an Uno which serves as an
intelligent buffer for the Microtronic. The Emic-2 requires a 9600
baud serial connection - that's a little bit too fast for the
Microtronic. 

The included .lisp program can be used to create speech programs
for the Microtronic. Load it into your favorite Common Lisp and
evaluate 

(microtronic-speech-program "Hello Microtronic!" "SPEECH1.MIC")

Put the generic SPEECH1.MIC file onto an SDCard and load it into the
emulator (PGM 1). If everything was set up correctly, the Emic-2 will
start speaking after a while. You might also have to tweak the CPU
speed throttle. Watch the output on the serial monitor.

