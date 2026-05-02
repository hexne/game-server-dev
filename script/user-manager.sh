#!/bin/bash

DB_USER="root"
DB_PASS="arch"
DB_NAME="game"

insert_users() {
    for i in $(seq 1 200); do
        username="user$i"
        password="pass$i"
        number="num$i"

        mysql -u"$DB_USER" -p"$DB_PASS" "$DB_NAME" \
            -e "INSERT INTO users (name, password_hash, number)
                VALUES ('$username', SHA2('$password', 256), '$number');"
    done
    echo "Inserted 200 users."
}

delete_users() {
    mysql -u"$DB_USER" -p"$DB_PASS" "$DB_NAME" \
        -e "DELETE FROM users;"
    echo "Deleted all users."
}

case "$1" in
    insert)
        insert_users
        ;;
    delete)
        delete_users
        ;;
    *)
        echo "Usage: $0 {insert|delete}"
        ;;
esac
