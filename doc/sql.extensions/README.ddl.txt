DDL enhancements in Firebird v2.
--------------------------------

Author: Claudio Valderrama C. <cvalde at usa.net>

Other DDL enhancements may have their own README file.

1) Ability to signal SQL NULL via a NULL pointer.
(Claudio Valderrama C.)

Previous to Firebird v2, UDF authors only could guess they got a null value,
but they couldn't be sure, so this let to several problems with UDFs. People
ended up assuming that a null string would be passed as an empty string, a null
numeric would be the same than zero and a null date would mean the base date used
by the engine. Of course, for a numeric value, the author only could assume null
if the UDF was done for an environments where it's known that value is not possible
normally. But several UDFs (including ib_udf supplied with FB) assumed an empty
string most likely would mean a null parameter than a string of length zero. The
trick may work with CHAR type (the minimal declared CHAR length is one and would
contain a blank character normally, so a binary zero in the first position would
signal effectively NULL) but it doesn't work with VARCHAR or CSTRING, where length
zero is valid. The other solution was to rely on raw descriptors, but this draws
people to an area with a lot of things to check, more than they would want to tackle.
The biggest problem is that the engine won't obey the declared type for a parameter;
it will simply send whatever data it has for that parameter, so the UDF is left to
decide whether to reject or to try to convert the parameter to the expected data
type. Since UDFs don't have a formal mechanism to signal errors, the returned value
will have to be used as an indicator.
But the basic problem was how to keep the simplicity of the typical declarations
(no descriptors) while at the same time being able to signal null. The engine
normally passed UDF parameters by reference and it means in practical terms to
pass a pointer to the data. By simply passing a null pointer we can tell the UDF
we have SQL NULL, but since we can't afford to crash an unknown number of different
public and private UDFs in use that don't expect NULL, we had to enhance the syntax
to be able to request explicitly NULL handling. Therefore, only UDFs that are able
to deal with the new scenario can request SQL NULL signaling.
To avoid adding more keywords, the NULL keyword is appended to the UDF parameter
type and this is all the required change. Example:
declare external function sample int null returns int by value...;

If you are already using functions from ib_udf and want to take advantage of
null signaling (and null recognition) in some functions, you should connect to
your desired database and run
upgrade/v2/ib_udf_upgrade.sql
and commit afterwards, preferable when no more users are connected to the database.
The code in the listed functions in that script has been modified to recognize
null only when NULL is signaled by the engine. Therefore, starting with FB v2,
rtrim and ltrim no longer assume that an empty string means a NULL string. If you
don't upgrade, the functions won't crash. They simply won't be able to detect NULL.
If you never have used ib_udf and want to do that, you should connect to your desired
database, run
udf/ib_udf2.sql
and commit afterwards, preferable when no more users are connected to the database.
Note the "2" at the end of the name. The original script for FB v1.5 is still
available in the same directory.
The directories "upgrade" and "udf" are inside the home directory of your FB v2
installation.


2) Implemented REVOKE ADMIN OPTION FROM user
(Dmitry Yemanov.)

SYSDBA, the database creator or the owner of an object can grant rights on that
object to other users. However, those rights can be made inheritable, too. By using
WITH GRANT OPTION, the grantor gives the grantee the right to become a grantor of
the same rights in turn. This ability can be removed by the original grantor with
REVOKE GRANT OPTION FROM user.

However, there's a second form that involves roles. Instead of specifying the
same rights for many users (soon it becomes a maintenance nightmare) you can
create a role, assign rights to that role and then grant it to a group of users.
By simply changing the role's rights you affect all those users. By using
WITH ADMIN OPTION, the grantor (typically the role creator) gives the grantee the
right to become a grantor of the same role in turn. Until FB v2, this ability
couldn't be removed unless the original grantor fiddles with system tables directly.
Now, the ability to grant the role can be removed by the original grantor with
REVOKE ADMIN OPTION FROM user.


3) Blob filter's blob types can be declared by mnemonics for known types.
(Alex Peshkov.)

