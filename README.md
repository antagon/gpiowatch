# gpiowatch

Monitor GPIO pins and execute a command.

## Usage

	Usage: gpiowatch -c <config-file>

	Options:
	 -c <file> load a configuration file
	 -v        show version information
	 -h        show usage information

## Configuration

	#
	# Example configuration file for gpiowatch.
	# Columns: PIN[ & PIN ...]; threshold-sec; command
	#
	
	# Execute a command if GPIO pins 16, 20, 21 are set to logical 1 (true) for
	# four consecutive seconds.
	16 & 20 & 21; 4; echo "Running the command (as user $(whoami), via $SHELL)"

## Compilation

	$ make

## Dependencies

* glibc

* Sysfs GPIO interface (must be enabled in the kernel)

## License

This software may be modified and distributed under the terms of the MIT
license. See the LICENSE file for details.

## References

https://www.kernel.org/doc/Documentation/gpio/sysfs.txt

http://elinux.org/GPIO

https://pinout.xyz/

