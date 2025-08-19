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

    /// Get the display name (custom name if set, otherwise exe name)
    pub fn display_name(&self) -> String {
        self.imp().display_name()
    }

    /// Update the label to show the current display name
    pub fn update_label(&self) {
        self.imp().update_label();
    }
}
