from asciimatics.widgets import Frame, ListBox, Layout, Button, Divider, Text, \
    TextBox, Widget
from asciimatics.widgets.utilities import THEMES
from asciimatics.scene import Scene
from asciimatics.screen import Screen
from asciimatics.exceptions import ResizeScreenError, NextScene, StopApplication
import sys
from ctypes import *
from os import listdir
from Card import Card, List
import pathlib

RED_AND_GREY_THEME = {
    "background": (Screen.COLOUR_DEFAULT, Screen.A_BOLD, 251),
    "borders": (Screen.COLOUR_BLACK, Screen.A_BOLD, 251),
    "button": (Screen.COLOUR_BLACK, Screen.A_BOLD, 251),
    "control": (Screen.COLOUR_DEFAULT, Screen.A_BOLD, Screen.COLOUR_DEFAULT),
    "disabled": (Screen.COLOUR_DEFAULT, Screen.A_BOLD, Screen.COLOUR_DEFAULT),
    "edit_text": (Screen.COLOUR_BLACK, Screen.A_BOLD, 251),
    "field": (Screen.COLOUR_BLACK, Screen.A_BOLD, 251),
    "focus_button": (251, Screen.A_BOLD, Screen.COLOUR_RED),
    "focus_control": (Screen.COLOUR_DEFAULT, Screen.A_BOLD, Screen.COLOUR_DEFAULT),
    "focus_edit_text": (240, Screen.A_BOLD, Screen.COLOUR_RED),
    "focus_field": (Screen.COLOUR_BLACK, Screen.A_BOLD, 251),
    "invalid": (Screen.COLOUR_DEFAULT, Screen.A_BOLD, Screen.COLOUR_DEFAULT),
    "label": (Screen.COLOUR_BLACK, Screen.A_BOLD, 251),
    "scroll": (Screen.COLOUR_DEFAULT, Screen.A_BOLD, Screen.COLOUR_DEFAULT),
    "selected_control": (Screen.COLOUR_DEFAULT, Screen.A_BOLD, Screen.COLOUR_DEFAULT),
    "selected_field": (251, Screen.A_BOLD, Screen.COLOUR_RED),
    "selected_focus_control": (Screen.COLOUR_DEFAULT, Screen.A_BOLD, Screen.COLOUR_DEFAULT),
    "selected_focus_field": (251, Screen.A_BOLD, Screen.COLOUR_RED),
    "title": (Screen.COLOUR_RED, Screen.A_BOLD, 251)
}

class VCardModel():
    def __init__(self):
        self.parser_lib = CDLL("./libvcparser.so")
        self._vcard_files = dict()
        self.current_id = None

        # parser library functions
        self.createCard = self.parser_lib.createCard
        self.createCard.argtypes = [c_char_p, POINTER(POINTER(Card))]
        self.createCard.restype = c_int

        self.validateCard = self.parser_lib.validateCard
        self.validateCard.argtypes = [POINTER(Card)]
        self.validateCard.restype = c_int

        self.dateToString = self.parser_lib.dateToString
        self.dateToString.argtypes = [c_void_p]
        self.dateToString.restype = c_char_p

        self.getFromFront = self.parser_lib.getFromFront
        self.getFromFront.argtypes = [POINTER(List)]
        self.getFromFront.restype = c_void_p

        self.insertFront = self.parser_lib.insertFront
        self.insertFront.argtypes = [POINTER(List), c_void_p]
        self.insertFront.restype = None

        self.clearList = self.parser_lib.clearList
        self.clearList.argtypes = [POINTER(List)]
        self.clearList.restype = None

        self.writeCard = self.parser_lib.writeCard
        self.writeCard.argtypes = [c_char_p, POINTER(Card)]
        self.writeCard.restype = c_int

    def add(self, contact):
        pass

    def get_cards(self):
        self._vcard_files.clear()
        file_list = listdir("./cards/")

        vcard_files = list()
        card_id = 0
        for file in file_list:
            card_pointer = POINTER(Card)()

            error = self.createCard(str.encode("./cards/" + file), byref(card_pointer))
            if (error != 0):
                continue
            
            error = self.validateCard(card_pointer)
            if (error != 0):
                continue

            vcard_files.append((file, card_id))
            self._vcard_files[card_id] = file
            card_id += 1

        return vcard_files

    def get_card_details(self, card_id): # returns a dictionary with keys "file_name", "full_name", "birthday", "anniversary", and "other_properties"
        card_pointer = POINTER(Card)()
        file_name = self._vcard_files[card_id]
        self.createCard(str.encode("./cards/" + file_name), byref(card_pointer))

        full_name = cast(self.getFromFront(card_pointer.contents.fn.contents.values), c_char_p).value.decode('utf-8')
        bday = self.dateToString(card_pointer.contents.birthday)
        if bday is not None:  
            bday = bday.decode('utf-8')
            bday = bday[:-1] # remove the \n
        anniversary = self.dateToString(card_pointer.contents.anniversary)
        if anniversary is not None:
            anniversary = anniversary.decode('utf-8')
            anniversary = anniversary[:-1] # remove the \n
        other_properties = card_pointer.contents.optionalProperties.contents.length

        return {"file_name": file_name, "full_name": full_name, "birthday": bday, "anniversary": anniversary, "other_properties": str(other_properties)}

    def get_current_card(self):
        if self.current_id is None:
            return {"file_name": "", "full_name": "", "birthday": "", "anniversary": "", "other_properties": "0"}
        else:
            return self.get_card_details(self.current_id)

    def update_current_card(self, details):
        if self.current_id is None:
            self.add(details)
        else:
            # get the old data for the contact
            card_pointer = POINTER(Card)()
            file_name = self._vcard_files[self.current_id]
            self.createCard(str.encode("./cards/" + file_name), byref(card_pointer))
            
            # update the contact information
            self.clearList(card_pointer.contents.fn.contents.values)
            self.insertFront(card_pointer.contents.fn.contents.values, str.encode(details["full_name"]))
            self.writeCard(str.encode("./cards/" + details["file_name"]), card_pointer)

            
    def delete_card(self, card_id):
        pathlib.Path.unlink("./cards/" + self._vcard_files[card_id])
        del self._vcard_files[card_id]

