CREATE DATABASE ser;
USE ser;

CREATE TABLE version (
    table_name VARCHAR(32) NOT NULL,
    table_version INT UNSIGNED NOT NULL DEFAULT '0'
);

INSERT INTO version (table_name, table_version) VALUES ('acc', '3');
INSERT INTO version (table_name, table_version) VALUES ('missed_calls', '3');
INSERT INTO version (table_name, table_version) VALUES ('location', '8');
INSERT INTO version (table_name, table_version) VALUES ('credentials', '6');
INSERT INTO version (table_name, table_version) VALUES ('domain', '2');
INSERT INTO version (table_name, table_version) VALUES ('attr_types', '1');
INSERT INTO version (table_name, table_version) VALUES ('global_attrs', '1');
INSERT INTO version (table_name, table_version) VALUES ('domain_attrs', '1');
INSERT INTO version (table_name, table_version) VALUES ('user_attrs', '3');
INSERT INTO version (table_name, table_version) VALUES ('phonebook', '1');
INSERT INTO version (table_name, table_version) VALUES ('silo', '3');
INSERT INTO version (table_name, table_version) VALUES ('uri', '2');
INSERT INTO version (table_name, table_version) VALUES ('server_monitoring', '1');
INSERT INTO version (table_name, table_version) VALUES ('trusted', '1');
INSERT INTO version (table_name, table_version) VALUES ('server_monitoring_agg', '1');
INSERT INTO version (table_name, table_version) VALUES ('speed_dial', '2');
INSERT INTO version (table_name, table_version) VALUES ('sd_attrs', '1');
INSERT INTO version (table_name, table_version) VALUES ('gw', '2');
INSERT INTO version (table_name, table_version) VALUES ('gw_grp', '2');
INSERT INTO version (table_name, table_version) VALUES ('lcr', '1');

CREATE TABLE acc (
    id INT AUTO_INCREMENT NOT NULL,
    from_uid VARCHAR(64),
    to_uid VARCHAR(64),
    to_did VARCHAR(64),
    from_did VARCHAR(64),
    sip_from VARCHAR(255),
    sip_to VARCHAR(255),
    sip_status VARCHAR(128),
    sip_method VARCHAR(16),
    in_ruri VARCHAR(255),
    out_ruri VARCHAR(255),
    from_uri VARCHAR(255),
    to_uri VARCHAR(255),
    sip_callid VARCHAR(255),
    sip_cseq INT,
    digest_username VARCHAR(64),
    digest_realm VARCHAR(255),
    from_tag VARCHAR(128),
    to_tag VARCHAR(128),
    src_ip INT UNSIGNED,
    src_port SMALLINT UNSIGNED,
    request_timestamp DATETIME NOT NULL,
    response_timestamp DATETIME NOT NULL,
    flags INT UNSIGNED NOT NULL DEFAULT '0',
    attrs VARCHAR(255),
    UNIQUE KEY id_key (id),
    KEY cid_key (sip_callid)
);

CREATE TABLE missed_calls (
    id INT AUTO_INCREMENT NOT NULL,
    from_uid VARCHAR(64),
    to_uid VARCHAR(64),
    to_did VARCHAR(64),
    from_did VARCHAR(64),
    sip_from VARCHAR(255),
    sip_to VARCHAR(255),
    sip_status VARCHAR(128),
    sip_method VARCHAR(16),
    inbound_ruri VARCHAR(255),
    outbound_ruri VARCHAR(255),
    from_uri VARCHAR(255),
    to_uri VARCHAR(255),
    sip_callid VARCHAR(255),
    sip_cseq INT,
    digest_username VARCHAR(64),
    digest_realm VARCHAR(255),
    from_tag VARCHAR(128),
    to_tag VARCHAR(128),
    request_timestamp DATETIME NOT NULL,
    response_timestamp DATETIME NOT NULL,
    flags INT UNSIGNED NOT NULL DEFAULT '0',
    attrs VARCHAR(255),
    UNIQUE KEY id_key (id),
    KEY cid_key (sip_callid)
);

