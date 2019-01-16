v 20130925 2
C 40000 40000 0 0 0 title-A4.sym
C 42400 44300 1 0 0 tp4056-mod-1.sym
{
T 43300 44400 5 8 0 1 0 0 1
footprint=TP4056_MOD_1
T 44000 45600 5 10 0 0 0 0 1
device=TP4056-charging-module
T 43400 45300 5 10 1 1 0 6 1
refdes=U1
T 42200 44100 5 10 1 1 0 0 1
value=Micro USB charger module
}
N 44000 44500 44600 44500 4
{
T 44200 44500 5 10 1 1 0 0 1
netname=bat-
}
N 44000 44800 44600 44800 4
{
T 44200 44800 5 10 1 1 0 0 1
netname=bat+
}
N 42900 45900 43800 45900 4
{
T 43400 45900 5 10 1 1 0 0 1
netname=bat+
}
C 45400 44700 1 0 0 header2-1.sym
{
T 46400 45350 5 10 0 0 0 0 1
device=HEADER2
T 45800 46000 5 10 1 1 0 0 1
refdes=J1
T 45400 44700 5 10 0 0 0 0 1
footprint=HEADER2_1
T 45300 44800 5 10 1 1 0 0 1
value=MCU board power header
}
N 44800 45300 45400 45300 4
{
T 45000 45300 5 10 1 1 0 0 1
netname=bat-
}
C 45400 43100 1 0 0 header2-1.sym
{
T 46400 43750 5 10 0 0 0 0 1
device=HEADER2
T 45400 43100 5 10 0 0 0 6 1
footprint=JST_XH_2
T 45800 44400 5 10 1 1 0 0 1
refdes=J2
T 44700 43000 5 10 1 1 0 0 1
value=18650 holder JST connector
}
N 45400 44100 44900 44100 4
{
T 45400 44100 5 10 1 1 0 6 1
netname=bat+
}
N 45400 43700 44900 43700 4
{
T 45400 43700 5 10 1 1 0 6 1
netname=bat-
}
T 45100 43200 9 10 1 0 0 0 1
Battery connector
T 46200 45400 9 10 1 0 0 0 1
Power to MCU board
C 42000 45900 1 0 0 switch-spdt-slide-1.sym
{
T 42400 46700 5 10 0 0 0 0 1
device=SPDT
T 42000 45900 5 10 0 1 0 0 1
footprint=SWITCH_SPDT_RA_OS102011MA1QN1
T 42300 46700 5 10 1 1 0 0 1
refdes=S1
T 42000 45900 5 10 1 1 0 0 1
value=Tracker on-off switch
}
N 42900 46300 44000 46300 4
{
T 42900 46300 5 10 1 1 0 0 1
netname=bat_out
}
N 44800 45700 45400 45700 4
{
T 44800 45700 5 10 1 1 0 0 1
netname=bat_out
}
T 45000 40400 9 10 1 0 0 0 1
tracker-power.sch
T 44700 41100 9 10 1 0 0 0 1
Tracker power on-off switch and charger for 18650
