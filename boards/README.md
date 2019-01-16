
Wed 16 Jan 10:42:28 +08 2019

## Printed circuit boards

Baby AUV uses these PCBs.

Schematic capture and layout were done on gschem (*.sch files) and pcb
(*.pcb files) respectively, of the gEDA package.  The .tgz files
contains these, as well as the board-house-ready Gerbers.  AFAIK
these applications do not run on Windows.  The *.sch files are
NOT Eagle files.

For convenience, the *.sch.pdf (board schematics) files are also
included in the tar file.

The Makefile is only used to prepare the tgz files for upload.

### bat.{sch,pcb}
A small board to tie the 2 batteries in parallel; also
used for charging the batteries.

### encode.{sch,pcb}
This small board also doubles as the mounting for
the thruster motor.  The encoder uses an IR reflective sensor
pointed at the top surface of the shaft coupler, half of which
is coloured black with a permanent marker.  This marking must be
properly synced, so that as the prop blade rises, the encoder
outputs a rising edge, and when the blade falls, it outputs a
falling edge.

### mpx5700-pcb.{sch,pcb}
This carries the MPX5700 pressure sensor, and attaches to the
aft bulkhead.

### neo6m-hc12.{sch,pcb}
This board carries the GPS module and RF modem module, and mounts
to the foward bulkhead.  It also carries an I2C port expander to
multiplex the UARTs of both modules, as well as power them on
and off.  This board does not strictly conform to the pf-
interface.

### pf-ec.{sch,pcb}
The CTD board, which takes input from the MPX500 depth
sensor and elecrical conductivity probe.  The pf-* boards all conform
to the same IDC bus and mechanical interface.

### pf-mcu.{sch,pcb}
The MCU (ATmega328p) board.  This also carries the DC-DC
converter and servo interface (for the moving mass actuator).

### pf-thruster.{sch,pcb}
The thruster board.  This interfaces to the
thruster, which is a DC motor, and the encoder board.

### tracker-power.{sch,pcb}
This is only used in the RF tracker, not the
AUV.  It is a mounting for the battery socket, switch, and
charging board.
