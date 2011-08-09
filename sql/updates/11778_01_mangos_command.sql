ALTER TABLE db_version CHANGE COLUMN required_11774_01_mangos_spell_proc_event required_11778_01_mangos_command bit;

DELETE FROM command WHERE name IN ('debug movestate');

INSERT INTO command (name, security, help) VALUES
('debug movestate',2,'Syntax: .debug movestate\r\n\r\nDisplay a list of details for the selected unit\'s movement.');