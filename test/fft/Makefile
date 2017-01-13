TARGET=fft
include ../shared/Makefile.shared

$(TARGET): $(TARGET).o
	$(CC) $(TARGET).o -lm $(SNIPER_LDFLAGS) -o $(TARGET)

run_$(TARGET):
	../../run-sniper -n 4 -c gainestown -senergystats --roi --viz -- ./fft -p 4

CLEAN_EXTRA=viz
