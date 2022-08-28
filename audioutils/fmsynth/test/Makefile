SRCS = ../fmsynth_eg.c ../fmsynth_op.c ../fmsynth.c
CFLAGS = -DFAR= -DCODE= -DOK=0 -DERROR=-1 -I .. -I ../../../include -g

TARGETS = opfunctest fmsyntheg_test fmsynthop_test fmsynth_test fmsynth_alsa

all: $(TARGETS)

opfunctest: $(SRCS) opfunc_test.c
	gcc $(CFLAGS) -o $@ $^ -lm

fmsyntheg_test: $(SRCS) fmsynth_eg_test.c
	gcc $(CFLAGS) -o $@ $^

fmsynthop_test: $(SRCS) fmsynth_op_test.c
	gcc $(CFLAGS) -o $@ $^

fmsynth_test: $(SRCS) fmsynth_test.c
	gcc $(CFLAGS) -o $@ $^

fmsynth_alsa: $(SRCS) fmsynth_alsa_test.c
	gcc $(CFLAGS) -o $@ $^ -lasound

clean:
	rm -rf $(TARGETS)
