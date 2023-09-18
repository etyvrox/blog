### MySQL EXPLAIN statement data exfiltration
MySQLsplaining - to explain something to a developer in a condescending way that assumes he has no knowledge about the topic. 

Jokes aside MySQL EXPLAIN statement **provides information about how MySQL executes statements**: EXPLAIN works with SELECT , DELETE , INSERT , REPLACE , and UPDATE statements. When EXPLAIN is used with an explainable statement, MySQL displays information from the optimizer about the statement execution plan. (https://dev.mysql.com/doc/refman/8.0/en/using-explain.html)

This statement might be used in some monitoring systems like Percona for example (https://www.percona.com/software/database-tools/percona-monitoring-and-management)

Let's suppose that somehow you gained access to functionality that allows you to query SQL but wraps your queries inside EXPLAIN statement - what can you exfiltrate with that?

First of all you can extract results of built-in functions via ERROR based SQL queries, e.g.:
 - username (MySQL 5)
 - MySQL version
 - hostname 
 - database

``` mysql
EXPLAIN SELECT EXTRACTVALUE(0x0a,CONCAT(0x0a,(SELECT VERSION())));
```
![Pasted image 20230915205402](https://github.com/etyvrox/blog/assets/7671966/b8f13af5-eaae-468a-9c75-3271a087dafd)
``` mysql
EXPLAIN SELECT EXTRACTVALUE(0x0a,CONCAT(0x0a,(SELECT DATABASE())));
```
![Pasted image 20230915205546](https://github.com/etyvrox/blog/assets/7671966/af211053-80b5-4022-bace-459ea25617a6)


Then if you try to extract any information not related to indexes you will get the following output:
Trying to generate error on getting table names
``` mysql
SELECT EXTRACTVALUE(0x0a, CONCAT(0x28, 0x7e, (SELECT GROUP_CONCAT(TABLE_NAME SEPARATOR ',') AS conc FROM information_schema.tables WHERE table_schema=DATABASE())));
```
![Pasted image 20230916162022](https://github.com/etyvrox/blog/assets/7671966/8b4b6c88-791d-4cca-86ce-c3be71d7ff4a)


As you can see no sensitive info is leaked when we apply to **EXPLAIN** statement
So from this point what can we do?

Let's first of all dig a little deeper into what EXPLAIN statement is indeed.
EXPLAIN helps to understand how MySQL executes given query. Now we are going to inspect examples of EXPLAIN on this sample table foo:
![Pasted image 20230918204603](https://github.com/etyvrox/blog/assets/7671966/f9fd938f-e1e7-4254-87d4-3bb0ef1f233d)

let's ask MySQL to explain the same command above
``` MySQL
EXPLAIN SELECT * FROM foo;
```
![Pasted image 20230918204808](https://github.com/etyvrox/blog/assets/7671966/7ee2489f-2f7f-4ccb-a086-abd1b4342ee1)
"Rows" column shows us how many columns MySQL had to examine to give us the result of executing the query.

We see that "key" and "key_len" columns returned NULL. This is because we don't yet have index in our table so let's create one with command:
```MySQL
CREATE INDEX id ON foo (id);
```
Before we continue let's quickly learn what index in general terms is:

Indexes are specialized lookup tables utilized by the database search engine to enhance data retrieval speed. They are not visible to users but serve the sole purpose of expediting database access.

So now EXPLAIN returns index name in "key" column after index creation
![Pasted image 20230918211432](https://github.com/etyvrox/blog/assets/7671966/7f9545af-6bd1-49fe-8af3-e28d8a311cd4)
Okay, we've specified three most valuable for us columns in terms of exfiltrating data with EXPLAIN statement. And now you can ask me why not just exfiltrate data with usual MySQL queries? The answer is - EXPLAIN won't show you any info regarding your queries even if they are error based (and don't include built-in functions).
![Pasted image 20230918212801](https://github.com/etyvrox/blog/assets/7671966/e0d460d3-f603-4e84-a796-0ea110b7d057)

As you can see executing queries on not indexed columns shows no difference. Now let's try filtering indexed column:
![Pasted image 20230918213427](https://github.com/etyvrox/blog/assets/7671966/fe828703-98e3-4dfd-b571-48632658670b)


As you see we can assert that data exists for sure only when there are 2 or more rows matching the query. In cases when data matching our query doesn't exist we get 1 row as output.
- No match - 1 row
- 1 match - 1 row
- More than 1 match  => 2 rows
So if we decompose and assume that we have something like unique data in our indexed column - we can bruteforce existing data until there are at least 2 rows matching our query:

![Pasted image 20230918220655](https://github.com/etyvrox/blog/assets/7671966/68dd2468-4885-4db5-80b2-3b364b2db48a)

To determine which type of data is used in column (id or varchar):
- EXPLAIN SELECT * FROM foo WHERE email < 0 (returns 5 rows meaning not right type of data)
- EXPLAIN SELECT * FROM foo WHERE email LIKE 'A%';

You should give attention to highlighted columns:

![Pasted image 20230918230155](https://github.com/etyvrox/blog/assets/7671966/0af7fbbb-9315-4041-8b2a-c53b6c431717)

Unfortunately I didn't manage to learn how to extract length of rows with EXPLAIN 
![Pasted image 20230918220609](https://github.com/etyvrox/blog/assets/7671966/be090e6a-6273-4848-886f-4d62437e1846)

If you want to dive deep into understanding what MySQL EXPLAIN is:
- https://www.linkedin.com/pulse/mysql-explain-explained-gowrav-vishwakarma-%E0%A4%97-%E0%A4%B0%E0%A4%B5-%E0%A4%B5-%E0%A4%B6-%E0%A4%B5%E0%A4%95%E0%A4%B0-%E0%A4%AE-/
- https://medium.com/datadenys/using-explain-in-mysql-to-analyze-and-improve-query-performance-f58357deb2aa

