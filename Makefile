ifeq ($(OS), Windows_NT)
	TARGET = cpu_scheduling_simulator.exe
	RM = del /Q
else
	TARGET = cpu_scheduling_simulator
	RM = rm -f
endif

c_file = main.c config.c random_distribution.c
h_file = config.h color.h random_distribution.h

CFLAGS = -Wall -Wextra -g -lm
OBJS = $(c_file:.c=.o)

$(TARGET): $(OBJS)
	gcc -o $(TARGET) $(OBJS) $(CFLAGS)

%.o: %.c $(h_file)
	gcc -o $@ -c $<

obj_clean:
	$(RM) *.o
clean:
	$(RM) *.o $(TARGET)