CREATE TABLE credentials (
    auth_username VARCHAR(64) NOT NULL,
    realm VARCHAR(64) NOT NULL,
    password VARCHAR(28) NOT NULL DEFAULT '',
    flags INT NOT NULL DEFAULT '0',
    ha1 VARCHAR(32) NOT NULL,
    ha1b VARCHAR(32) NOT NULL DEFAULT '',
    uid VARCHAR(64) NOT NULL,
    UNIQUE KEY (auth_username, realm),
    KEY uid (uid)
);

CREATE TABLE attr_types (
    name VARCHAR(32) NOT NULL,
    rich_type VARCHAR(32) NOT NULL DEFAULT 'string',
    raw_type INT NOT NULL DEFAULT '2',
    type_spec VARCHAR(255),
    KEY upt_idx1 (name)
);

CREATE TABLE global_attrs (
    name VARCHAR(32) NOT NULL,
    type INT NOT NULL DEFAULT '0',
    value VARCHAR(64),
    flags INT UNSIGNED NOT NULL DEFAULT '0',
    UNIQUE KEY global_attrs_idx (name, value)
);

CREATE TABLE domain_attrs (
    did VARCHAR(64),
    name VARCHAR(32) NOT NULL,
    type INT NOT NULL DEFAULT '0',
    value VARCHAR(64),
    flags INT UNSIGNED NOT NULL DEFAULT '0',
    UNIQUE KEY domain_attr_idx (did, name, value),
    KEY domain_did (did, flags)
);

CREATE TABLE user_attrs (
    uid VARCHAR(64) NOT NULL,
    name VARCHAR(32) NOT NULL,
    value VARCHAR(64),
    type INT NOT NULL DEFAULT '0',
    flags INT UNSIGNED NOT NULL DEFAULT '0',
    UNIQUE KEY userattrs_idx (uid, name, value)
);

CREATE TABLE domain (
    did VARCHAR(64) NOT NULL,
    domain VARCHAR(128) NOT NULL,
    last_modified timestamp NOT NULL,
    flags INT UNSIGNED NOT NULL DEFAULT '0',
    UNIQUE KEY domain_idx (did, domain)
);

CREATE TABLE location (
    uid VARCHAR(64) NOT NULL,
    contact VARCHAR(255) NOT NULL,
    received VARCHAR(255),
    expires DATETIME NOT NULL DEFAULT '1970-01-01 00:00:00',
    q FLOAT NOT NULL DEFAULT '1.0',
    callid VARCHAR(255),
    cseq INT UNSIGNED,
    flags INT UNSIGNED NOT NULL DEFAULT '0',
    user_agent VARCHAR(64),
    UNIQUE KEY location_key (uid, contact),
    KEY location_contact (contact)
);

CREATE TABLE trusted (
    src_ip VARCHAR(39) NOT NULL,
    proto VARCHAR(4) NOT NULL,
    from_pattern VARCHAR(64) NOT NULL,
    UNIQUE KEY trusted_idx (src_ip, proto, from_pattern)
);

CREATE TABLE server_monitoring (
    time DATETIME NOT NULL DEFAULT '1970-01-01 00:00:00',
    id INT NOT NULL DEFAULT '0',
    param VARCHAR(32) NOT NULL DEFAULT '',
    value INT NOT NULL DEFAULT '0',
    increment INT NOT NULL DEFAULT '0',
    KEY sm_idx1 (id, param)
);

CREATE TABLE server_monitoring_agg (
    param VARCHAR(32) NOT NULL DEFAULT '',
    s_value INT NOT NULL DEFAULT '0',
    s_increment INT NOT NULL DEFAULT '0',
    last_aggregated_increment INT NOT NULL DEFAULT '0',
    av DOUBLE NOT NULL DEFAULT '0',
    mv INT NOT NULL DEFAULT '0',
    ad DOUBLE NOT NULL DEFAULT '0',
    lv INT NOT NULL DEFAULT '0',
    min_val INT NOT NULL DEFAULT '0',
    max_val INT NOT NULL DEFAULT '0',
    min_inc INT NOT NULL DEFAULT '0',
    max_inc INT NOT NULL DEFAULT '0',
    lastupdate DATETIME NOT NULL DEFAULT '1970-01-01 00:00:00',
    KEY smagg_idx1 (param)
);

