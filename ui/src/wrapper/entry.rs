use gtk::glib;
use gtk;

pub mod entry;

glib::wrapper! {
    pub struct Entry(ObjectSubclass<entry::Entry>)
        @extends
            gtk::ListBoxRow, gtk::Widget,
        @implements
            gtk::Accessible, gtk::Actionable, gtk::Buildable, gtk::ConstraintTarget;
}

impl Entry {
    pub fn new() -> Self {
        glib::Object::new()
    }
}
