from ctypes import *

# Node of a linked list
class Node(Structure):
    pass

Node._fields_ = [("data", c_void_p),
                ("previous", POINTER(Node)),
                ("next", POINTER(Node))]


# Structure for the head of the linked list
DELFUNC = CFUNCTYPE(None, c_void_p)
CMPFUNC = CFUNCTYPE(c_int, c_void_p, c_void_p)
PRTFUNC = CFUNCTYPE(c_char_p, c_void_p)

class List(Structure):
    _fields_ = [("head", POINTER(Node)),
                ("tail", POINTER(Node)),
                ("length", c_int),
                ("deleteData", POINTER(DELFUNC)),
                ("compare", POINTER(CMPFUNC)),
                ("printData", POINTER(PRTFUNC))]
    

# Structure for a DateTime object
class DateTime(Structure):
    _fields_ = [("UTC", c_bool),
                ("isText", c_bool),
                ("date", c_char_p),
                ("time", c_char_p),
                ("text", c_char_p)]


# Structure for a vCard property
class Property(Structure):
    _fields_ = [("name", c_char_p),
                ("group", c_char_p),
                ("parameters", POINTER(List)),
                ("values", POINTER(List))]
    

# Structure for a vCard object
class Card(Structure):
    _fields_ = [("fn", POINTER(Property)),
                ("optionalProperties", POINTER(List)),
                ("birthday", POINTER(DateTime)),
                ("anniversary", POINTER(DateTime))]