The original allowed syntax for declaring a blob filter was:
declare filter <name> input_type <number> output_type <number>
 entry_point <function_in_library> module_name <library_name>;

The alternative new syntax is:
declare filter <name> input_type <mnemonic> output_type <mnemonic>
 entry_point <function_in_library> module_name <library_name>;

where <mnemonic> refers to a subtype known to the engine. Initially they are
binary, text and others mostly of internal usage, but if the user is enough
brave, having written a new name in rdb$types, that name could be used, since
it's parsed only at declaration time. The engine keeps the numerical value.
Remember, only negative subtype values are meant to be defined by users. To get
the predefined types, do

select * from rdb$types where rdb$field_name = 'RDB$FIELD_SUB_TYPE';

RDB$FIELD_NAME      RDB$TYPE RDB$TYPE_NAME              RDB$DESCRIPTION RDB$SYSTEM_FLAG
=================== ======== ========================== =============== ===============

RDB$FIELD_SUB_TYPE         0 BINARY                          <null>                   1
RDB$FIELD_SUB_TYPE         1 TEXT                            <null>                   1
RDB$FIELD_SUB_TYPE         2 BLR                             <null>                   1
RDB$FIELD_SUB_TYPE         3 ACL                             <null>                   1
RDB$FIELD_SUB_TYPE         4 RANGES                          <null>                   1
RDB$FIELD_SUB_TYPE         5 SUMMARY                         <null>                   1
RDB$FIELD_SUB_TYPE         6 FORMAT                          <null>                   1
RDB$FIELD_SUB_TYPE         7 TRANSACTION_DESCRIPTION         <null>                   1
RDB$FIELD_SUB_TYPE         8 EXTERNAL_FILE_DESCRIPTION       <null>                   1

Examples.

Original declaration:
declare filter pesh input_type 0 output_type 3 entry_point 'f' module_name 'p';
Alternative declaration:
declare filter pesh input_type binary output_type acl entry_point 'f' module_name 'p';

Bizarre declaration for user defined blob subtype. Remember to commit after the insertion:
SQL> insert into rdb$types values('RDB$FIELD_SUB_TYPE', -100, 'XDR', 'test type', 0);
SQL> commit;
SQL> declare filter pesh2 input_type xdr output_type text entry_point 'p2' module_name 'p';
SQL> show filter pesh2;
BLOB Filter: PESH2
        Input subtype: -100 Output subtype: 1
        Filter library is p
        Entry point is p2


4) Allow comments in database objects.
(Claudio Valderrama C.)

Proposed syntax for testing:

COMMENT ON DATABASE IS {'txt'|NULL};
COMMENT ON <basic_type> name IS {'txt'|NULL};
COMMENT ON COLUMN table_or_view_name.field_name IS {'txt'|NULL};
COMMENT ON {PROCEDURE | [EXTERNAL] FUNCTION} [<package_name> .] name.param_name IS {'txt'|NULL};
COMMENT ON [PROCEDURE | FUNCTION] PARAMETER [<package_name> .] name.param_name IS {'txt'|NULL};

An empty literal string '' will act as NULL since the internal code (DYN in this case)
works this way with blobs.

basic_type:
- DOMAIN
- TABLE
- VIEW
- TRIGGER
- FILTER
- EXCEPTION
- GENERATOR
- SEQUENCE
- INDEX
- ROLE
- CHARACTER SET
- COLLATION
- PACKAGE
- USER (ability to store comment depends upon user management plugin)
- SECURITY CLASS (not implemented because Borland hid them).
- [GLOBAL] MAPPING


5) Allow setting and dropping default values from table fields.
(Claudio Valderrama C.)

Domains allow to change or drop their default. It seems natural that table fields
can be manipulated the same way without going directly to the system tables. Here's
the syntax borrowed from the one to alter domains:

ALTER TABLE t ALTER [COLUMN] c SET DEFAULT default_value;
ALTER TABLE t ALTER [COLUMN] c DROP DEFAULT;

Notes:
- Array fields cannot have a default value.
- If you change the type of a field, the default may remain in place. This is
because a field can be given the type of a domain with a default but the field
itself can override such domain. On the other hand, the field can be given a
type directly in whose case the default belongs logically to the field (albeit
the information is kept on an implicit domain created behind scenes).


