/*

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
                                       
   $Id: lock_file.c,v 1.1 2002/09/05 21:39:29 jku Exp $
*/
                                          

#define LOCK_PREFIX "/var/lock/LCK.."
void unlock_port();

int check_lock(char *lck_file,pid_t *pid) {
        char pid_str[12];
        int lck_fd,n;

	memset(pid_str, '\0', 12);

	lck_fd = open(lck_file, O_RDONLY, 0);	/* verificar se existe um lockfile sem uso */
	if(!lck_fd) {
		return(0);
	}

	n = read(lck_fd, &pid_str, 11);		/* ler o pid do ficheiro */
	close(lck_fd);
	if(n < 0) {   
		return(0);
	}

	sscanf(pid_str, " %d", pid);   /* verificar se processo ainda existe */
	if(pid == 0 || (kill(*pid, 0) == -1 && errno == ESRCH)) {
        	return(0);			/* nao existe */
        } else {
        	return(1);			/* existe */
        }
}

int lock_port() {
	int lck_fd,n;
	char lck_file[1024];
	pid_t pid;
	char pid_str[12];

	memset(pid_str, '\0', 12);
       	sprintf(lck_file, "%s%s", LOCK_PREFIX, serial_port);
	while(1) {
		if ((lck_fd = open(lck_file, O_RDWR | O_EXCL | O_CREAT, 0644))<0) {
			if(errno == EEXIST) {
				if (check_lock(lck_file,&pid)) {
#ifdef DEBUG
					fprintf(stderr, "%s : Port em uso pid -> %d\n", serial_port, pid);
#endif DEBUG
					exit(1);
				}
				else {
					unlink(lck_file);
#ifdef DEBUG
					fprintf(stderr, "%s : Lock sem uso removido pid -> %d\n",serial_port, pid);
#endif DEBUG
					continue;
				}
			}
			else {
#ifdef DEBUG
				fprintf(stderr, "%s : erro fazendo lock ao port\n", serial_port);
#endif DEBUG
				return(1);
			}
		}
		
		pid = getpid();		/* get pid do processo e escrever no lock file */
		sprintf(pid_str, "%010d\n", pid);
		write(lck_fd, pid_str, 11);

		close(lck_fd);
		return(0);
	}
}
 
void unlock_port() {
	char lck_file[255];
	
	sprintf(lck_file, "%s%s", LOCK_PREFIX, serial_port);
	unlink(lck_file);
}