CREATE TABLE phonebook (
    id INT AUTO_INCREMENT NOT NULL,
    uid VARCHAR(64) NOT NULL,
    fname VARCHAR(32),
    lname VARCHAR(32),
    sip_uri VARCHAR(255) NOT NULL,
    UNIQUE KEY pb_idx (id),
    KEY pb_uid (uid)
);

CREATE TABLE gw (
    gw_name VARCHAR(128) NOT NULL,
    ip_addr INT UNSIGNED NOT NULL,
    port SMALLINT UNSIGNED,
    uri_scheme TINYINT UNSIGNED,
    transport SMALLINT UNSIGNED,
    grp_id INT NOT NULL,
    UNIQUE KEY gw_idx1 (gw_name),
    KEY gw_idx2 (grp_id)
);

CREATE TABLE gw_grp (
    grp_id INT AUTO_INCREMENT NOT NULL,
    grp_name VARCHAR(64) NOT NULL,
    UNIQUE KEY gwgrp_idx (grp_id)
);

CREATE TABLE lcr (
    prefix VARCHAR(16) NOT NULL,
    from_uri VARCHAR(255) NOT NULL DEFAULT '%',
    grp_id INT,
    priority INT,
    KEY lcr_idx1 (prefix),
    KEY lcr_idx2 (from_uri),
    KEY lcr_idx3 (grp_id)
);

CREATE TABLE silo (
    mid INT AUTO_INCREMENT NOT NULL,
    src_addr VARCHAR(255) NOT NULL,
    dst_addr VARCHAR(255) NOT NULL,
    r_uri VARCHAR(255) NOT NULL,
    uid VARCHAR(64) NOT NULL,
    inc_time DATETIME NOT NULL DEFAULT '1970-01-01 00:00:00',
    exp_time DATETIME NOT NULL DEFAULT '1970-01-01 00:00:00',
    ctype VARCHAR(128) NOT NULL DEFAULT 'text/plain',
    body BLOB NOT NULL DEFAULT '',
    UNIQUE KEY silo_idx1 (mid)
);

CREATE TABLE uri (
    uid VARCHAR(64) NOT NULL,
    did VARCHAR(64) NOT NULL,
    username VARCHAR(64) NOT NULL,
    flags INT UNSIGNED NOT NULL DEFAULT '0',
    UNIQUE KEY uri_idx1 (username, did, flags),
    UNIQUE KEY uri_uid (uid, flags)
);

CREATE TABLE speed_dial (
    id INT AUTO_INCREMENT NOT NULL,
    uid VARCHAR(64) NOT NULL,
    dial_username VARCHAR(64) NOT NULL,
    dial_did VARCHAR(64) NOT NULL,
    new_uri VARCHAR(255) NOT NULL,
    UNIQUE KEY speeddial_idx1 (uid, dial_did, dial_username),
    UNIQUE KEY speeddial_id (id),
    KEY speeddial_uid (uid)
);

CREATE TABLE sd_attrs (
    id VARCHAR(64) NOT NULL,
    name VARCHAR(32) NOT NULL,
    value VARCHAR(64),
    type INT NOT NULL DEFAULT '0',
    flags INT UNSIGNED NOT NULL DEFAULT '0',
    UNIQUE KEY userattrs_idx (id, name, value)
);

GRANT ALL ON ser.* TO 'ser'@'%' IDENTIFIED BY 'heslo';
GRANT ALL ON ser.* TO 'ser'@'localhost' IDENTIFIED BY 'heslo';
FLUSH PRIVILEGES;
GRANT SELECT ON ser.* TO 'serro'@'%' IDENTIFIED BY '47serro11';
GRANT SELECT ON ser.* TO 'serro'@'localhost' IDENTIFIED BY '47serro11';
FLUSH PRIVILEGES;
