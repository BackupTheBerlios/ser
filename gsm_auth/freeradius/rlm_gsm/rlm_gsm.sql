
-- SQL script for RLM_GSM (FreeRADIUS) module

DROP DATABASE IF EXISTS gsm_auth;

-- create th database
CREATE DATABASE gsm_auth;

USE gsm_auth;

-- create the table
CREATE TABLE gsm_user(
      -- IMSI
    imsi VARCHAR(32) NOT NULL PRIMARY KEY,
      -- REALM
    realm VARCHAR(64) NOT NULL DEFAULT "localhost",
      -- GSM challenge
    challenge VARCHAR(64) NOT NULL,
      -- GSM response
    response VARCHAR(64) NOT NULL,
      -- GSM session key
    session_key VARCHAR(64) NOT NULL,
      -- expiration time
    access_time DATETIME NOT NULL DEFAULT 0
);

INSERT INTO gsm_user (imsi, realm, challenge, response, \
	session_key, access_time) \
	VALUES ('232033006758425', 'gorn.fokus.fraunhofer.de', \
	'30000000000000000000000000000000', '1EF8EED7', \
	'AB9CC857FC15E400', NOW());

