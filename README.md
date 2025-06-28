# CIS*2750 Contact Manager   
This is a cumulative project for U of Guelph course CIS*2750. The goal of the project was to create a contact manager program that stores contact information using the vCard format. The program consists of three major components: the vCard parser library (written in C), the UI (written in Python), and the MySQL database.

### vCard Parser
The parser library provides the following funtionality:
 - Validate the contents of a given vCard file (following the RFC 6350 specification) and provide relavent error codes if an issue is found
 - Extract the contact details from a valid vCard file
 - Export existing contact data to a vCard file 

### User Interface
The user interface is developed using the Python Asciimatics library and utilizes the vCard parser library (via ctypes) to provide the following functionality:
 - View and edit details for an existing contact
 - Create a new contact
 - Delete an existing contact
 - Connect to the database and run several simple queries

### Database
The database stores the information about each contact that has been created by the program as well as the location of the vCard file corresponding to the contact.