class ListView(Frame):
    def __init__(self, screen, model):
        super(ListView, self).__init__(screen,
                                       screen.height * 2 // 3,
                                       screen.width * 2 // 3,
                                       on_load=self._reload_list,
                                       hover_focus=True,
                                       can_scroll=False,
                                       title="vCard List")
        # Save off the model that accesses the contacts database.
        self._model = model

        self.set_theme("red_and_grey")

        # Create the form for displaying the list of vCard files.
        self._list_view = ListBox(
            Widget.FILL_FRAME,
            model.get_cards(),
            name="cards",
            add_scroll_bar=True,
            on_change=self._on_pick,
            on_select=self._edit)
        self._edit_button = Button("Edit", self._edit)
        self._delete_button = Button("Delete", self._delete)
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        layout.add_widget(self._list_view)
        layout.add_widget(Divider())
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Add", self._add), 0)
        layout2.add_widget(self._edit_button, 1)
        layout2.add_widget(self._delete_button, 2)
        layout2.add_widget(Button("Quit", self._quit), 3)
        self.fix()
        self._on_pick()

    def _on_pick(self):
        self._edit_button.disabled = self._list_view.value is None
        self._delete_button.disabled = self._list_view.value is None

    def _reload_list(self, new_value=None):
        self._list_view.options = self._model.get_cards()
        self._list_view.value = new_value

    def _add(self):
        self._model.current_id = None
        raise NextScene("vCard Details")

    def _edit(self):
        self.save()
        self._model.current_id = self.data["cards"]
        raise NextScene("vCard Details")

    def _delete(self):
        self.save()
        self._model.delete_card(self.data["cards"])
        self._reload_list()

    @staticmethod
    def _quit():
        raise StopApplication("User pressed quit")


class DetailsView(Frame):
    def __init__(self, screen, model):
        super(DetailsView, self).__init__(screen,
                                          screen.height * 2 // 3,
                                          screen.width * 2 // 3,
                                          hover_focus=True,
                                          can_scroll=False,
                                          title="vCard Details",
                                          reduce_cpu=True)
        # Save off the model that accesses the contacts database.
        self._model = model

        self.set_theme("red_and_grey")

        # Create the form for displaying the list of contacts.
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        layout.add_widget(Text("File Name:", "file_name"))
        layout.add_widget(Text("Contact:", "full_name"))
        layout.add_widget(Text("Birthday:", "birthday"))
        layout.add_widget(Text("Anniversary:", "anniversary"))
        layout.add_widget(Text("Other Properties:", "other_properties"))
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 3)
        self.fix()

    def reset(self):
        # Do standard reset to clear out form, then populate with new data.
        super(DetailsView, self).reset()
        self.data = self._model.get_current_card()

    def _ok(self):
        self.save()
        self._model.update_current_card(self.data)
        raise NextScene("vCard List")

    @staticmethod
    def _cancel():
        raise NextScene("vCard List")


def main_screen(screen, scene):
    scenes = [
        Scene([ListView(screen, contacts)], -1, name="vCard List"),
        Scene([DetailsView(screen, contacts)], -1, name="vCard Details")
    ]

    screen.play(scenes, stop_on_resize=True, start_scene=scene, allow_int=True)


contacts = VCardModel()
last_scene = None
THEMES["red_and_grey"] = RED_AND_GREY_THEME
while True:
    try:
        Screen.wrapper(main_screen, catch_interrupt=True, arguments=[last_scene])
        sys.exit(0)
    except ResizeScreenError as e:
        last_scene = e.scene