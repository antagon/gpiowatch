CFLAGS=-O3 -std=gnu99 -pedantic -Wall -ggdb
LDFLAGS=

TARGET=gpiowatch

.PHONY: all clean

all: $(TARGET)

clean:
	$(RM) *.o $(TARGET)

$(TARGET): main.o sysfs_gpio.o
	$(CC) $(CFLAGS) $^ -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

