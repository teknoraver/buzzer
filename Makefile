CFLAGS := -Wall -Wextra -O2
LDFLAGS := -lm

all: buzzer

clean:
	$(RM) buzzer
