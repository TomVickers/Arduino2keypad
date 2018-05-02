# Makefile for USB2keybus project
#
# To use the Makefile method for building the project:
#   sudo apt-get install gcc-avr avr-libc avrdude
#   wget https://github.com/arduino/Arduino/archive/master.zip
#     unzip archive and rename top dir /usr/share/Arduino

# parameters for avrdude
BAUD=115200
AVR_TYPE=atmega2560
AVR_FREQ=16000000L
PROGRAM_DEV=/dev/ttyACM0
PROGRAM_TYPE=wiring

# path to Arduino lib source code
ARDUINO_CORE_PATH=/usr/share/Arduino/hardware/arduino/avr/cores/arduino
ARDUINO_VARIANT_PATH=/usr/share/Arduino/hardware/arduino/avr/variants/mega

DEFINES=-DF_CPU=$(AVR_FREQ) -DARDUINO=10802 -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR
INCLUDES= -I$(ARDUINO_CORE_PATH) -I$(ARDUINO_VARIANT_PATH)
DEF_FLAGS= $(DEFINES) $(INCLUDES) -mmcu=$(AVR_TYPE) -Wall -Os -ffunction-sections -fdata-sections
CFLAGS=$(DEF_FLAGS) -fno-fat-lto-objects
CPPFLAGS= $(DEF_FLAGS) -fno-exceptions -fpermissive -fno-exceptions -fno-threadsafe-statics
LINK_FLAGS= -w -Os -flto -fuse-linker-plugin -Wl,--gc-sections,--relax -mmcu=$(AVR_TYPE)

PROJ_SRCS= \
	KeypadSerial.cpp       \
	ModSoftwareSerial.cpp  \
	PiSerial.cpp           \
    USB2keybus.cpp         \
	USBprotocol.cpp        \
	Volts.cpp

# list of all the Arduino core .c & .cpp files
CORE_C_SRCS = $(wildcard $(ARDUINO_CORE_PATH)/*.c)
CORE_CPP_SRCS = $(wildcard $(ARDUINO_CORE_PATH)/*.cpp)

# if you want to exclude any core files not needed for your build, list them here
CORE_EXCLUDE= IPAddress.o PluggableUSB.o Tone.o

# build the list of needed obj files from the source files
OBJ_LIST1=$(patsubst %.cpp,%.o,$(PROJ_SRCS)) 
OBJ_LIST1+=$(notdir $(patsubst %.cpp,%.o,$(CORE_CPP_SRCS)))
OBJ_LIST1+=$(notdir $(patsubst %.c,%.o,$(CORE_C_SRCS)))
OBJ_LIST1+=wiring_pulse_asm.o

# location for built objects
OBJDIR=obj

# the final obj list
OBJS=$(addprefix $(OBJDIR)/,$(filter-out $(CORE_EXCLUDE),$(OBJ_LIST1)))

.PHONY: flash clean

all: main.hex
	@echo build complete

USB2keybus.cpp: USB2keybus.ino
	@ln -sf USB2keybus.ino USB2keybus.cpp

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	avr-g++ $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(ARDUINO_CORE_PATH)/%.cpp
	@mkdir -p $(OBJDIR)
	avr-g++ $(CPPFLAGS) -c $< -o $(OBJDIR)/$(notdir $@)

$(OBJDIR)/%.o: $(ARDUINO_CORE_PATH)/%.c
	@mkdir -p $(OBJDIR)
	avr-gcc $(CFLAGS) -c $< -o $(OBJDIR)/$(notdir $@)

$(OBJDIR)/%.o: $(ARDUINO_CORE_PATH)/wiring_pulse.S
	@mkdir -p $(OBJDIR)
	avr-gcc $(CFLAGS) -x assembler-with-cpp -c $< -o $(OBJDIR)/wiring_pulse_asm.o

main.elf: $(OBJS)
	avr-gcc $(LINK_FLAGS) -o $@ $^
	avr-size --mcu=$(AVR_TYPE) -C main.elf

# save for later (use this instead?)
#	@avr-objcopy -j .text -j .data -O ihex $^ $@
main.hex: main.elf
	@avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings \
		--change-section-lma .eeprom=0 main.elf main.eep
	@avr-objcopy -O ihex -R .eeprom main.elf main.hex

flash: main.hex
	avrdude -v -p $(AVR_TYPE) -c $(PROGRAM_TYPE) -P $(PROGRAM_DEV) -b $(BAUD) -D -U flash:w:$<:i

clean:
	rm -rf main.hex main.elf main.eep $(OBJDIR)