6) New blob filter restriction.
(Dmitry Yemanov)

In FB2, the pair (input subtype, output subtype) must be unique for blob filter
declarations. This fix stops ambiguity in deciding which blob filter will be
executed to go from input type X to output type Z. If you have such problem in
databases created with earlier versions and you have a backup for them that want
to restore in FB2, expect to see your database restore being rejected by FB2.


7) ALTER computed fields
(Adriano dos Santos Fernandes)

Syntax:

alter table ...
    alter <field> [type <data type>] computed by (<expression>);

Notes:
- You cannot alter a non-COMPUTED field to COMPUTED and vice-versa.

Example:

create table test (
    n integer,
    dn computed by (n * 2)
);

alter table test
    alter dn computed by (n + n);


8) ALTER VIEW
(Adriano dos Santos Fernandes)

Function:

Ability to alter a view without the need to recreate (drop and create) the view and all of it's
dependencies.

Syntax:

{ create [ or alter ] | alter } view <view_name> [ ( <field list> ) ]
    as <select statement>

Example:

create table users (
    id integer,
    name varchar(20),
    passwd varchar(20)
);

create view v_users as
    select name from users;

alter view v_users (id, name) as
    select id, name from users;


9) GRANT/REVOKE rights GRANTED BY specified user
(Alex Peshkoff)

Function:

Makes it possible to specify non-default (which is CURRENT_USER) grantor in GRANT and REVOKE
commands.

Syntax:

grant <right> to <object> [ { granted by | as } [ user ] <username> ]
revoke <right> from <object> [ { granted by | as } [ user ] <username> ]

Example (as SYSDBA):

create role r1;
grant r1 to user1 with admin option;
grant r1 to public granted by user1;

(in isql)
show grant;
/* Grant permissions for this database */
GRANT R1 TO PUBLIC GRANTED BY USER1
GRANT R1 TO USER1 WITH ADMIN OPTION

Misc:
GRANTED BY form of clause is recommended by SQL standard. AS is supported by some other
servers (Informix), and is added for better compatibility.


10) ALTER CHARACTER SET
(Adriano dos Santos Fernandes)

Function:

Allows to change the default collation of a character set.

The default collation is used when table columns are created with a given character set (explicit
or implicit through the database default character set) without a collation specified.
String constants also uses the default collation of the connection character set.

Syntax:

alter character set <charset_name>
    set default collation <collation_name>

Example:

create database 'peoples.fdb'
    default character set win1252;

alter character set win1252
    set default collation win_ptbr;

create table peoples (
    id integer,
    name varchar(50)  -- will use the database default character set and the win1252 default collation
);

insert into peoples values (1, 'adriano');
insert into peoples values (2, 'ADRIANO');

-- will retrieve both records because win_ptbr is case insensitive
select * from peoples where name like 'A%';


11) Default collation in CREATE DATABASE
(Adriano dos Santos Fernandes)

Function:

Specify the default collation of the default character set. See also ALTER CHARACTER SET.

Syntax:

create database <file name>
    [ page_size <page size> ]
    [ length = <length> ]
    [ user <user name> ]
    [ password <user password> ]
    [ set names <charset name> ]
    [ default character set <charset name> [ collation <collation name> ] ]
    [ difference file <file name> ]

Example:

create database 'test.fdb'
    default character set win1252 collation win_ptbr;


12) REVOKE ALL ON ALL
(Alex Peshkoff)

Function:

When user is removed from security database (or any other authentication source),
it's useful to revoke his access to any object in database.

Syntax:

REVOKE ALL ON ALL FROM [USER] username
REVOKE ALL ON ALL FROM [ROLE] rolename

Example:

# gsec -del guest
# isql employee
fbs bin # ./isql employee
Database:  employee
SQL> REVOKE ALL ON ALL FROM USER guest;
SQL>


13) Syntax for change nullability of a field or domain
(Adriano dos Santos Fernandes)

Nullability of a table field or a domain can now be changed with the ALTER command. Syntax:

