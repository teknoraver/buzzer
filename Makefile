CFLAGS := -Wall -Wextra -O2

buzzer: buzzer.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) buzzer