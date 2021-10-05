CFLAGS := -Wall -Wextra -O2
LDLIBS := -lm

all: buzzer

clean:
	$(RM) buzzer