ALTER TABLE <table name> ALTER <field name> {DROP | SET} NOT NULL

ALTER DOMAIN <domain name> {DROP | SET} NOT NULL

A change in a table from NULL to NOT NULL is subject to a full data validation on the table.
A change in a domain changes and validates all the tables using the domain.

An explicity NOT NULL on a field depending on a domain prevails over the domain. In this case,
changing the domain to nullable does not automatically change the field to nullable.


14) CONTINUE statement
(Adriano dos Santos Fernandes)

Syntax: CONTINUE [<label>];

CONTINUE is a complementary command to BREAK/LEAVE and allows the restart (next iteration) of a
FOR/WHILE block.


15) RECREATE, CREATE OR ALTER and ALTER SEQUENCE statements
(Adriano dos Santos Fernandes)
(Dmitry Yemanov)

Syntax present in 3.0:

{ CREATE | RECREATE } { SEQUENCE | GENERATOR } <sequence name> [ START WITH <value> ]

CREATE OR ALTER { SEQUENCE | GENERATOR } <sequence name> { RESTART | START WITH <value> }

ALTER { SEQUENCE | GENERATOR } <sequence name> RESTART [ WITH <value> ]

Syntax present in 2.5 for reference:

ALTER SEQUENCE <sequence name> RESTART WITH <value>


16) Increment for sequences.
(Claudio Valderrama)

For 3.0, the options specified in (15) include INCREMENT:
{ CREATE | RECREATE } { SEQUENCE | GENERATOR } <sequence name> [ START WITH <value> ] [ INCREMENT [BY] <increment> ]
CREATE OR ALTER { SEQUENCE | GENERATOR } <sequence name> { RESTART | START WITH <value> } [ INCREMENT [BY] <increment> ]
ALTER { SEQUENCE | GENERATOR } <sequence name> RESTART [ WITH <value> ] [ INCREMENT [BY] <increment> ]

This is the increment that's applied to the SQL standard way of working with generators:
NEXT VALUE FOR <sequence name>
is equivalent to
gen_id(<sequence name>, <increment>)

The default increment for user generators is one and for system generators, zero. This makes possible
to apply NEXT VALUE FOR <sys generator>
since system generators cannot be changed by user requests now. Example:

create sequence seq increment 10; -- starts at zero and changes by 10
select next value for seq from rdb$database; -- result is 10
select gen_id(seq, 1) from rdb$database; -- result is 11, gen_id() is not affected by the increment.

If the database is new and no trigger has been defined, doing
select next value for rdb$procedures from rdb$database;
many times will produce zero again and again, because it's a system generator.

The increment cannot be zero for user generators. Example:

SQL> create generator g00 increment 0;
Statement failed, SQLSTATE = 42000
unsuccessful metadata update
-CREATE SEQUENCE G00 failed
-INCREMENT 0 is an illegal option for sequence G00

The change in the value of INCREMENT is a feature that takes effect for each request that starts
after the change commits. Procedures that are invoked for the first time after the change in INCREMENT
will use the new value if they contain NEXT VALUE FOR statements. Procedures that are already
running are not affected because they're cached. Procedures relying on NEXT VALUE FOR do not need
to be recompiled to see the new increment, but if they are running already, they are loaded and
no effect is seen. Of course, gen_id(gen, expression) is not affected by the increment.


17) More security for system objects.
(Claudio Valderrama)

For 3.0, system domains cannot be altered:

SQL> alter domain rdb$linger type boolean;
Statement failed, SQLSTATE = 42000
unsuccessful metadata update
-ALTER DOMAIN RDB$LINGER failed
-System domain RDB$LINGER cannot be modified

The value of a system generator cannot be changed by user requests:

SQL> select gen_id(rdb$procedures, 1) from rdb$database;
               GEN_ID
=====================
Statement failed, SQLSTATE = 42000
System generator RDB$PROCEDURES cannot be modified

SQL> set generator rdb$procedures to 0;
Statement failed, SQLSTATE = 42000
unsuccessful metadata update
-SET GENERATOR RDB$PROCEDURES failed
-System generator RDB$PROCEDURES cannot be modified

