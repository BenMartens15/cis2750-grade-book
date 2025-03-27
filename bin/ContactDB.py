import mysql.connector

class ContactDB:
    def __init__(self):
        pass

    def login(self, username, password, db_name):
        try:
            self.conn = mysql.connector.connect(host="dursley.socs.uoguelph.ca", database=db_name, user=username, password=password)
            return True
        except mysql.connector.Error as err:
            return False