import mysql.connector

class ContactDB:
    def __init__(self):
        self._conn = None
        self._cursor = None

    def login(self, username, password, db_name):
        try:
            self._conn = mysql.connector.connect(host="dursley.socs.uoguelph.ca", database=db_name, user=username, password=password)
            self._conn.autocommit = True
            self._cursor = self._conn.cursor()
            create_query = "CREATE TABLE IF NOT EXISTS FILE (" \
                "file_id INT AUTO_INCREMENT," \
                "file_name VARCHAR(60) NOT NULL," \
                "last_modified DATETIME," \
                "creation_time DATETIME NOT NULL," \
                "PRIMARY KEY(file_id))"
            self._cursor.execute(create_query)
            create_query = "CREATE TABLE IF NOT EXISTS CONTACT (" \
                "contact_id INT AUTO_INCREMENT," \
                "name VARCHAR(256) NOT NULL," \
                "birthday DATETIME," \
                "anniversary DATETIME," \
                "file_id INT NOT NULL," \
                "PRIMARY KEY(contact_id)," \
                "FOREIGN KEY(file_id) REFERENCES FILE(file_id))"
            self._cursor.execute(create_query)
            return True
        except mysql.connector.Error as err:
            return False
        
    def insert_file(self, file_name, last_modified, creation_time):
        query = f"INSERT INTO FILE (file_name, last_modified, creation_time) VALUES ('{file_name}', '{last_modified}', '{creation_time}')"
        try:
            self._cursor.execute(query)
            return True
        except mysql.connector.Error as err:
            return False

    def insert_contact(self, name, birthday, anniversary, file_id):
        query = f"INSERT INTO CONTACT (name, birthday, anniversary, file_id) VALUES ('{name}', '{birthday}', '{anniversary}', {file_id})"
        try:
            self._cursor.execute(query)
            return True
        except mysql.connector.Error as err:
            return False
        
    def close_connection(self):
        self._cursor.close()    
        self._conn.close()