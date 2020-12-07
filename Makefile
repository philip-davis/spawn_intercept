all: libexspawn despooler spawn_done_signaller

spawn_done_signaller: spawn_done_signaller.c exspawn.o
	mpicc -o spawn_done_signaller spawn_done_signaller.c exspawn.o

despooler: despooler.c exspawn.o
	mpicc -o despooler despooler.c exspawn.o
	
libexspawn: exspawn.o
	mpicc -shared -o libexspawn.so exspawn.o

exspawn.o: exspawn.c
	mpicc -fPIC -c exspawn.c

clean:
	rm -f spawn_done_signaller despooler libexspawn.so *\.o
