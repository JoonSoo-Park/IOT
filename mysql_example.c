#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>

void finish_with_error(MYSQL *conn) 
{
    fprintf(stderr, "%s\n", mysql_error(conn));
    mysql_close(conn);
    exit(1);
}

int main()
{
    MYSQL *conn = mysql_init(NULL);

    if (conn == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    if (mysql_real_connect(conn, "localhost", "iotserver", "password", NULL, 0, NULL, 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    if (mysql_query(conn, "CREATE DATABASE IF NOT EXISTS testdb"))
    {
        finish_with_error(conn);
    }

    if (mysql_query(conn, "use testdb"))
    {
        finish_with_error(conn);
    }

    if (mysql_query(conn, "DROP TABLE IF EXISTS Cars"))
    {
        finish_with_error(conn);
    }

    if (mysql_query(conn, "CREATE TABLE Cars(Id INT, Name TEXT, Price INT)"))
    {
        finish_with_error(conn);
    }

    if (mysql_query(conn, "INSERT INTO Cars VALUE(1, 'Audi', 523642)"))
    {
        finish_with_error(conn);
    }

    if (mysql_query(conn, "INSERT INTO Cars VALUE(2, 'Mercedes', 123451)"))
    {
        finish_with_error(conn);
    }

    if (mysql_query(conn, "SELECT * FROM Cars"))
    {
        finish_with_error(conn);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL)
    {
        finish_with_error(conn);
    }

    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    while (row = mysql_fetch_row(result))
    {
        for (int i = 0; i < num_fields; ++i)
            printf("%s     ", row[i] ? row[i] : "NULL");
        printf("\n");
    }

    mysql_free_result(result);
    mysql_close(conn);
    exit(0);
}