Also, the name RDB$GENERATORS does not refer anymore to an invisible system
generator. The name can be applied to an user generator.


18) Added ROLE clause to CREATE DATABASE statement.
(Alex Peshkov)

For 3.0, setting role when creating database may affect user rights to create databases.
This can happen if user is granted (in appropriate security database) system role RDB$ADMIN
or some ordinary role which in turn is granted CREATE DATABASE right. When using API call
IProvider::createDatabase (or isc_create_database) there are no problems with placing
isc_dpb_sql_role_name tag with required value into DPB, but CREATE DATABASE statement
missed ROLE clause before 3.0.

ISQL now also takes into an account global role setting when creating databases.


19) Added {PRESERVE | DELETE} FILE clause to DROP SHADOW statement.
(Alex Peshkov)

In some cases it's desired to keep shadow file after dropping shadow (for example for
backup purporse). In FB3 appropriate clause is added to DROP SHADOW. Full syntax is:

DROP SHADOW <number> [{PRESERVE | DELETE} FILE]

Default behavior is to delete file, keeping backwards compatibility.


20) Added database encryption/decryption clauses to ALTER DATABASE statement.
(Alex Peshkov)

Database crypt support was added to FB3. Actual encryption is performed by appropriate
plugin, but process is controlled using SQL.

ALTER DATABASE ENCRYPT WITH <plugin-name> [ KEY <key-name> ]

Encrypts database using named plugin. Control returns from DDL command NOT waiting
for encryption completion, crypt progress can be monitored using
MON$DATABASE.MON$CRYPT_PAGE field. For example:

select MON$CRYPT_PAGE * 100 / MON$PAGES from MON$DATABASE

will print percent of crypt completion. Additional KEY clause makes it possible to pass name
of crypt key to dbcrypt plugin. It's plugin's decision what to do with that key name (up to
ignoring it).

ALTER DATABASE DECRYPT

Decrypts database.


21) New clauses in CREATE and ALTER role operators.
(Alex Peshkov)

Provide support for system privileges. One can:

CREATE ROLE <name> SET SYSTEM PRIVILEGES TO <privilege1> {, <privilege2> {, ...  <privilegeN> }}
ALTER ROLE <name> SET SYSTEM PRIVILEGES TO <privilege1> {, <privilege2> {, ...  <privilegeN> }}

This forms assign non-empty list of system privileges to role <name>. Privileges previously assigned
to role <name> are cleared when using second form.

ALTER ROLE <name> DROP SYSTEM PRIVILEGES

This form clears list of system privileges in role <name>.

System privileges make it possible to delegate part of DBO rights to other users.
Pay attention that system privileges provide very thin level of control, therefore sometimes
you will need to give user >1 privilege to perform some task (for example add
IGNORE_DB_TRIGGERS to USE_GSTAT_UTILITY cause gstat wants to ignore database triggers).

List of valid system privileges for FB4 is as follows:
USER_MANAGEMENT					Manage users
READ_RAW_PAGES					Read pages in raw format using Attachment::getInfo()
CREATE_USER_TYPES				Add/change/delete non-system records in RDB$TYPES
USE_NBACKUP_UTILITY				Use nbackup to create database's copies
CHANGE_SHUTDOWN_MODE			Shutdown DB and bring online
TRACE_ANY_ATTACHMENT			Trace other users' attachments
MONITOR_ANY_ATTACHMENT			Monitor (tables MON$) other users' attachments
ACCESS_SHUTDOWN_DATABASE		Access database when it's shut down
CREATE_DATABASE					Create new databases (given in security.db)
DROP_DATABASE					Drop this database
USE_GBAK_UTILITY				Use appropriate utility
USE_GSTAT_UTILITY				...
USE_GFIX_UTILITY				...
IGNORE_DB_TRIGGERS				Insruct engine not to run DB-level triggers
CHANGE_HEADER_SETTINGS			Modify parameters in DB header page
SELECT_ANY_OBJECT_IN_DATABASE	Use SELECT for any selectable object
ACCESS_ANY_OBJECT_IN_DATABASE	Access (in any possible way) any object
MODIFY_ANY_OBJECT_IN_DATABASE	Modify (up to drop) any object
CHANGE_MAPPING_RULES			Change authentication mappings
USE_GRANTED_BY_CLAUSE			Use GRANTED BY in GRANT and REVOKE operators
GRANT_REVOKE_ON_ANY_OBJECT		GRANT and REVOKE rights on any object in database
GRANT_REVOKE_ANY_DDL_RIGHT		GRANT and REVOKE any DDL rights
CREATE_PRIVILEGED_ROLES			Use SET SYSTEM PRIVILEGES in roles
MODIFY_EXT_CONN_POOL			Manage properties of pool of external connections
REPLICATE_INTO_DATABASE			Use replication API to load changesets into database
PROFILE_ANY_ATTACHMENT			Profile other users' attachments
GET_DBCRYPT_INFO				Use getInfo() items, related with DB encryption


