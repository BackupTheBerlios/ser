DROP DATABASE ser;
REVOKE ALL PRIVILEGES ON ser.* FROM 'ser'@'%';
DROP USER 'ser'@'%';
REVOKE ALL PRIVILEGES ON ser.* FROM 'ser'@'localhost';
DROP USER 'ser'@'localhost';
FLUSH PRIVILEGES;
REVOKE ALL PRIVILEGES ON ser.* FROM 'serro'@'%';
DROP USER 'serro'@'%';
REVOKE ALL PRIVILEGES ON ser.* FROM 'serro'@'localhost';
DROP USER 'serro'@'localhost';
FLUSH PRIVILEGES;
