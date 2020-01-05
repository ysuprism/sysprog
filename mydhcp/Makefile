#Makefile

all: mydhcpd mydhcpc

mydhcpd: mydhcpd.c mydhcpd_utils.c mydhcpd.h mydhcp.h
	gcc -o mydhcpd mydhcpd.c mydhcpd_utils.c mydhcpd.h mydhcp.h

mydhcpc: mydhcpc.c mydhcp.h
	gcc -o mydhcpc mydhcpc.c mydhcp.h

clean:
	\rm mydhcpd mydhcpc
