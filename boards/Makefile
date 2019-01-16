# For compiling: 
CC = avr-gcc
OBJCOPY = avr-objcopy
TTY = /dev/ttyUSB0
#CFLAGS= -DCOMPILED="\"`date +'Build %H:%M:%S %d/%m/%Y'`\"" -std=c99 -O -g -Wall
CFLAGS= -std=c99 -DF_CPU=16000000UL -O -g -Wall -mmcu=`[ -f MCU ] && cat MCU || cat $*.mcu`
#DUDEFLAGS=-v -v -F -p `[ -f PARTNO ] && cat PARTNO || cat $*.partno` -cusbasp -F
DUDEFLAGS=-v -v -F -p `[ -f PARTNO ] && cat PARTNO || cat $*.partno` -carduino -P $(TTY) -b 57600

boards= bat encode pf-thruster pf-mcu neo6m-hc12 pf-ec mpx5700-pcb \
tracker-power
all: $(foreach b, $(boards), $(b).tgz) sym

boardclean: $(foreach b, $(boards), $(b).boardclean)
.PHONY: sym
sym:
	mkdir -p sym
	set -vxe; for f in `sed -n 's/.* \([^ ]*\.sym\)$$/\1/p' *.sch |sort -u`; do \
	  e=`find ~/share/gEDA/sym -name $$f`; \
	  [ -z "$$e" ] && e=`find /usr/share/gEDA/sym -name $$f`; \
	  cp $$e sym/; done
	chmod 644 sym/*

gerbers=bottom.gbr bottommask.gbr bottomsilk.gbr outline.gbr \
top.gbr topmask.gbr topsilk.gbr fab.gbr plated-drill.cnc xy bom

%.sch.pdf:%.ps
	ps2pdf $< $@
%.sch.ps:%.sch
	/bin/false $<
%.tgz: %.sch.pdf $(foreach g, $(gerbers), %.$(g))
	tar zcvf $@ $^
$(foreach g, $(gerbers), %.$(g)): %.pcb
	/bin/false $<

%.list: %.c
	$(CC) -c $(CFLAGS) -E $< |less
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
	$*.bom $*.cmd *.'sch#' $*.xy $*.bom $*.new.pcb *.sch.ps *.ps \
	*.sch.pdf
%.project:
	echo schematics $*.sch >>$@
	echo output-name $* >>$@
	echo elements-dir $(HOME)/share/pcb/pcblib-newlib >>$@
%.pcb: %.sch %.project
	gsch2pcb --use-files $*.project

%.avi:%.AVI
	mencoder -o $@ -oac mp3lame -ovc lavc $<
