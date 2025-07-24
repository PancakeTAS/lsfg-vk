use gtk::glib;
use gtk;
use adw;

pub mod process;
pub mod process_entry;

glib::wrapper! {
    pub struct ProcessPicker(ObjectSubclass<process::ProcessPicker>)
        @extends
            adw::ApplicationWindow, adw::Window,
            gtk::ApplicationWindow, gtk::Window, gtk::Widget,
        @implements
            gtk::gio::ActionGroup, gtk::gio::ActionMap,
            gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget,
            gtk::Native, gtk::Root, gtk::ShortcutManager;
}

glib::wrapper! {
    pub struct ProcessEntry(ObjectSubclass<process_entry::ProcessEntry>)
    @extends
        gtk::ListBoxRow, gtk::Widget,
    @implements
        gtk::Accessible, gtk::Actionable, gtk::Buildable, gtk::ConstraintTarget;
}


impl ProcessPicker {
    pub fn new() -> Self {
        glib::Object::new()
    }
}

impl ProcessEntry {
    pub fn new() -> Self {
        glib::Object::new()
    }
}
