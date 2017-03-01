# gpiowatch

## Usage

	Usage: gpiowatch -c <config-file>

	Options:
	 -c <file> load a configuration file
	 -v        show version information
	 -h        show usage information

## Configuration

	# Wait for GPIO pins 16, 20, 21 to be set to logical 1 (true) for four
	# consecutive seconds, and execute a command...
	16 & 20 & 21; 4; echo Running the command (as user $(whoami), via $SHELL)...
	# ... other commands may follow.

## License

This software may be modified and distributed under the terms of the MIT
license. See the LICENSE file for details.

