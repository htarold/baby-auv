# (C) 2019 Harold Tay GPLv3
# Makefile for Baby AUV operational and testing code.
# For compiling:
CC = avr-gcc
OBJCOPY = avr-objcopy
#CFLAGS= -DCOMPILED="\"`date +'Build %H:%M:%S %d/%m/%Y'`\"" -std=c99 -O -g -Wall
CFLAGS= -std=c99 -fshort-enums -DF_CPU=8000000UL -O -Os -g -Wall \
-mmcu=atmega328p -fdata-sections -ffunction-sections -Wl,--gc-sections \
-DIDENT=$@
#DUDEFLAGS=-v -v -F -p `[ -f PARTNO ] && cat PARTNO || cat $*.partno` -cusbasp -F
DUDEFLAGS=-v -v -F -p m328 -carduino -P /dev/ttyUSB0 -b 57600

bins= extimes.elf tasks_antennas.elf tasks_ctrl.elf \
tasks_diptest.elf tasks_fg2.elf tasks_mission.elf tasks_pitch.elf \
tasks_repl.elf tasks_rftest.elf tasks_swim2.elf tasks_swim.elf \
tasks_thrust.elf tasks_tracker.elf test_accel.elf test_ads1015.elf \
test_cmpas.elf test_division.elf \
test_ctd-calib.elf test_file.elf test_file_seek.elf test_i2c.elf \
test_i2c-scan.elf test_imaths.elf test_lcd.elf \
test_lsm303-calib.elf test_mma-calib.elf test_morse.elf test_nav2.elf \
test_nav.elf test_sd2.elf test_show-ee.elf test_spt.elf \
test_spt_scan.elf test_syslog.elf test_thrust.elf test_tude.elf \
test_tude-log.elf test_uart_echo.elf test_write-ee.elf yield.elf \
host_rx_posn.host host_tx_posn.host
all: $(bins)
clean:
	rm -f $(bins) *.o
.PHONY: $(BINS)

test_uart_echo.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_uart_echo` -o $@
	avr-size $@

test_thrust.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_thrust` -o $@
	avr-size $@
test_mma-calib.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_mma-calib lsm303` -o $@
	avr-size $@

test_tude.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_tude lsm303-calib lsm303` -o $@
	avr-size $@
test_morse.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_morse` -o $@
	avr-size $@
test_nav.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_nav` -o $@
	avr-size $@
test_imaths.elf: #test_imaths.o imaths.o tx.o fmt.o time.o
	set -vx; $(CC) $(CFLAGS) `./deps .c test_imaths` -o $@
	avr-size $@
test_syslog.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_syslog` -o $@
	avr-size $@
test_division.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_syslog` -o $@
	avr-size $@
test_i2c.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_i2c` -o $@
	avr-size $@
test_lsm303-calib.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_lsm303-calib lsm303-calib lsm303` -o $@
	avr-size $@
test_show-ee.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_show-ee` -o $@
	avr-size $@
test_write-ee.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_write-ee` -o $@
	avr-size $@
test_ads1015.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_ads1015` -o $@
	avr-size $@
test_sd2.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_sd2` -o $@
	avr-size $@
test_i2c-scan.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_i2c-scan` -o $@
	avr-size $@
test_accel.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_accel` -o $@
	avr-size $@
test_cmpas.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_cmpas lsm303` -o $@
	avr-size $@
test_tude-log.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_tude-log lsm303` -o $@
	avr-size $@
test_ctd-calib.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c ctd-calib` -o $@
	avr-size $@
test_file.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_file` -o $@
	avr-size $@
test_file_seek.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_file_seek` -o $@
	avr-size $@
test_spt.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_spt` -o $@
	avr-size $@
test_spt_scan.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_spt_scan` -o $@
	avr-size $@
yield.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c yield bg2 fg_empty` -o $@
	avr-size $@
test_prng:test_prng.c prng.c
	gcc -O test_prng.c -o $@
test_lcd.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_lcd` -o $@
	avr-size $@;
tasks_tracker.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c tasks_tracker lsm303 bg2` -o $@
	avr-size $@;
EXTIMES_DEPS := imaths tx fmt
extimes.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c extimes` -o $@
	avr-size $@;
test_nav2.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c test_nav2` -o $@
	avr-size $@;