22) New grantee type in GRANT and REVOKE operators - SYSTEM PRIVILEGE.
(Alex Peshkov)

With support for various system privileges in engine it's getting very convenient to grant some
rights to users already having specific system privilege. Therefore appropriate grantee type is
suppoprted now. Example:

GRANT ALL ON PLG$SRP.PLG$SRP_VIEW TO SYSTEM PRIVILEGE USER_MANAGEMENT

Grants all rights to view (used in SRP management plugin) to users having USER_MANAGEMENT privilege.

22) Added replication control clauses to ALTER DATABASE statement.
(Dmitry Yemanov)

ALTER DATABASE {ENABLE | DISABLE} PUBLICATION

Enables or disabled replication. The change is applied immediately after commit.

ALTER DATABASE INCLUDE ALL TO PUBLICATION

Enables replication for all tables inside the database, including the ones to be created in the future.

ALTER DATABASE INCLUDE TABLE {<table1>, <table2>, ..., <tableN>} TO PUBLICATION

Enables replication for the specified set of tables.

ALTER DATABASE EXCLUDE ALL FROM PUBLICATION

Disables replication for all tables inside the database, including the ones to be created in the future.

ALTER DATABASE EXCLUDE TABLE {<table1>, <table2>, ..., <tableN>} FROM PUBLICATION

Disables replication for the specified set of tables.

23) Added optional replication control clauses to CREATE TABLE and ALTER TABLE statements.
(Dmitry Yemanov)

CREATE TABLE <name> ... [ {ENABLE | DISABLE} PUBLICATION ]
ALTER TABLE <name> ... [ {ENABLE | DISABLE} PUBLICATION ]

Defines whether replication is enabled for the specified table.
If not specified in the CREATE TABLE statement, the database-level default behaviour is applied.

24) Added the ability to change deterministic and sql security option without specifying the entire body of the function.
(Alexander Zhdanov)

ALTER FUNCTION <name> [ {DETERMINISTIC | NOT DETERMINISTIC} ] [ SQL SECURITY {DEFINER | INVOKER} | DROP SQL SECURITY ]

25) Added the ability to change sql security option without specifying the entire body of the procedure
(Alexander Zhdanov)

ALTER PROCEDURE <name> SQL SECURITY {DEFINER | INVOKER} | DROP SQL SECURITY

26) Added the ability to change sql security option without specifying the entire body of the package
(Alexander Zhdanov)

ALTER PACKAGE <name> SQL SECURITY {DEFINER | INVOKER} | DROP SQL SECURITY

27) Added OWNER clause to CREATE DATABASE statement.
(Dmitry Sibiryakov)

<db_initial_option> list is expanded by "OWNER username" clause which allows to set an owner user name for the created database.
Only users with administrator rights can use this option.

28) COLLATE clause can be used as a part of character data type as per SQL standard.
(Dmitry Sibiryakov)

If is used twice, an error is returned.


DDL enhancements in Firebird v6.
--------------------------------

1) DROP [IF EXISTS]

Using subclause IF EXISTS, it's now possible to try to drop objects and do not get errors when they did not exist.

