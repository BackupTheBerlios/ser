/*
   $Id: serial.c,v 1.2 2002/09/19 12:23:53 jku Exp $

   wx2000 -- Weather Data Logger Extration Program (ws2000 model)

   Copyright (C) 2000 Friedrich Zabel

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 */


struct termios optionsold; /* old serial port settings */

void signal_handler_IO (int status) {
#ifdef DEBUG
	printf("received SIGIO signal.\n");
#endif DEBUG
	wait_flag = FALSE;
}

int open_port(void) {
	int fd;
	char serial_device[255];
	
	lock_port(); /* check & create lock file */

	sprintf(serial_device, "/dev/%s", serial_port);
	fd = open( serial_device, O_RDWR | O_NONBLOCK | O_NOCTTY | O_NDELAY);
	if (fd == -1)	{
		printf("Error - could not open port.\n");
	}
	return fd;
}

int setserial(int fd) {

	struct termios options;
	struct sigaction saio;

	saio.sa_handler = signal_handler_IO;
      	saio.sa_flags = 0;
      	saio.sa_restorer = NULL;
      	sigaction(SIGIO,&saio,NULL);
      
	tcgetattr(fd, &optionsold);
	tcgetattr(fd, &options);

	fcntl(fd, F_SETOWN, getpid());	/* allow the process to receive SIGIO */
	fcntl(fd, F_SETFL, FASYNC);
	
	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);

	options.c_cflag = PARENB | CLOCAL | CREAD | CS8 | CSTOPB;
	
	options.c_oflag = 0;

	options.c_iflag = BRKINT | IGNPAR;

	options.c_lflag = PENDIN;

	options.c_cc[VMIN]=0;
	options.c_cc[VTIME]=1;

	tcflush(fd, TCIFLUSH);
	
	tcsetattr(fd, TCSANOW, &options);
}

void set_dtr(int fd) {

	int temp;

	ioctl(fd,TIOCMGET, &temp);
	
	temp = temp | TIOCM_DTR;
	
	ioctl(fd, TIOCMSET, &temp);
}

void lower_dtr(int fd) {

	int temp;

	ioctl(fd,TIOCMGET, &temp);
	
	temp = temp & ~TIOCM_DTR;
	
	ioctl(fd, TIOCMSET, &temp);
}