tasks_ctrl.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c tasks_ctrl bg1 fg1 lsm303` -o $@
	avr-size $@;
tasks_repl.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c tasks_repl bg1 fg1 lsm303` -o $@
	avr-size $@;
tasks_mission.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c tasks_mission bg1 fg1 lsm303` -o $@
	avr-size $@;
tasks_fg2.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c fg2 bg2 ` -o $@
	avr-size $@;
tasks_diptest.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c tasks_diptest bg1 fg1 lsm303` -o $@
	avr-size $@;
tasks_rftest.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c tasks_rftest bg2` -o $@
	avr-size $@;
tasks_pitch.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c fg3 bg1 lsm303` -o $@
	avr-size $@;
tasks_swim.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c fg_swim bg1 lsm303` -o $@
	avr-size $@;
tasks_swim2.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c fg_swim2 bg1 lsm303` -o $@
	avr-size $@;
tasks_thrust.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c fg_thrust bg1 lsm303` -o $@
	avr-size $@;
tasks_antennas.elf:
	set -vx; $(CC) $(CFLAGS) `./deps .c fg_antennas bg2` -o $@
	avr-size $@;

host_tx_posn.host: host_tx_posn.c pkt.c crc.c
	gcc -g -O -Wall host_tx_posn.c pkt.c crc.c -o $@
host_rx_posn.host: host_rx_posn.c pkt.c crc.c
	gcc -g -O -Wall host_rx_posn.c pkt.c crc.c -o $@

%.dep: %.c
	@$(CC) -MM $(CFLAGS) -E $<
%.list: %.c
	$(CC) -c $(CFLAGS) -E $< |less
%.o:%.h
%.o:%.c
	$(CC) -c $(CFLAGS) $*.c -o $@
%.elf:%.o
	$(CC) $(CFLAGS) $*.o -o $@
	avr-size $@
%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@
%.bin: %.hex
	$(OBJCOPY) -I ihex -O binary $< $@
%.program:%.hex
	avrdude $(DUDEFLAGS) -U flash:w:$<

# Copy fuses (if there is a .fuse section in the executable)
%.fuses: %.elf
	$(OBJCOPY) -j .fuse -O binary $*.elf $@
%.lfuse: %.fuses
	dd bs=1  count=1 if=$< of=$@
%.hfuse: %.fuses
	dd bs=1  count=1 skip=1 if=$< of=$@
%.efuse: %.fuses
	dd bs=1  count=1 skip=2 if=$< of=$@
%.putfuses: %.lfuse %.hfuse %.efuse
	avrdude $(DUDEFLAGS) -U hfuse:w:$*.hfuse:r -U lfuse:w:$*.lfuse:r -U efuse:w:$*.efuse:r
%.partno: %.mcu
	sed -e 's/atmega\([0123456789]*\)[a]/m\1/i' \
	    -e 's/attiny\([0123456789]*\)[a]/t\1/i' <$< >$@

%.efuse.dump:
	avrdude $(DUDEFLAGS) -U efuse:r:$@:r
%.hfuse.dump:
	avrdude $(DUDEFLAGS) -U hfuse:r:$@:r
%.lfuse.dump:
	avrdude $(DUDEFLAGS) -U lfuse:r:$@:r

0xff:
	dd if=/dev/zero bs=1024 count=1 |tr '\0' '\377' >$@
eeprom.erase: 0xff
	avrdude $(DUDEFLAGS) -U eeprom:w:$<:r
%.eeprom.dump:
	avrdude $(DUDEFLAGS) -U eeprom:r:$@:r
%.flash.dump:
	avrdude $(DUDEFLAGS) -U flash:r:$@:r

%.bitclean:
	rm -f $*.o $*.elf $*.hex core

.PRECIOUS:*.bitclean *.boardclean
%.clean:%.bitclean %.boardclean
	:

# For boards:
%.boardclean:
	rm -f $*.*.gbr $*.*.cnc $*.*.bak* $*.sch~ $*.pcb- $*.net \
	$*.bom $*.cmd *.'sch#' $*.xy $*.bom $*.new.pcb
%.project:
	echo schematics $*.sch >>$@
	echo output-name $* >>$@
	echo elements-dir $(HOME)/share/pcb/pcblib-newlib >>$@
%.pcb: %.sch %.project
	gsch2pcb --use-files $*.project

%.avi:%.AVI
	mencoder -o $@ -oac mp3lame -ovc lavc $<
