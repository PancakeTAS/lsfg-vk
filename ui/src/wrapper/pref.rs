use gtk::glib;
use gtk;
use adw;

pub mod dropdown;
pub mod entry;
pub mod switch;

glib::wrapper! {
    pub struct PrefDropdown(ObjectSubclass<dropdown::PrefDropdown>)
        @extends
            adw::PreferencesRow, gtk::ListBoxRow, gtk::Widget,
        @implements
            gtk::Accessible, gtk::Actionable, gtk::Buildable, gtk::ConstraintTarget;
}

glib::wrapper! {
    pub struct PrefSwitch(ObjectSubclass<switch::PrefSwitch>)
        @extends
            adw::PreferencesRow, gtk::ListBoxRow, gtk::Widget,
        @implements
            gtk::Accessible, gtk::Actionable, gtk::Buildable, gtk::ConstraintTarget;
}

glib::wrapper! {
    pub struct PrefEntry(ObjectSubclass<entry::PrefEntry>)
        @extends
            adw::PreferencesRow, gtk::ListBoxRow, gtk::Widget,
        @implements
            gtk::Accessible, gtk::Actionable, gtk::Buildable, gtk::ConstraintTarget;
}

impl PrefDropdown {
    pub fn new() -> Self {
        glib::Object::new()
    }
}

impl PrefSwitch {
    pub fn new() -> Self {
        glib::Object::new()
    }
}

impl PrefEntry {
    pub fn new() -> Self {
        glib::Object::new()
    }
}
