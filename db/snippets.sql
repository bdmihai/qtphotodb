/* copy rows from Tags to Albums */
INSERT INTO Albums SELECT * FROM Tags WHERE Name LIKE '20%';

/* delete copied ros from Tags */
DELETE FROM Tags WHERE Name LIKE '20%';

/* reorder index */
UPDATE Tags SET Id = (SELECT COUNT(*) FROM Tags t WHERE t.Id <= Tags.Id);
UPDATE Albums SET Id = (SELECT COUNT(*) FROM Albums t WHERE t.Id <= Albums.Id);
