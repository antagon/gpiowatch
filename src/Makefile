CFLAGS=-O3 -std=gnu99 -pedantic -Wall -ggdb
LDFLAGS=-lm

TARGET=gpiowatch

.PHONY: all clean

all: $(TARGET)

clean:
	$(RM) *.o $(TARGET)

$(TARGET): main.o sysfs_gpio.o config.o
	$(CC) $(CFLAGS) $^ -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