For ALTER TABLE ... DROP subclause, DDL triggers are not fired if there are only DROP IF EXISTS subclauses and all
of them are related to non existing columns or constraints.

For others commands where IF EXISTS is part of the main command, DDL triggers are not fired when the object
did not exist.

The following statements are supported:

DROP EXCEPTION [IF EXISTS] <exception>
DROP INDEX [IF EXISTS] <index>
DROP PROCEDURE [IF EXISTS] <procedure>
DROP TABLE [IF EXISTS] <table>
DROP TRIGGER [IF EXISTS] <trigger>
DROP VIEW [IF EXISTS] <view>
DROP FILTER [IF EXISTS] <filter>
DROP DOMAIN [IF EXISTS] <domain>
DROP [EXTERNAL] FUNCTION [IF EXISTS] <function>
DROP SHADOW [IF EXISTS] <shadow>
DROP ROLE [IF EXISTS] <role>
DROP GENERATOR [IF EXISTS] <generator>
DROP SEQUENCE [IF EXISTS] <sequence>
DROP COLLATION [IF EXISTS] <collation>
DROP USER [IF EXISTS] <user> [USING PLUGIN <plugin>]
DROP PACKAGE [IF EXISTS] <package>
DROP PACKAGE BODY [IF EXISTS] <package>
DROP [GLOBAL] MAPPING [IF EXISTS] <mapping>
ALTER TABLE <table> DROP [IF EXISTS] <column name>
ALTER TABLE <table> DROP CONSTRAINT [IF EXISTS] <constraint name>

2) CREATE [IF NOT EXISTS]

Using subclause IF NOT EXISTS, it's now possible to try to create objects and do not get errors when they
already exists.

For ALTER TABLE ... ADD subclause, DDL triggers are not fired if there are only IF NOT EXISTS subclauses and all
of them are related to existing columns or constraints.

For others commands (except users currently) where IF NOT EXISTS is part of the main command,
DDL triggers are not fired when the object already exists.

The engine only verifies if the name (object, column or constraint) already exists, and if yes, do nothing.
It never tries to match the existing object with the one being created.

Some objects share the same "namespace", for example, there cannot be a table and a procedure with the same name.
In this case, if there is table XYZ and CREATE PROCEDURE IF NOT EXISTS XYZ is tried, the procedure will not be created
and no error will be raised.

The following statements are supported:

CREATE EXCEPTION [IF NOT EXISTS] ...
CREATE INDEX [IF NOT EXISTS] ...
CREATE PROCEDURE [IF NOT EXISTS] ...
CREATE TABLE [IF NOT EXISTS] ...
CREATE TRIGGER [IF NOT EXISTS] ...
CREATE VIEW [IF NOT EXISTS] ...
CREATE FILTER [IF NOT EXISTS] ...
CREATE DOMAIN [IF NOT EXISTS] ...
CREATE FUNCTION [IF NOT EXISTS] ...
DECLARE EXTERNAL FUNCTION [IF NOT EXISTS] ...
CREATE SHADOW [IF NOT EXISTS] ...
CREATE ROLE [IF NOT EXISTS] ...
CREATE GENERATOR [IF NOT EXISTS] ...
CREATE SEQUENCE [IF NOT EXISTS] ...
CREATE COLLATION [IF NOT EXISTS] ...
CREATE USER [IF NOT EXISTS] ...
CREATE PACKAGE [IF NOT EXISTS] ...
CREATE PACKAGE BODY [IF NOT EXISTS] ...
CREATE [GLOBAL] MAPPING [IF NOT EXISTS] ...
ALTER TABLE <table> ADD [IF NOT EXISTS] <column name> ...
ALTER TABLE <table> ADD CONSTRAINT [IF NOT EXISTS] <constraint name> ...

3) Creation of an inactive index

CREATE [UNIQUE] [ASC[ENDING] | DESC[ENDING]]
  INDEX indexname [{ACTIVE | INACTIVE}]
  ON tablename {(col [, col ...]) | COMPUTED BY (<expression>)}
  [WHERE <search_condition>]

'isql -x' generates script accordingly.
